/* NdrTypes.h - ORPC NDR (un)marshaling types */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*

modification history
--------------------
01s,02aug01,dbs  add [v1_enum] support
01r,08feb01,nel  SPR#63885. SAFEARRAYs added. 
01q,24aug00,dbs  fix many OPC-related SPRs, copied over from T2 VxDCOM
01p,18jul00,dbs  add enum NDR type
01o,29feb00,dbs  fix typo and previous unmarshaling bug
01n,28feb00,dbs  fix nasty bug when unmarshaling arrays of pointers
01m,24feb00,dbs  add back-ptr to NDRTYPES
01l,07feb00,dbs  simplify NdrType classes, enhance marshaling of arrays to
                 support all kinds
01k,14oct99,dbs  apply fix for ARM double format
01j,23sep99,dbs  add final parts of VARIANT marshaling
01i,16sep99,dbs  marshaling enhancements, part 2
01h,14sep99,dbs  add VARIANT, pointer and string types - first stage
01g,16aug99,dbs  add variant and string support
01f,12aug99,dbs  improve struct support
01e,30jul99,dbs  tighten up type-safety of NDR types
01d,24may99,dbs  fix interface-ptr marshaling
01c,20may99,dbs  add type-kind method to all classes
01b,14may99,dbs  add alignment requirement to NdrType
01a,12may99,dbs  created

*/

#ifndef __INCNdrTypes_h
#define __INCNdrTypes_h

#include "dcomProxy.h"
#include "NdrStreams.h"

//////////////////////////////////////////////////////////////////////////
//
// NdrSimple<T> -- an NdrType subclass for all basic C/C++ primitive
// types.
//

template <typename PrimT>
class NdrSimple : public NdrType
    {
  public:
    NdrSimple (NDRTYPES& n) : NdrType (n), m_pValue (0) {}

    TypeKind	kind () const { return TK_SIMPLE; }
    void	resize (size_t) {}
    size_t	size (NdrUnmarshalStream*) { return sizeof (PrimT); }
    size_t	alignment () const { return sizeof (PrimT); }
    long	value () const { return (long) *m_pValue; }
    void	bind (void* pv) { m_pValue = (PrimT*) pv; }
    
    HRESULT 	marshal1 (NdrMarshalStream* pStrm)
	{
	pStrm->align (sizeof (PrimT));
	return pStrm->insert (sizeof (PrimT), m_pValue, true);
	}
    
    HRESULT 	unmarshal1 (NdrUnmarshalStream* pStrm)
	{
	pStrm->align (sizeof (PrimT));
	return pStrm->extract (sizeof (PrimT), m_pValue, true);
	}

  private:
    PrimT*	m_pValue;
    };

//////////////////////////////////////////////////////////////////////////
//
// NdrEnum -- an NdrType subclass for handling old-style (16-bit)
// enumerated values. These are conveyed on the wire as 16-bit values
// (if declared as IDL enums) but need to be marshaled from (and
// unmarshaled to) true 'enum' types as understood by the local
// compiler.
//

class NdrEnum : public NdrType
    {
    enum DUMMY { DUMMY_VALUE0=0, DUMMY_VALUE1=1 };
    
  public:
    NdrEnum (NDRTYPES& n)
      : NdrType (n),
        m_pValue (0),
        m_bV1Enum (false)
	{}

    TypeKind	kind () const { return TK_SIMPLE; }
    void	resize (size_t) {}
    size_t	size (NdrUnmarshalStream*) { return sizeof (DUMMY); }
    size_t	alignment () const;
    long	value () const { return  (long) *m_pValue; }
    void	bind (void* pv) { m_pValue = (DUMMY*) pv; }
    
    void        init (bool isV1Enum = false) { m_bV1Enum = isV1Enum; }
    
    HRESULT	marshal1 (NdrMarshalStream* pStrm);
    HRESULT	unmarshal1 (NdrUnmarshalStream* pStrm);

  private:
    DUMMY*	m_pValue;
    bool        m_bV1Enum;
    };

//////////////////////////////////////////////////////////////////////////
//
// NdrStruct -- an NdrType subclass that represents 'structures' at
// the 'C' or IDL level. The init() method is given an array of
// StructMemberInfo items, each of which describes one member of the
// structure, in terms of its type (via an NdrType pointer) and its
// offset within the structure, plus the index of the 'size_is'
// member, i.e. the one holding the array-length.
//

class NdrStruct : public NdrType
    {
  public:

    NdrStruct (NDRTYPES& n)
      : NdrType (n),
	m_nMembers (0),
	m_pMemberInfo (0),
	m_pInstance (0),
	m_nSizeIs (0)
	{}

    virtual ~NdrStruct ();

    void	init (size_t nmems, const NdrMemberInfo mems [], int nsize_is);

    TypeKind	kind () const { return TK_STRUCT; }
    void	resize (size_t) {}
    size_t	size (NdrUnmarshalStream*);
    size_t	alignment () const;
    long	value () const { return 0; }
    void	bind (void*);
    HRESULT	marshal1 (NdrMarshalStream* pStrm);
    HRESULT	marshal2 (NdrMarshalStream* pStrm);
    HRESULT	unmarshal1 (NdrUnmarshalStream* pStrm);
    HRESULT	unmarshal2 (NdrUnmarshalStream* pStrm);
    
  protected:
    size_t		m_nMembers;	// number of members
    NdrMemberInfo*	m_pMemberInfo;	// member descriptions
    void*		m_pInstance;	// ptr to current instance
    int			m_nSizeIs;	// index of 'size_is' member
    };

//////////////////////////////////////////////////////////////////////////
//
// NdrPointer -- an NdrType subclass that represents a [unique] or
// [ptr] pointer to some other type...
//

class NdrPointer : public NdrType
    {
  public:

    NdrPointer (NDRTYPES& n, bool bRefPtr=false)
      : NdrType (n),
	m_pPtr (0),
	m_pointeeType (0),
	m_refptr (bRefPtr)
	{}

    void	init (const NdrTypeDesc& pt) { m_pointeeType=pt; }

    TypeKind	kind () const { return TK_PTR; }
    void	resize (size_t);
    size_t	size (NdrUnmarshalStream*) { return sizeof (void*); }
    size_t	alignment () const { return sizeof (long); }
    long	value () const { return 0; }
    void	bind (void* pv) { m_pPtr = (void**) pv; }
    HRESULT	marshal1 (NdrMarshalStream* pStrm);
    HRESULT	marshal2 (NdrMarshalStream* pStrm);
    HRESULT	unmarshal1 (NdrUnmarshalStream* pStrm);
    HRESULT	unmarshal2 (NdrUnmarshalStream* pStrm);
    
  protected:
    void**	m_pPtr;			// ptr to pointer-variable
    NdrTypeDesc	m_pointeeType;		// type of pointee
    bool	m_refptr;		// true when [ref] pointer
    };

//////////////////////////////////////////////////////////////////////////
//
// NdrArray -- an NdrType subclass that represents a fixed-size array
// at the IDL/C++ level. Its init() method is given the NdrType
// representation of each of its elements, the size (in raw memory
// bytes) of each element of the array, and the number of elements in
// the array.
//

class NdrArray : public NdrType
    {
  public:

    NdrArray (NDRTYPES& n)
      : NdrType (n),
	m_pElementType (0),
	m_nElementSize (0),
	m_ptr (0) ,
	m_arraySize (0),
	m_offset (0),
	m_max (0)
	{}

    void init
	(
	const NdrTypeDesc&	elementType,
	size_t			elemSize,
	size_t			max,
	size_t			offset,
	size_t			len
	);

    TypeKind	kind () const { return TK_ARRAY; }
    void	resize (size_t n) { m_arraySize = n; }
    size_t	size (NdrUnmarshalStream*);
    size_t	alignment () const;
    long	value () const { return 0; }
    void	bind (void*);
    HRESULT	marshal1 (NdrMarshalStream* pStrm);
    HRESULT	marshal2 (NdrMarshalStream* pStrm);
    HRESULT	unmarshal1 (NdrUnmarshalStream* pStrm);
    HRESULT	unmarshal2 (NdrUnmarshalStream* pStrm);
    
  protected:
    NdrTypeDesc	m_pElementType;		// type of element
    size_t	m_nElementSize;		// size of one element
    void*	m_ptr;			// ptr to current element

    size_t	m_arraySize;		// (transmitted) size of array
    size_t	m_offset;		// offset of mshl data
    size_t	m_max;			// max len of array
    };

//////////////////////////////////////////////////////////////////////////
//
// NdrConfArray -- an NdrType subclass that represents a 'conformant
// array' at the IDL/C++ level. It sub-classes NdrArray (the
// fixed-size array representation) and over-rides the marshal() and
// unmarshal() methods. Note that conformant arrays inside structures
// (conformant structures) are actual typed by WIDL as plain arrays,
// and conformant arrays are only used for method args, or are behind
// [unique] pointers if inside structures.
//

class NdrConfArray : public NdrArray
    {
  public:

    NdrConfArray (NDRTYPES& n) : NdrArray (n)
	{}

    TypeKind	kind () const { return TK_CARRAY; }
    size_t	size (NdrUnmarshalStream*);
    HRESULT	marshal1 (NdrMarshalStream* pStrm);
    HRESULT	unmarshal1 (NdrUnmarshalStream* pStrm);

    };

//////////////////////////////////////////////////////////////////////////
//
// NdrConfVarArray -- an NdrType subclass that represents a
// 'conformant varying array' at the IDL/C++ level. It sub-classes
// NdrConfArray and records the 'max' size of the array, which is
// different from the transmitted size of the array, and is declared
// like:-
//
// [size_is(max), length_is(num)] FOO* pFoo
//
// meaning that although the maximum size of the array is 'max' only
// 'num' elements are actually transmitted. Thanks DCE!
//

class NdrConfVarArray : public NdrConfArray
    {
  public:

    NdrConfVarArray (NDRTYPES& n) : NdrConfArray (n)
	{}

    TypeKind	kind () const { return TK_CVARRAY; }
    size_t	size (NdrUnmarshalStream*);
    HRESULT	marshal1 (NdrMarshalStream* pStrm);

    };

//////////////////////////////////////////////////////////////////////////
//
// NdrConfStruct -- an NdrType subclass that represents a 'conformant
// structure' at the IDL/C++ level, i.e. a structure whose final member
// is a conformant array, and the length of this array is held in one 
// of the structures other members.
//
// To achieve this, the final member is treated as if it were a
// fixed-size array, i.e. the WIDL-generated code should contain a
// pointer to an NdrArray as the last member of the structure
// definition, and it will be manipulated by the NdrConfStruct
// marshaling routines so that it works correctly. When the conformant
// structure is marshaled, the array-length is marshaled before the
// structure itself.
//

class NdrConfStruct : public NdrStruct
    {
  public:

    NdrConfStruct (NDRTYPES& n) : NdrStruct (n)
	{}

    TypeKind	kind () const { return TK_CSTRUCT; }
    size_t	size (NdrUnmarshalStream*);

    HRESULT	marshal1 (NdrMarshalStream* pStrm);
    HRESULT	unmarshal1 (NdrUnmarshalStream* pStrm);

  private:
    size_t	confElemResize ();
    };

//////////////////////////////////////////////////////////////////////////
//
// NdrInterface -- an NdrType subclass that represents a COM interface
// pointer at the 'C' or IDL level. The bind() method always takes the
// address of the interface-pointer variable, whether marshaling or
// unmarshaling.
//

class NdrInterface : public NdrType
    {
  public:

    NdrInterface (NDRTYPES& n) : NdrType (n), m_pPointer (0)
	{}

    void	init (REFIID riid) { m_iid = riid; }
    
    TypeKind	kind () const { return TK_INTERFACE; }
    void	resize (size_t) {}
    size_t	size (NdrUnmarshalStream*) { return sizeof (void*); }
    size_t	alignment () const { return sizeof (void*); }
    long	value () const { return 0; }
    void	bind (void* pv);
    HRESULT	marshal1 (NdrMarshalStream* pStrm);
    HRESULT	marshal2 (NdrMarshalStream* pStrm);
    HRESULT	unmarshal (NdrUnmarshalStream* pStrm);
    HRESULT	unmarshal1 (NdrUnmarshalStream* pStrm);
    HRESULT	unmarshal2 (NdrUnmarshalStream* pStrm);
    
  protected:
    IUnknown**	m_pPointer;		// address of interface-ptr
    IID		m_iid;			// IID of interface
    };

//////////////////////////////////////////////////////////////////////////
//
// NdrBSTR -- an NdrType subclass that represents a BSTR type...
//

class NdrBSTR : public NdrType
    {
  public:

    NdrBSTR (NDRTYPES& n) : NdrType (n), m_pBstr (0)
	{}

    TypeKind	kind () const { return TK_BSTR; }
    void	resize (size_t) {}
    size_t	size (NdrUnmarshalStream*) { return sizeof (void*); }
    size_t	alignment () const { return sizeof (void*); }
    long	value () const { return 0; }
    void	bind (void* pv) { m_pBstr = (BSTR*) pv; }
    HRESULT	marshal1 (NdrMarshalStream* pStrm);
    HRESULT	marshal2 (NdrMarshalStream* pStrm);
    HRESULT	unmarshal1 (NdrUnmarshalStream* pStrm);
    HRESULT	unmarshal2 (NdrUnmarshalStream* pStrm);
    
  protected:
    BSTR*	m_pBstr;		// ptr to BSTR variable
    };

//////////////////////////////////////////////////////////////////////////
//
// NdrVariant -- an NdrType subclass that represents a VARIANT type...
//

class NdrVariant : public NdrType
    {
  public:

    NdrVariant (NDRTYPES& n) : NdrType (n), m_pVariant (0)
	{}

    TypeKind	kind () const { return TK_STRUCT; }
    void	resize (size_t) {}
    size_t	size (NdrUnmarshalStream*);
    size_t	alignment () const { return sizeof (long); }
    long	value () const { return 0; }
    void	bind (void* pv) { m_pVariant = (VARIANT*) pv; }
    HRESULT	marshal1 (NdrMarshalStream* pStrm);
    HRESULT	marshal2 (NdrMarshalStream* pStrm);
    HRESULT	unmarshal1 (NdrUnmarshalStream* pStrm);
    HRESULT	unmarshal2 (NdrUnmarshalStream* pStrm);
    
  protected:
    VARIANT*	m_pVariant;		// ptr to VARIANT variable

    };


//////////////////////////////////////////////////////////////////////////
//
// NdrSafearray -- an NdrType subclass that represents a SAFEARRAY type...
//

class NdrSafearray : public NdrType
    {
  public:

    NdrSafearray (NDRTYPES& n);
    ~NdrSafearray ();

    TypeKind	kind () const { return TK_STRUCT; }
    void	resize (size_t) {}
    size_t	size (NdrUnmarshalStream*) { return sizeof (ULONG); }
    size_t	alignment () const { return sizeof (long); }
    long	value () const { return 0; }
    void	bind (void * pv);

    HRESULT	marshal1 (NdrMarshalStream* pStrm);
    HRESULT	marshal2 (NdrMarshalStream* pStrm);
    HRESULT	unmarshal1 (NdrUnmarshalStream* pStrm);
    HRESULT	unmarshal2 (NdrUnmarshalStream* pStrm);
    
  protected:
    VARIANT*	m_pVariant;		// ptr to VARIANT variable. This contains the
                                // memory rep of the SAFEARRAY.

  private:
    enum
        {
        PHASE1 = 0,
        PHASE2
        }       m_phase;        // used in unmarshalling pointer types.

    SAFEARRAY * m_pTag;         // used to hold a list of the tags for
                                // unmarshalling pointer types.

    HRESULT actionArray 
        (
        int                     dim, 
        NdrMarshalStream *      pMaStrm,
        NdrUnmarshalStream *    pUnStrm,
        long *                  ix
        );

    HRESULT marshalBody (NdrMarshalStream * pStrm, long * ix);
    HRESULT unmarshalBody (NdrUnmarshalStream * pStrm, long * ix);
    };

//////////////////////////////////////////////////////////////////////////
//
// NdrWString -- an NdrType subclass that represents a wide-string,
// NULL-terminated...
//

class NdrWString : public NdrType
    {
  public:

    NdrWString (NDRTYPES& n) : NdrType (n), m_pString (0) {}

    TypeKind	kind () const { return TK_PTR; }
    void	resize (size_t) {}
    size_t	size (NdrUnmarshalStream*);
    size_t	alignment () const { return sizeof (long); }
    long	value () const { return 0; }
    void	bind (void* pv) { m_pString = (OLECHAR*) pv; }
    HRESULT	marshal1 (NdrMarshalStream* pStrm);
    HRESULT	unmarshal1 (NdrUnmarshalStream* pStrm);
    
  protected:
    OLECHAR*	m_pString;		// ptr to string
    size_t	m_max;			// max length of string
    size_t	m_offset;		// offset of mshl data
    size_t	m_len;			// len of mshl data
    };


//////////////////////////////////////////////////////////////////////////
//
// NdrCString -- an NdrType subclass that represents a wide-string,
// NULL-terminated...
//

class NdrCString : public NdrType
    {
  public:

    NdrCString (NDRTYPES& n) : NdrType (n), m_pString (0) {}

    TypeKind	kind () const { return TK_PTR; }
    void	resize (size_t) {}
    size_t	size (NdrUnmarshalStream*);
    size_t	alignment () const { return sizeof (long); }
    long	value () const { return 0; }
    void	bind (void* pv) { m_pString = (char*) pv; }
    HRESULT	marshal1 (NdrMarshalStream* pStrm);
    HRESULT	unmarshal1 (NdrUnmarshalStream* pStrm);
    
  protected:
    char*	m_pString;		// ptr to string
    size_t	m_max;			// max length of string
    size_t	m_offset;		// offset of mshl data
    size_t	m_len;			// len of mshl data
    };


//////////////////////////////////////////////////////////////////////////
//
// NdrTypeFactory -- serves up memory for instances of any of the
// supported marshalable types, and records them so that they are
// destroyed when this object is destroyed. It achieves this by
// allocating the type elements from a private heap, which is
// destroyed upon destruction of the object itself. It relies on the
// class NdrType having a special placement-new operator which
// allocates memory from this factory object's private heap.
//

class NdrTypeFactory
    {
    enum { TYPESIZE=32 };
    
  public:
    NdrTypeFactory (int hint=256);
    ~NdrTypeFactory ();

    void*	allocate (size_t);
    
  private:

    char*	m_begin;
    char*	m_curr;
    char*	m_end;

    };


#endif



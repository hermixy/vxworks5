/* dcomProxy.h - VxWorks DCOM runtime marshaling support */

/* Copyright (c) 1998 Wind River Systems, Inc. */

/*
modification history
--------------------
02i,02aug01,dbs  add [v1_enum] support
02h,31jul01,dbs  remove obsolete InterfaceProxy class, add p/s registration
                 macro
02g,28feb01,nel  SPR#35698. Add boolean type.
02f,19sep00,nel  Change constness of widlMarshal to match T3.
02e,24aug00,dbs  fix many OPC-related SPRs
02d,18jul00,dbs  add CV array type and enum type
02c,02mar00,dbs  add revised VTBL macros
02b,11feb00,dbs  add void_t() to allow void-pointers in marshaling
02a,07feb00,dbs  simplify NdrType classes, enhance marshaling of arrays to
                 support all kinds
01z,02feb00,dbs  change proxy vtbl to be template-based
01y,14oct99,dbs  make double an explicit typedesc
01x,22sep99,dbs  add uhyper type
01w,16sep99,dbs  marshaling enhancements, part 2
01v,14sep99,dbs  change REFIIDs to const IID& for WIDL compatibility, add
                 VARIANT, pointer and string types
01u,12aug99,dbs  improve NDR structure support
01t,05aug99,dbs  remove byte typedef
01s,30jul99,dbs  tighten up type-safety of NDR types
01r,25jun99,dbs  add channel-ID to stub-msg
01q,03jun99,aim  removed BYTE_ORDER and LITTLE_ENDIAN defs
01p,02jun99,dbs  remove OS-specific macros
01o,28may99,dbs  make stub disp-tbl a struct
01n,27may99,dbs  remove DCE-specific stub-function type
01m,25may99,dbs  remove NDRTYPES::alloc/free methods
01l,20may99,dbs  move NDR phase into streams
01k,18may99,dbs  add proxy/stub marshaling phase to NDR-streams
01j,14may99,dbs  add alignment requirement to NdrType
01i,14may99,dbs  remove obsolete PFNSTUB type
01h,14may99,dbs  add new NDR support
01g,11may99,dbs  simplify proxy remoting architecture
01f,11may99,dbs  add fwd-decl of IRpcStubBuffer etc, rename symbols
                 like VXCOM_* to VXDCOM_*
01e,04may99,dbs  remove cplusplus check
01d,30apr99,dbs  fix up bracketing in vtbl macros
01c,29apr99,dbs  fix -Wall warnings
01b,22apr99,dbs  add HRESULT member to RPCOLEMESSAGE
01a,20apr99,dbs  added mod-hist
*/

/* This header is to be included in generated proxy/stub code. */

#ifndef __INCdcomProxy_h
#define __INCdcomProxy_h

#include "dcomLib.h"
#include "comObjLib.h"

//////////////////////////////////////////////////////////////////////////
//
// DATA REPRESENTATION -- we use the NDR format, which is actually
// defined as a vector of 4 bytes, the first of which contains the
// integer ordering field. However, its difficult in C to pass this
// around, so we use an unsigned-long representation instead.
//

#if (_BYTE_ORDER == _LITTLE_ENDIAN)
#define VXDCOM_NDR_CHAR_REP  0x00
#define VXDCOM_NDR_INT_REP   0x01
#define VXDCOM_NDR_FLOAT_REP 0x00
#define VXDCOM_DREP_LITTLE_ENDIAN 0x00000010
#define VXDCOM_DREP_BIG_ENDIAN 0x00000000
#define VXDCOM_DREP_MASK 0x000000FF
#define VXDCOM_DREP_LOCAL VXDCOM_DREP_LITTLE_ENDIAN
#else /* big endian */
#define VXDCOM_NDR_CHAR_REP  0x00
#define VXDCOM_NDR_INT_REP   0x00
#define VXDCOM_NDR_FLOAT_REP 0x00
#define VXDCOM_DREP_LITTLE_ENDIAN 0x10000000
#define VXDCOM_DREP_BIG_ENDIAN 0x00000000
#define VXDCOM_DREP_MASK 0xFF000000
#define VXDCOM_DREP_LOCAL VXDCOM_DREP_BIG_ENDIAN
#endif

#define VXDCOM_NDR_DATAREP0 ((VXDCOM_NDR_INT_REP << 4) | VXDCOM_NDR_CHAR_REP)
#define VXDCOM_NDR_DATAREP1 (VXDCOM_NDR_FLOAT_REP)
#define VXDCOM_NDR_DATAREP2 (0)
#define VXDCOM_NDR_DATAREP3 (0)

typedef unsigned long RPCOLEDATAREP;

//////////////////////////////////////////////////////////////////////////
//
// Forward type declarations...
//

class NdrTypeFactory;
class NdrMarshalStream;
class NdrUnmarshalStream;

class RpcProxyMsg;

class NDRTYPES;

///////////////////////////////////////////////////////////////////////////
//
// IUnknown proxy functions
//

extern HRESULT IUnknown_QueryInterface_vxproxy (IUnknown*, REFIID, void**);
extern ULONG   IUnknown_AddRef_vxproxy (IUnknown*);
extern ULONG   IUnknown_Release_vxproxy (IUnknown*);

///////////////////////////////////////////////////////////////////////////
//
// ndr_make_right - re-formats a 2-byte, 4-byte or 8-byte number
// into the format indicated by the dataRep field...
//

template <typename T>
inline void ndr_make_right (T& t, ULONG drepOther)
    {
    if (VXDCOM_DREP_LOCAL == drepOther)
        return;

    char *a = (char*) &t;
    char *b = ((char*) &t) + sizeof (T) - 1;
    char tmp;

    for (size_t n=0; n < (sizeof (T) / 2); ++n)
        {
        tmp = *a;
        *a = *b;
        *b = tmp;
        ++a;
        --b;
        }
    }


//////////////////////////////////////////////////////////////////////////
//
// NDR Utility macros...
//

#define NDR_SIZEOF(type)    ((size_t) ((char*)(((type*)0) + 1)))

#define NDR_OFFSETOF(type,member)    ((size_t) &((type*) 0)->member)

//////////////////////////////////////////////////////////////////////////
//
// Define NDR/IDL types that aren't part of C/C++
//

typedef unsigned long	DREP;

//////////////////////////////////////////////////////////////////////////
//

class NdrType
    {

  public:

    enum TypeKind
	{
	TK_SIMPLE=0,
	TK_STRUCT,
	TK_ARRAY,
	TK_CSTRUCT,
	TK_CARRAY,
	TK_VARRAY,
	TK_CVARRAY,
	TK_UNION,
	TK_NE_UNION,
	TK_PTR,
	TK_BSTR,
	TK_INTERFACE
	};

    void* operator new (size_t, NdrTypeFactory*);
    void  operator delete (void*);

    NdrType (NDRTYPES& n) : m_dwRefCount (0), m_ndrtypes (n) {}

    virtual ~NdrType () {}

    virtual TypeKind	kind () const =0;
    virtual size_t	size (NdrUnmarshalStream*) =0;
    virtual size_t	alignment () const =0;
    virtual void	resize (size_t) =0;
    virtual long	value () const =0;
    virtual void	bind (void*) =0;

    virtual HRESULT	marshal1 (NdrMarshalStream*) =0;
    virtual HRESULT	marshal2 (NdrMarshalStream*);

    virtual HRESULT	unmarshal1 (NdrUnmarshalStream*) =0;
    virtual HRESULT	unmarshal2 (NdrUnmarshalStream*);

    virtual HRESULT	marshal (NdrMarshalStream*);
    virtual HRESULT	unmarshal (NdrUnmarshalStream*);

    ULONG AddRef () { return ++m_dwRefCount; }
    ULONG Release ()
	{
	ULONG n = --m_dwRefCount;
	if (n == 0)
	    delete this;
	return n;
	}

    void arraySizeSet (int n);
    int  arraySizeGet ();

  private:
    DWORD	m_dwRefCount;
    NDRTYPES&	m_ndrtypes;
    };

//////////////////////////////////////////////////////////////////////////
//
// Define the generic 'type descriptor' as ref-counting pointer to an
// NdrType (subclass) instance.
//

typedef CComPtr<NdrType> NdrTypeDesc;

//////////////////////////////////////////////////////////////////////////
//
// NDR Structure support -- the nSizeIs member indicates the
// associated structure member which provides the dynamic sizing
// information for a particular data member. For fixed-size structure
// elements, this value will be -1 (i.e. there is no sizing info) and
// for those with a size_is attribute the field will hold the index
// (in the range 0..N-1 where N is the number of struct members) of
// the data member that is nominated in the size_is field.
//
// For conformant structures, i.e. those whose last element is a
// conformant-array, they may use the cstruct_t type descriptor, and
// then won't require the last member to have nSizeIs set.
//

struct NdrMemberInfo
    {
    NdrTypeDesc	pType;
    size_t	nOffset;
    int		nSizeIs;
    
    NdrMemberInfo (const NdrTypeDesc& pt, size_t offset, int nsize = -1)
	: pType (pt), nOffset (offset), nSizeIs (nsize)
	{}

    NdrMemberInfo () : pType (0), nOffset (0), nSizeIs (-1)
	{}
    };

#define NDR_MEMBER(sname,mem,typ)     NdrMemberInfo(typ,NDR_OFFSETOF(sname,mem))
#define NDR_MEMBERX(sname,mem,typ,sz) NdrMemberInfo(typ,NDR_OFFSETOF(sname,mem),sz)

//////////////////////////////////////////////////////////////////////////
//

struct RpcMode
    {
    enum Mode_t { OBJECT=0, DCE=1 };
    };

//////////////////////////////////////////////////////////////////////////
//
// RPC proxy message
//

class RPC_PROXY_MSG
    {
  public:
    RPC_PROXY_MSG (const IID& riid, RpcMode::Mode_t mode, ULONG opnum, void*);
    ~RPC_PROXY_MSG ();

    NdrMarshalStream*	marshalStreamGet ();
    HRESULT		SendReceive ();
    NdrUnmarshalStream* unmarshalStreamGet ();

    RpcProxyMsg*	m_pImpl;
    };

//////////////////////////////////////////////////////////////////////////
//
// RPC stub message
//

class RPC_STUB_MSG
    {
  public:
    RPC_STUB_MSG (NdrUnmarshalStream*, NdrMarshalStream*, int =0);
    ~RPC_STUB_MSG () {}

    NdrUnmarshalStream* unmarshalStreamGet ();
    NdrMarshalStream*	marshalStreamGet ();
    int			channelIdGet ();
    
  private:
    
    NdrUnmarshalStream* m_pUnmshlStrm;
    NdrMarshalStream*	m_pMshlStrm;
    int			m_channelId;
    };

//////////////////////////////////////////////////////////////////////////
//
// Stub dispatch function type...
//

typedef HRESULT (STDMETHODCALLTYPE * PFN_ORPC_STUB) (IUnknown*, RPC_STUB_MSG&);


///////////////////////////////////////////////////////////////////////////
//
// Stub dispatch-table type...
//

struct VXDCOM_STUB_DISPTBL
    {
    unsigned int		nFuncs;
    const PFN_ORPC_STUB*	funcs;
    };


//////////////////////////////////////////////////////////////////////////
//
// autoregistration helper class for proxy/stubs
//

class vxdcom_ps_autoreg
    {
  public:
    vxdcom_ps_autoreg
        (
        const IID&                      iid,
        const void*                     pv,
        const VXDCOM_STUB_DISPTBL*      ps
        );
    };

#define VXDCOM_PS_AUTOREGISTER(itf)     \
    static vxdcom_ps_autoreg            \
    _the_##itf##_ps_autoreg (IID_##itf, \
        &itf##_vxproxy_vtbl,            \
        &itf##_vxstub_disptbl)

//////////////////////////////////////////////////////////////////////////
//
// NDR Types
//

class NDRTYPES
    {
  public:
    int arraySize;

  public:
    NDRTYPES (int hint=256);
    ~NDRTYPES ();

    NdrTypeDesc	byte_t ();
    NdrTypeDesc	short_t ();
    NdrTypeDesc	long_t ();
    NdrTypeDesc	enum_t ();
    NdrTypeDesc	v1enum_t ();
    NdrTypeDesc	hyper_t ();
    NdrTypeDesc	float_t ();
    NdrTypeDesc	double_t ();
    NdrTypeDesc	bstr_t ();
    NdrTypeDesc	struct_t (int nelems, const NdrMemberInfo m[], int nSizeIs = -1);
    NdrTypeDesc	cstruct_t (int nelems, const NdrMemberInfo m[], int nSizeIs);
    NdrTypeDesc	array_t (const NdrTypeDesc& eltype, size_t elsz, size_t nelems=0);
    NdrTypeDesc	carray_t (const NdrTypeDesc& eltype, size_t elsz, size_t nelems=0);
    NdrTypeDesc	cvarray_t (const NdrTypeDesc& eltype, size_t elsz, size_t nelems=0, size_t nmax=0);
    NdrTypeDesc	interfaceptr_t (const IID&);
    NdrTypeDesc	variant_t ();
    NdrTypeDesc	cstring_t ();
    NdrTypeDesc	wstring_t ();
    NdrTypeDesc	pointer_t (const NdrTypeDesc& pointeeType);
    NdrTypeDesc	refptr_t (const NdrTypeDesc& pointeeType);

    // map C types to IDL types
    NdrTypeDesc	void_t () { return byte_t (); }
    NdrTypeDesc boolean_t () { return byte_t (); }
    NdrTypeDesc	char_t () { return byte_t (); }
    NdrTypeDesc	uchar_t	() { return byte_t (); }
    NdrTypeDesc	ushort_t () { return short_t (); }
    NdrTypeDesc	ulong_t () { return long_t (); }
    NdrTypeDesc	uhyper_t () { return hyper_t (); }
    NdrTypeDesc	int_t () { return long_t (); }
    NdrTypeDesc	uint_t () { return long_t (); }
    NdrTypeDesc	hresult_t () { return long_t (); }
    NdrTypeDesc	longlong_t () { return hyper_t (); }
    
  private:
    NdrTypeFactory*	m_pFactory;
    };

HRESULT widlMarshal (void const*, NdrMarshalStream*, const NdrTypeDesc&);
HRESULT widlUnmarshal (void*, NdrUnmarshalStream*, const NdrTypeDesc&);

#endif



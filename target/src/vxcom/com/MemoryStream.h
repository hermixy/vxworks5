/* MemoryStream.h */

/* Copyright (c) 1998 Wind River Systems, Inc. */

/*

modification history
--------------------
01h,10dec01,dbs  diab build
01g,27jun01,dbs  fix include paths and names
01f,01feb00,nel  Added ComTrack code
01e,28may99,dbs  use new STL usage
01d,26may99,dbs  move over to STL collections
01c,12may99,dbs  add methods to stream-class to access raw storage
01b,23apr99,dbs  added SimpleStream interface
01a,19apr99,dbs  created

  */

#ifndef __INCMemoryStream_h
#define __INCMemoryStream_h

// Inhibit ComTrack for these internal classes
#ifdef VXDCOM_COMTRACK_LEVEL
#undef VXDCOM_COMTRACK_LEVEL
#endif
#define VXDCOM_COMTRACK_LEVEL 0

#include "comObjLib.h"			// for IStream
#include "private/comStl.h"             // for std::vector class

///////////////////////////////////////////////////////////////////////////
//
// ISimpleStream interface - purely for internal use within VxDCOM,
// this interface provides a lightweight version of the official COM
// IStream interface, with 32-bit args (rather than 64-bit as in
// IStream) and fewer members.
//
// As this is a LOCAL interface, and not intended ever to be remoted
// (that is what IStream is for) it does not return HRESULTs but
// rather returns results directly as return values.
//

interface ISimpleStream : public IUnknown
    {
    virtual ULONG STDMETHODCALLTYPE extract
	(
	void*		pv,		// buffer to extract to
	ULONG		nb		// num bytes to extract
	) =0;

    virtual ULONG STDMETHODCALLTYPE insert
	(
	const void*	pv,		// data to insert
	ULONG		nb		// num bytes to insert
	) =0;

    virtual ULONG STDMETHODCALLTYPE locationSet
	(
	ULONG		index		// index to seek to
	) =0;

    virtual ULONG STDMETHODCALLTYPE locationGet () =0;

    virtual ULONG STDMETHODCALLTYPE size () =0;
    };

EXTERN_C const IID IID_ISimpleStream;

///////////////////////////////////////////////////////////////////////////
//
// Memory-based IStream objects. Built using WOTL classes, so uses
// task-allocator for memory automatically. Implements both IStream
// and ISimpleStream.
//

class MemoryStream : public CComObjectRoot,
		     public IStream,
		     public ISimpleStream
    {

  private:
    typedef STL_VECTOR(char)		VECTOR;
    typedef VECTOR::iterator		ITERATOR;
    
    VECTOR	m_vector;		// storage
    ULONG	m_curr;			// curr location

  public:

    MemoryStream ();
    ~MemoryStream ();

#ifdef __DCC__
    char*  begin () { return &(*(m_vector.begin ())); }
    char*  end ()   { return &(*(m_vector.end ())); }
#else
    char*  begin () { return m_vector.begin (); }
    char*  end () { return m_vector.end (); }
#endif
    size_t size () const { return m_vector.size (); }
    //
    // ISimpleStream methods
    //
    ULONG STDMETHODCALLTYPE extract (void*, ULONG);
    ULONG STDMETHODCALLTYPE insert (const void*,ULONG);
    ULONG STDMETHODCALLTYPE locationSet (ULONG);
    ULONG STDMETHODCALLTYPE locationGet ();
    ULONG STDMETHODCALLTYPE size ();

    //
    // IStream methods
    //
    HRESULT STDMETHODCALLTYPE Read (void*, ULONG, ULONG*);
    HRESULT STDMETHODCALLTYPE Write (const void*, ULONG, ULONG*);
    HRESULT STDMETHODCALLTYPE Seek (LARGE_INTEGER, DWORD, ULARGE_INTEGER*);
    HRESULT STDMETHODCALLTYPE SetSize (ULARGE_INTEGER)
        { return E_NOTIMPL; }

    HRESULT STDMETHODCALLTYPE CopyTo (IStream*,
				      ULARGE_INTEGER,
				      ULARGE_INTEGER*,
				      ULARGE_INTEGER*)
        { return E_NOTIMPL; }

    HRESULT STDMETHODCALLTYPE Commit (DWORD)
        { return E_NOTIMPL; }

    HRESULT STDMETHODCALLTYPE Revert ()
        { return E_NOTIMPL; }

    HRESULT STDMETHODCALLTYPE LockRegion (ULARGE_INTEGER,
					  ULARGE_INTEGER,
					  DWORD)
        { return E_NOTIMPL; }

    HRESULT STDMETHODCALLTYPE UnlockRegion (ULARGE_INTEGER,
					    ULARGE_INTEGER,
					    DWORD)
        { return E_NOTIMPL; }

    HRESULT STDMETHODCALLTYPE Stat (STATSTG*, DWORD)
        { return E_NOTIMPL; }

    HRESULT STDMETHODCALLTYPE Clone (IStream**)
        { return E_NOTIMPL; }


    BEGIN_COM_MAP(MemoryStream)
        COM_INTERFACE_ENTRY(ISimpleStream)
        COM_INTERFACE_ENTRY(IStream)
    END_COM_MAP()

    };

typedef CComObject<MemoryStream> VxRWMemStream;


#endif

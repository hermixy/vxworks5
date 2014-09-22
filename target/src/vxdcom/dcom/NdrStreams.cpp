/* NdrStreams.cpp - VxDCOM NDR marshaling streams */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01s,17dec01,nel  Add include symbol for diab build.
01r,01oct01,nel  SPR#69557. Add extra padding bytes to make VT_BOOL type work.
01q,13jul01,dbs  fix up includes
01p,09feb01,nel  SPR#63068. Correct NdrStreams::reserve algorithm to allocate
                 at least required amount of memory, not just double the
                 existing amount.
01o,18oct00,nel  SPR#34927. Init some member variables.
01n,18sep00,nel  SPR#33730. Merge T2 OPC fixes into T3 branch.
01m,19jan00,nel  Modifications for Linux debug build
01l,05aug99,dbs  fix align() when buffer empty
01k,20jul99,dbs  optimise NDR reformatting
01j,20jul99,aim  removed call to print_this()
01i,09jul99,dbs  make sure reserve() is called before attempting to align the
                 stream
01h,17jun99,aim  includes vxdcom.h
01g,17jun99,dbs  change to COM_MEM_ALLOC
01f,02jun99,aim  changes for solaris build
01e,25may99,dbs  make sure stream dtors free buffer memory
01d,20may99,dbs  move NDR phase into streams
01c,18may99,dbs  no need to memcpy() when addresses are same
01b,17may99,dbs  replace realloc() with vxdcom calls
01a,12may99,dbs  created

*/

/*
  DESCRIPTION:


*/

#include <stdlib.h>
#include "NdrStreams.h"
#include "dcomProxy.h"
#include "orpcLib.h"
#include "private/comMisc.h"

#if defined (VXDCOM_PLATFORM_VXWORKS)
#include <memLib.h>
#else
#include <memory.h>
#endif

#include <stdio.h>

/* Include symbol for diab */
extern "C" int include_vxdcom_NdrStreams (void)
    {
    return 0;
    }

#if 0
static void
print_this (NdrMarshalStream* p)
    {
    byte* a = p->begin ();
    int len = p->size ();

    for (int i = 0; i < len; i++)
        {
        int b = 0;
        b = (int) *a;
        ++a;
        }
    }
#endif

//////////////////////////////////////////////////////////////////////////
//
// ndrMakeRight -- only ever called for 2, 4 or 8-byte quantities
//

inline void ndrMakeRight (size_t nb, void* pv)
    {
    char *a = (char*) pv;
    char *b = ((char*) pv) + nb - 1;
    char tmp;

    for (size_t n=0; n < (nb / 2); ++n)
        {
        tmp = *a;
        *a++ = *b;
        *b-- = tmp;
        }

    }

//////////////////////////////////////////////////////////////////////////
//

NdrMarshalStream::NdrMarshalStream
    (
    NdrPhase::Phase_t   ph,
    DREP                drep,
    byte*               pb,
    size_t              nb
    ) 
      : m_drep (drep),
        m_buffer (pb),
        m_iptr (pb),
        m_end (pb + nb),
        m_bOwnMemory (false),
        m_phase (ph),
	m_endPadding (0)
    {
    }

//////////////////////////////////////////////////////////////////////////
//

NdrMarshalStream::~NdrMarshalStream ()
    {
    if (m_bOwnMemory && m_buffer)
        delete [] m_buffer;
    }

//////////////////////////////////////////////////////////////////////////
//

NdrMarshalStream::NdrMarshalStream (NdrPhase::Phase_t ph, DREP drep)
  : m_drep (drep),
    m_buffer (0),
    m_iptr (0),
    m_end (0),
    m_bOwnMemory (true),
    m_phase (ph),
    m_endPadding (0)
    {
    }

//////////////////////////////////////////////////////////////////////////
//

size_t NdrMarshalStream::size () const
    {
    return (m_iptr - m_buffer);
    }

//////////////////////////////////////////////////////////////////////////
//

HRESULT NdrMarshalStream::align (size_t n)
    {
    // Reserve as many bytes as we are trying to align to
    reserve (n);

    // Now align the buffer-pointer
    unsigned long p1 = (unsigned long) m_buffer;
    unsigned long p2 = (unsigned long) m_iptr;
    unsigned long p3 = (unsigned long) m_iptr;
    const unsigned long mask = n - 1;
    
    // Align p2 w.r.t. p1
    p2 = p1 + (((p2 - p1) + mask) & ~mask);

    if (p3 != p2)
        memset (m_iptr, 0, p2 - p3);

    m_iptr = (byte*) p2;

    if (m_iptr > m_end)
        return E_FAIL;
    return S_OK;
    }

//////////////////////////////////////////////////////////////////////////
//
// NdrMarsalStream::reserve -- reserve space for 'nb' more bytes in 
// the stream. If necessary, the stream expands to allow space...
//

HRESULT NdrMarshalStream::reserve (size_t nb)
    {
    // There should never be 'negative space' in the buffer...
    int space = m_end - m_iptr;
    COM_ASSERT(space >= 0);

    // Make sure the space is enough to hold the requested amount, if
    // not, then we need to expand the buffer...
    if (space < (int) nb)
        {
        // Cannot expand if we don't own the memory...
        if (! m_bOwnMemory)
            return E_UNEXPECTED;
        
        // Need to expand - make it worthwhile for small values of
        // 'nb' by having a minimum expansion size of MINEXPAND...
        const size_t MINEXPAND = 512;
        size_t expansion = (nb < MINEXPAND) ? MINEXPAND : nb;

        // New total length 'len' is old total plus expansion...
        size_t len = (m_end - m_buffer) + expansion;

        // Allocate new memory...
        byte* ptmp = new byte [len];
        if (! ptmp)
            return E_OUTOFMEMORY;

        // Copy in previous contents, if there were any...
        if (m_buffer)
            memcpy (ptmp, m_buffer, m_iptr - m_buffer);

        // update members and free previous buffer
        m_end = ptmp + len;
        m_iptr = ptmp + (m_iptr - m_buffer);
        if (m_buffer)
            delete [] m_buffer;
        m_buffer = ptmp;
        }
    return S_OK;
    }

//////////////////////////////////////////////////////////////////////////
//

HRESULT NdrMarshalStream::insert
    (
    size_t              nb,
    const void*         pvData,
    bool                reformat
    )
    {
    HRESULT hr = reserve (nb);
    if (FAILED (hr))
        return hr;

    memcpy (m_iptr, pvData, nb);

    if ((m_drep != VXDCOM_DREP_LOCAL) && reformat)
        ndrMakeRight (nb, m_iptr);

    m_iptr += nb;

    // enable this for UMR checks.
    // print_this (this);

    return S_OK;
    }

//////////////////////////////////////////////////////////////////////////
//
// NdrUnmarshalStream ctor -- allocate stub-buffer (if in stub
// unmarshal phase) of same size as supplied buffer...
//

NdrUnmarshalStream::NdrUnmarshalStream
    (
    NdrPhase::Phase_t   ph,
    DREP                drep,
    byte*               pb,
    size_t              nb
    )
      : m_drep (drep),
        m_buffer (pb),
        m_optr (pb),
        m_end (pb + nb),
        m_phase (ph),
        m_stubBuffer (0),
        m_stubNext (0)
    {
    // If we are in the stub-unmarshaling pahse, its safe to allocate
    // memory for stub variables (arrays, etc) from a private heap,
    // which will automatically be reclaimed in the stream's dtor. We
    // allocate twice the stream's stub-data size, just to be safe...
    if (m_phase == NdrPhase::STUB_UNMSHL)
        {
        m_stubBuffer = new byte [2 * nb];
        m_stubNext = m_stubBuffer;
        }
    }

//////////////////////////////////////////////////////////////////////////
//

NdrUnmarshalStream::~NdrUnmarshalStream ()
    {
    if (m_stubBuffer)
        delete [] m_stubBuffer;
    }

//////////////////////////////////////////////////////////////////////////
//
// NdrUnmarshalStream::stubAlloc -- allocate 'nb' bytes from the safe
// stub memory. Since the stub-buffer is always as big as the original
// packet stub-data, then it should never run out...
//

byte* NdrUnmarshalStream::stubAlloc (size_t nb)
    {
    if (m_phase != NdrPhase::STUB_UNMSHL)
        return 0;
    
    if (m_stubBuffer == 0)
        return 0;

    // Round up m_stubNext to multiple of 4...
    m_stubNext = (byte*) (((ULONG)(m_stubNext + 3)) & ~3);
    byte* p = m_stubNext;
    m_stubNext += nb;
    return p;
    }

//////////////////////////////////////////////////////////////////////////
//
//

NdrUnmarshalStream::NdrUnmarshalStream ()
  : m_drep (0),
    m_buffer (0),
    m_optr (0),
    m_end (0),
    m_phase (NdrPhase::NOPHASE),
    m_stubBuffer (0),
        m_stubNext (0)
    {
    }

//////////////////////////////////////////////////////////////////////////
//
// assignment -- as we don't own the buffer, we just copy the pointers
//

NdrUnmarshalStream& NdrUnmarshalStream::operator=
    (
    const NdrUnmarshalStream&        rhs
    )
    {
    m_drep = rhs.m_drep;
    m_buffer = rhs.m_buffer;
    m_optr = rhs.m_optr;
    m_end = rhs.m_end;
    m_phase = rhs.m_phase;

    // set stub-buffer ptrs to NULL...
    m_stubBuffer = 0;
    m_stubNext = 0;

    return *this;
    }

//////////////////////////////////////////////////////////////////////////
//

size_t NdrUnmarshalStream::size () const
    {
    return (m_optr - m_buffer);
    }

//////////////////////////////////////////////////////////////////////////
//

HRESULT NdrUnmarshalStream::align (size_t n)
    {
    unsigned long p1 = (unsigned long) m_buffer;
    unsigned long p2 = (unsigned long) m_optr;
    unsigned long p3 = (unsigned long) m_optr;
    const unsigned long mask = n - 1;
    
    // Align p2 w.r.t. p1
    p2 = p1 + (((p2 - p1) + mask) & ~mask);
    byte* oldptr = m_optr;
    m_optr = (byte*) p2;

    // If we have gone beyond the end, fail...
    if (m_optr > m_end)
        return E_FAIL;

    // Clear the gap we have just opened to zeroes...
    if (p3 != p2)
        memset (oldptr, 0, p2 - p3);

    return S_OK;
    }

//////////////////////////////////////////////////////////////////////////
//
// NdrUnmarshalStream::extract -- extract data from the stream to the
// supplied location at 'pvData' - if pvData is NULL then don't
// actually copy any data, just adjust the stream pointers
//

HRESULT NdrUnmarshalStream::extract
    (
    size_t      nb,
    void*       pvData,
    bool        reformat
    )
    {
    if ((m_end - m_optr) < (int) nb)
        return E_FAIL;

    // only copy data out if pvData is non-NULL, and does not point
    // back into this stream's buffer, as it will when unmarshaling by
    // copying occurs in optimised cases...
    
    if (pvData && (pvData != ((void*) m_optr)))
        memcpy (pvData, m_optr, nb);
    m_optr += nb;

    if ((m_drep != VXDCOM_DREP_LOCAL) && reformat && pvData)
        ndrMakeRight (nb, pvData);

    return S_OK;
    }


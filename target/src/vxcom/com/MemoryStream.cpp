/* MemoryStream.cpp - COM/DCOM MemoryStream class implementation */

/*
modification history
--------------------
01f,17dec01,nel  Add include symbol for diab.
01e,10dec01,dbs  diab build
01d,27jun01,dbs  fix include paths and names
01c,26apr99,aim  added TRACE_CALL
01b,23apr99,dbs  add simpler mem-stream implementation using vector
01a,20apr99,dbs  created during Grand Renaming

*/

/*
  DESCRIPTION:

  MemoryStream -- 

*/

#include "TraceCall.h"
#include "MemoryStream.h"


/* Include symbol for diab */

extern "C" int include_vxcom_MemoryStream (void)
    {
    return 0;
    }

////////////////////////////////////////////////////////////////////////////
//
// IID_ISimpleStream is {81D2333D-F973-11d2-A800-00C04F68A5B5}
//

const IID IID_ISimpleStream =
    { 0x81d2333d, 0xf973, 0x11d2, 
    { 0xa8, 0x0, 0x0, 0xc0, 0x4f, 0x68, 0xa5, 0xb5 } };

////////////////////////////////////////////////////////////////////////////
//

MemoryStream::MemoryStream () : m_curr (0)
    {
    TRACE_CALL;
    }

////////////////////////////////////////////////////////////////////////////
//

MemoryStream::~MemoryStream ()
    {
    TRACE_CALL;
    }

////////////////////////////////////////////////////////////////////////////
//
// MemoryStream::extract -- reads 'nb' bytes from the stream to
// address 'pv' and returns the number of bytes read. This number may
// legitimately be smaller than that requested.
//

ULONG MemoryStream::extract (void* pv, ULONG nb)
    {
    TRACE_CALL;
    // Adjust size if there aren't enough bytes in the stream
    if ((m_vector.size () - m_curr) < nb)
	nb = m_vector.size () - m_curr;

    // copy 'nb' bytes from the stream to 'pv'
#ifdef __DCC__
    char* dest = reinterpret_cast<char*> (pv);
    ITERATOR i = m_vector.begin () + m_curr;
    ITERATOR j = i + nb;
    copy (i, j, dest);
#else
    memcpy (pv, m_vector.begin () + m_curr, nb);
#endif
    

    // adjust the current location
    m_curr += nb;

    // return the number of bytes read
    return nb;
    }

////////////////////////////////////////////////////////////////////////////
//
// MemoryStream::insert -- writes 'nb' bytes into the stream (at the
// current location) from the data pointed to by 'pv' and returns the
// number of bytes written.
//

ULONG MemoryStream::insert (const void* pv, ULONG nb)
    {
    TRACE_CALL;
    // ensure the array is big enough
    m_vector.reserve (m_curr + nb);

    // insert bytes from 'pv' into the stream
    m_vector.insert (m_vector.begin () + m_curr,
		     (char*)pv,
		     ((char*)pv) + nb);

    // update current position
    m_curr += nb;

    // return number of bytes written
    return nb;
    }

////////////////////////////////////////////////////////////////////////////
//
// MemoryStream::locationSet -- move the current location of the
// stream to the given position 'pos' and return the current position
// after the move. If the requested position is outside the current
// stream, then the location is not changed, and (-1) is returned.
//

ULONG MemoryStream::locationSet (ULONG pos)
    {
    TRACE_CALL;
    if (pos > m_vector.size ())
	return (ULONG) -1;
    return (m_curr = pos);
    }

////////////////////////////////////////////////////////////////////////////
//
// MemoryStream::locationGet -- returns the current location of the
// stream, i.e. the seek pointer.
//

ULONG MemoryStream::locationGet ()
    {
    TRACE_CALL;
    return m_curr;
    }

////////////////////////////////////////////////////////////////////////////
//
// MemoryStream::locationGet -- returns the current size of the
// stream.
//

ULONG MemoryStream::size ()
    {
    TRACE_CALL;
    return m_vector.size ();
    }


////////////////////////////////////////////////////////////////////////////
//

HRESULT MemoryStream::Read
    (
    void *	pv,
    ULONG	cb,
    ULONG *	pcbRead
    )
    {
    TRACE_CALL;
    ULONG nr = extract (pv, cb);
    if (pcbRead)
	*pcbRead = nr;
    return (nr == cb) ? S_OK : S_FALSE;
    }

////////////////////////////////////////////////////////////////////////////
//

HRESULT MemoryStream::Write
    (
    const void *	pv,
    ULONG		cb,
    ULONG *		pcbWritten
    )
    {
    TRACE_CALL;
    ULONG nw = insert (pv, cb);
    if (pcbWritten)
	*pcbWritten = nw;
    return (nw == cb) ? S_OK : S_FALSE;
    }

////////////////////////////////////////////////////////////////////////////
//

HRESULT MemoryStream::Seek
    (
    LARGE_INTEGER	dlibMove,
    DWORD		dwOrigin,
    ULARGE_INTEGER *	plibNewPosition
    )
    {
    TRACE_CALL;
    HRESULT		hr = S_OK;
    ULONG		pos;
    
    switch (dwOrigin)
        {
        case STREAM_SEEK_SET:
            // offset from beginning
	    pos = locationSet ((ULONG) dlibMove);
            if (plibNewPosition)
                *plibNewPosition = m_curr;
            break;

        case STREAM_SEEK_CUR:
            // offset from current
	    pos = locationSet (m_curr + (int) dlibMove);
            if (plibNewPosition)
                *plibNewPosition = m_curr;
            break;

        case STREAM_SEEK_END:
            // offset from end
	    pos = locationSet (size() - dlibMove);
            if (plibNewPosition)
                *plibNewPosition = m_curr;
            break;
        }
    return hr;
    }


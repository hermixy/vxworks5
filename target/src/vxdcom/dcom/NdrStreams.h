/* NdrStreams.h - ORPC NDR (un)marshaling streams */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*

modification history
--------------------
01g,01oct01,nel  SPR#69557. Add extra padding bytes to make VT_BOOL type work.
01f,18sep00,nel  SPR#33730. Merge T2 OPC fixes into T3 branch.
01e,25may99,dbs  make sure stream dtors free buffer memory
01d,20may99,dbs  move NDR phase into streams
01c,18may99,dbs  add marshaling phase accessor methods
01b,18may99,dbs  add proxy/stub marshaling phase to NDR-streams
01a,12may99,dbs  created

*/

#ifndef __INCNdrStreams_h
#define __INCNdrStreams_h

#include "dcomProxy.h"

//////////////////////////////////////////////////////////////////////////
//
// NdrPhase -- the current marshaling phase a stream is involved in
//

struct NdrPhase
    {
    enum Phase_t
	{
	NOPHASE=0,
	PROXY_MSHL,
	STUB_UNMSHL,
	STUB_MSHL,
	PROXY_UNMSHL
	};
    };
    
//////////////////////////////////////////////////////////////////////////
//
// NdrMarshalStream -- provides a stream into which any of the basic
// C/C++ data-types (of size 1, 2, 4 or 8 bytes) can be marshaled,
// using the insert() method. It provides 2 constructors - the first
// uses a fixed-size, user-supplied buffer, and only allows data to be
// marshaled into that buffer, until it is full. The second
// constructor takes only the data-representation argument, and causes
// the stream to internally allocate memory as required, so it can
// cope with variable-sized marshaling easily.
//

class NdrMarshalStream
    {
  public:

    // ctor -- user-supplied fixed-size memory buffer
    NdrMarshalStream (NdrPhase::Phase_t ph, DREP drep, byte*, size_t);

    // ctor -- internally-managed expanding buffer
    NdrMarshalStream (NdrPhase::Phase_t ph, DREP drep);


    // dtor
    ~NdrMarshalStream ();
    
    HRESULT align (size_t);
    HRESULT insert (size_t wordSize, const void* pvData, bool reformat);
    size_t  size () const;
    DREP    drep () const { return m_drep; }

    byte*   begin () const { return m_buffer; }
    byte*   end () const { return m_iptr; }

    NdrPhase::Phase_t phaseGet () const { return m_phase; }

    void addEndPadding (DWORD amount) { m_endPadding += amount; };
    DWORD getEndPadding () const { return m_endPadding; };
    
  private:
    DREP		m_drep;		// data representation
    byte*		m_buffer;	// base of buffer
    byte*		m_iptr;		// insert pointer
    byte*		m_end;		// end of buffer
    bool		m_bOwnMemory;	// does it own the buffer mem?
    NdrPhase::Phase_t	m_phase;	// marshaling phase
    DWORD		m_endPadding;	// extra padding to be added to 
    					// end of stream.
    HRESULT reserve (size_t);

    };

//////////////////////////////////////////////////////////////////////////
//
// NdrUnmarshalStream -- provides a stream from which data can be
// unmarshaled into any of the standard data sizes (1, 2, 4 or 8 byte
// quantities) via the extract() method. It is constructed with the
// address and length of a buffer holding the data to be unmarshaled,
// and a data-representation identifier indicating that buffer's NDR
// encoding.
//

class NdrUnmarshalStream
    {
  public:

    NdrUnmarshalStream (NdrPhase::Phase_t ph, DREP drep, byte*, size_t);
    NdrUnmarshalStream ();
    ~NdrUnmarshalStream ();

    NdrUnmarshalStream& operator= (const NdrUnmarshalStream& rhs);

    HRESULT align (size_t);
    HRESULT extract (size_t wordSize, void* pvData, bool reformat);
    size_t  size () const;
    DREP    drep () const { return m_drep; }

    byte*   begin () const { return m_buffer; }
    byte*   end () const { return m_end; }

    const byte*   curr () const { return m_optr; }

    NdrPhase::Phase_t phaseGet () const { return m_phase; }

    byte*   stubAlloc (size_t n);

  private:
    DREP		m_drep;		// data representation
    byte*		m_buffer;	// base of buffer
    byte*		m_optr;		// extract pointer
    byte*		m_end;		// end of buffer
    NdrPhase::Phase_t	m_phase;	// marshaling phase
    byte*		m_stubBuffer;	// stub-only buffer
    byte*		m_stubNext;	// ptr into that buffer
    };


#endif


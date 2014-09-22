/* HandleSet */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01l,17dec01,nel  Add include symbol for diab.
01k,31jul01,dbs  fix operation of isSet on solaris
01j,13jul01,dbs  fix up includes
01i,19aug99,aim  change assert to VXDCOM_ASSERT
01h,18aug99,aim  removed inline for SIMNT build
01g,11aug99,aim  ostream operator prints N handles
01f,30jul99,aim  removed debug
01e,21jul99,aim  quantify tweaks
01d,20jul99,aim  added inline statements
01c,18jun99,aim  added copy ctor and operator=
01b,04jun99,aim  removed debug
01a,07may99,aim  created
*/

#include "HandleSet.h"
#include "TraceCall.h"
#include "private/comMisc.h"

/* Include symbol for diab */
extern "C" int include_vxdcom_HandleSet (void)
    {
    return 0;
    }

HandleSet::HandleSet ()
    {
    TRACE_CALL;
    reset ();
    }

HandleSet::HandleSet (REACTOR_HANDLE_SET_TYPE mask)
  : m_count (0),
    m_maxHandle (INVALID_REACTOR_HANDLE),
    m_mask (mask)
    {
    TRACE_CALL;

    REACTOR_HANDLE h;

    for (h = 0; h < maxSize (); ++h)
	{
	if (FD_ISSET (h, &mask))
	    {
	    ++m_count;
	    if (h > m_maxHandle)
		m_maxHandle = h;
	    }
	}
    }

HandleSet::HandleSet (const HandleSet& other)
  : m_count (other.m_count),
    m_maxHandle (other.m_maxHandle),
    m_mask (other.m_mask)
    {
    }

HandleSet& HandleSet::operator= (const HandleSet& rhs)
    {
    if (this != &rhs)
	{
	m_count = rhs.m_count;
	m_maxHandle = rhs.m_maxHandle;
	(void) ::memcpy (&m_mask, &rhs.m_mask, sizeof (m_mask));
	}
    return *this;
    }


HandleSet::~HandleSet ()
    {
    TRACE_CALL;
    }

void HandleSet::reset ()
    {
    TRACE_CALL;
    m_count = 0;
    m_maxHandle = INVALID_REACTOR_HANDLE;
    FD_ZERO (&m_mask);
    }

void HandleSet::set (REACTOR_HANDLE h)
    {
    TRACE_CALL;

    if (h != INVALID_REACTOR_HANDLE && !isSet (h))
	{
	COM_ASSERT (h < HandleSet::maxSize ());

	++m_count;

	FD_SET (h, &m_mask);

	if (h > m_maxHandle)
	    m_maxHandle = h;
	}
    }

bool HandleSet::isSet (REACTOR_HANDLE h) const
    {
    TRACE_CALL;

    if (h == INVALID_REACTOR_HANDLE)
	return false;

    int fd_is_set = FD_ISSET (h, &m_mask);
    
    bool x = (fd_is_set != 0) ? true : false;
#if 0
    if (x)
        {
        cout << "FD_ISSET returned " << fd_is_set
             << " Bit " << h << " is 1 in set 0x" << hex
             << m_mask.fds_bits[0] << dec << endl;
        }
    else
        {
        cout << "FD_ISSET returned " << fd_is_set
             << " Bit " << h << " is 0 in set 0x" << hex
             << m_mask.fds_bits[0] << dec << endl;
        }
#endif
    return x;
    }

void HandleSet::clr (REACTOR_HANDLE h)
    {
    TRACE_CALL;

    if (h != INVALID_REACTOR_HANDLE && isSet (h))
	{
	--m_count;

	FD_CLR (h, &m_mask);

	if (m_count)
	    maxHandleSet ();
	else
	    m_maxHandle = INVALID_REACTOR_HANDLE;
	}
    }

REACTOR_HANDLE HandleSet::maxHandleSet ()
    {
    m_maxHandle = INVALID_REACTOR_HANDLE;

    REACTOR_HANDLE h;
    HandleSetIterator hsIter (*this);

    while ((h = hsIter ()) != INVALID_REACTOR_HANDLE)
	m_maxHandle = h;

    return m_maxHandle;
    }

HandleSet::operator REACTOR_HANDLE_SET_TYPE* ()
    {
    TRACE_CALL;
    return &m_mask;
    }

int HandleSet::count () const
    {
    TRACE_CALL;
    return m_count;
    }

REACTOR_HANDLE HandleSet::maxHandle () const
    {
    TRACE_CALL;
    return m_maxHandle;
    }

int HandleSet::maxSize ()
   {
   return FD_SETSIZE;
   }

int HandleSet::sync (REACTOR_HANDLE n)
    {
    // DO NOT use HandleSetIterator here.  It won't work.
 
    if (n == INVALID_REACTOR_HANDLE)
	n = maxSize ();

    REACTOR_HANDLE h;

    // reset
    m_count = 0;
    m_maxHandle = INVALID_REACTOR_HANDLE;

    COM_ASSERT (n < maxSize ());

    for (h = 0; h < n; ++h)
	if (isSet (h))
	    {
	    ++m_count;
	    if (h > m_maxHandle)
		m_maxHandle = h;
	    }

    return m_maxHandle;
    }

ostream&
operator<< (ostream& os, const HandleSet& hs)
    {
    int count = hs.count ();

    if (count > 0)
	{
	REACTOR_HANDLE h;
	HandleSetIterator hsIter (hs);

	os << "[N=" << count << "] ";
    
	while ((h = hsIter ()) != INVALID_REACTOR_HANDLE)
	    if (hsIter.last ())
		os << h;
	    else
		os << h << ",";
	}

    return os;
    }

HandleSetIterator::HandleSetIterator (const HandleSet& hs)
  : m_handleSet (hs),
    m_count (hs.count ()),
    m_index (0)
    {
    TRACE_CALL;
    }

REACTOR_HANDLE HandleSetIterator::end () const
    {
    return INVALID_REACTOR_HANDLE;
    }

bool HandleSetIterator::last () const
    {
    return m_count == 0;
    }

REACTOR_HANDLE HandleSetIterator::operator() ()
    {
    for (; m_count > 0; ++m_index)
	if (m_handleSet.isSet (m_index))
	    {
	    --m_count;
	    return m_index++;
	    }

    // mark end of iter
    return INVALID_REACTOR_HANDLE;
    }

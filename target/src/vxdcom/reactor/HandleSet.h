/* HandleSet - Encapsulates a set of fd's for select/poll */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,18aug99,aim  removed inline for SIMNT build
01d,21jul99,aim  quantify tweaks
01c,20jul99,aim  added inline statements
01b,18jun99,aim  added copy ctor and operator=
01a,07may99,aim  created
*/

#ifndef __INCHandleSet_h
#define __INCHandleSet_h

#include "ReactorTypes.h"
#include "iostream"

class HandleSet
    {
  public:

    virtual ~HandleSet();

    HandleSet ();
    HandleSet (REACTOR_HANDLE_SET_TYPE mask);

    HandleSet (const HandleSet&);
    HandleSet& operator= (const HandleSet&);

    int count () const;
    int sync (REACTOR_HANDLE = maxSize ());
    void reset ();
    bool isSet (REACTOR_HANDLE h) const;
    void clr (REACTOR_HANDLE h);
    void set (REACTOR_HANDLE h);
    REACTOR_HANDLE maxHandle () const;
    operator REACTOR_HANDLE_SET_TYPE* ();

    friend class HandleSetIterator;

    friend ostream& operator<<
	(
	ostream&,
	const HandleSet&
	);

    static int maxSize ();

  private:

    REACTOR_HANDLE maxHandleSet ();
    
    int				m_count;
    REACTOR_HANDLE		m_maxHandle;
    REACTOR_HANDLE_SET_TYPE	m_mask;
    };

class HandleSetIterator
    {
  public:
    HandleSetIterator (const HandleSet &hs);

    REACTOR_HANDLE end() const;
    bool last() const;

    REACTOR_HANDLE operator() ();

    // "Next" operator.  Returns the next <REACTOR_HANDLE> in the
    // <HandleSet> up to <HandleSet->maxHandle>.  When all the Handles
    // have been seen returns <INVALID_REACTOR_HANDLE>.

  private:

    const HandleSet&	m_handleSet;
    int			m_count;
    int			m_index;
    int			m_maxHandle;

    // unsupported
    HandleSetIterator ();
    HandleSetIterator (const HandleSetIterator&);
    HandleSetIterator& operator= (HandleSetIterator&);
    };

#endif // __INCHandleSet_h

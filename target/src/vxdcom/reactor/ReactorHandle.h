/* ReactorHandle */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01b,28jun99,aim  added handleValid
01a,10may99,aim  created
*/

#include <ReactorTypes.h>
#include <iostream>

class ReactorHandle
    {
  public:
    virtual ~ReactorHandle ();

    virtual REACTOR_HANDLE handleGet () const;
    virtual REACTOR_HANDLE handleSet (REACTOR_HANDLE);

    virtual bool handleInvalid () const;
    virtual bool handleIsValid () const;

    friend ostream& operator<< (ostream& os, const ReactorHandle&);

  protected:
    ReactorHandle ();		// ensure ReactorHandle is an ABC.

  private:
    REACTOR_HANDLE m_reactorHandle;
    };

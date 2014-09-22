/* SockAddr - Socket address encapsulation */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,15jun99,aim  make hostAddrGet() and family() reentrant
01b,05jun99,aim  added clone method
01a,10may99,aim  created
*/

#ifndef __INCSockAddr_h
#define __INCSockAddr_h

#include "ReactorTypes.h"
#include <iostream>

class SockAddr
    {
  public:

    virtual ~SockAddr ();
    virtual int size () const = 0;
    virtual int family () const = 0;
    virtual int portGet () const = 0;
    virtual int hostAddrGet (char* buf, int len) const = 0;
    virtual int familyNameGet (char* buf, int len) const = 0;
    virtual sockaddr* addr () = 0;
    virtual operator sockaddr* () = 0;
    virtual int clone (sockaddr*&) const = 0;

    friend ostream& operator<< (ostream& os, const SockAddr&);

  protected:

    SockAddr ();
    };

#endif // __INCSockAddr_h

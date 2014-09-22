/* INETSockAddr */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01d,15jul99,dbs  add equality op
01c,15jun99,aim  added hostNameGet
01b,05jun99,aim  added clone method
01a,11May99,aim  created
*/

#ifndef __INCINETSockAddr_h
#define __INCINETSockAddr_h

#include "SockAddr.h"

class INETSockAddr : public SockAddr, public sockaddr_in
    {
  public:
    virtual ~INETSockAddr ();

    INETSockAddr ();
    // INADDR_ANY, and port 0.

    INETSockAddr (int portNumber);
    INETSockAddr (const char* hostname, int portNumber = 0);
    INETSockAddr (unsigned long hostaddr, int portNumber = 0);

    virtual int size () const;
    virtual int family () const;
    virtual int portGet () const;
    virtual int hostAddrGet (char* buf, int len) const;
    virtual int familyNameGet (char* buf, int len) const;
    virtual sockaddr* addr ();
    virtual operator sockaddr* ();
    virtual int clone (sockaddr*& buf) const;

    virtual sockaddr_in* addr_in ();
    virtual operator sockaddr_in* ();

    virtual const sockaddr_in* addr_in () const;
    virtual operator const sockaddr_in* () const;

    bool operator == (const INETSockAddr& other) const;
    
  private:

    int setAddr (const char* host_name);
    };

#endif // __INCINETSockAddr_h

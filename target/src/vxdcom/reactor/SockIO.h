/* SockIO */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01a,10may99,aim  created
*/

#ifndef __INCSockIO_h
#define __INCSockIO_h

#include <ReactorTypes.h>
#include <SockEP.h>

class SockIO : public SockEP
    {
  public:
    virtual ~SockIO ();

    SockIO ();

    virtual size_t send (const void *buf,
			 size_t n,
			 int flags) const;
    // Send an <n> byte buffer to the connected socket (uses
    // <send(3n)>).

    virtual size_t recv (void *buf,
			 size_t n,
			 int flags) const;
    // Recv an <n> byte buffer from the connected socket (uses
    // <recv(3n)>).

    virtual size_t send (const void *buf,
			 size_t n) const;
    // Send an <n> byte buffer to the connected socket (uses
    // <write(2)>).

    virtual size_t recv (void *buf,
			 size_t n) const;
    // Recv an <n> byte buffer from the connected socket (uses
    // <read(2)>).
    };

#endif // __INCSockIO_h

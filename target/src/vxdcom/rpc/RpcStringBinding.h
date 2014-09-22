/* RpcStringBinding - A DCE/RPC endpoints symbolic address */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01g,13jul01,dbs  fix up includes
01f,13mar01,nel  SPR#35873 - add extra method to check to see if an address
                 string can be resolved.
01e,16jul99,aim  added formatNoPortNumber
01d,15jul99,dbs  add isLocalhost() method
01c,08jul99,dbs  add equality operator
01b,07jul99,aim  added strdup; apparently it doesn't exist on the target??
01a,06jul99,aim  created
*/

#ifndef __INCRpcStringBinding_h
#define __INCRpcStringBinding_h

#include <iostream>
#include "dcomLib.h"
#include "comLib.h"
#include "SockAddr.h"
#include "orpc.h"

class RpcStringBinding
    {
  public:
    enum { ncacn_ip_tcp = 7 };

    virtual ~RpcStringBinding ();

    RpcStringBinding ();

    RpcStringBinding
        (
        DUALSTRINGARRAY*        pdsa,
        int			portNumber = -1
        );

    RpcStringBinding
	(
	const char*	binding,
	PROTSEQ         protSeq = ncacn_ip_tcp
	);

    RpcStringBinding
	(
	const OLECHAR*	binding,
	PROTSEQ         protSeq = ncacn_ip_tcp
	);

    RpcStringBinding
	(
	const char*	binding, 
	PROTSEQ         protSeq,
	int		portNumber
	);

    RpcStringBinding
	(
	const OLECHAR*	binding, 
	PROTSEQ         protSeq,
	int		portNumber
	);

    RpcStringBinding
	(
	const SockAddr& sockAddr,
	PROTSEQ		protSeq = ncacn_ip_tcp
	);

    RpcStringBinding (const RpcStringBinding& other);
    RpcStringBinding& operator= (const RpcStringBinding& rhs);

    bool isValid () const;

    bool isLocalhost () const;

    bool resolve () const;
    
    const char* formatted (bool includePortNumber = true) const;
    // returns: protseq<host-addr>[<port>]

    // Accessors
    const char* ipAddress () const;
    int portNumber () const;
    PROTSEQ protocolSequence () const;

    int format
	(
	BSTR*	bstr,
	bool	includePortNumber = true
	) const;

    bool operator== (const RpcStringBinding& sb) const;
    bool operator< (const RpcStringBinding& sb) const;
    
    friend ostream& operator<< (ostream& os, const RpcStringBinding&);

  private:
    
    char*	  m_ipAddress;
    int		  m_portNumber;
    PROTSEQ	  m_protocolSeq;
    mutable char* m_strFormat;
    mutable char* m_strFormatNoPortNumber;

    int initialiseFromString (const char* binding);

    char* copyString (const char*) const;
    void  delString (char*&);
    };

#endif // __INCRpcStringBinding_h

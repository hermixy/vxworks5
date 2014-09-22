/* RpcDispatcher */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01f,20sep00,nel  Add changes made in T2 since branch.
01e,28feb00,dbs  use correct ThreadOS method to get/set priority
01d,09jul99,dbs  use new filenames
01c,25jun99,dbs  add channel ID to stub msg
01b,08jun99,aim  rework
01a,27may99,aim  created
*/

#ifndef __INCRpcDispatcher_h
#define __INCRpcDispatcher_h

#include "RpcDispatchTable.h"
#include "RpcPdu.h"
#include "orpc.h"

class RpcDispatcher
    {
  public:
    virtual ~RpcDispatcher ();

    RpcDispatcher (RpcDispatchTable* = 0);

    bool supportsInterface (REFIID riid);
    int dispatch (const RpcPdu& request, RpcPdu& reply, int channelId, REFIID iid);

    bool priorityModify (REFCLSID clsid, const ORPCTHIS& orpcThis, int *pNewPriority);
    void priorityRestore (int origPriority);
    
  private:
    RpcDispatchTable* m_dispatchTable;

    // unsupported
    RpcDispatcher (const RpcDispatcher& other);
    RpcDispatcher& operator= (const RpcDispatcher& rhs);
    };

#endif // __INCRpcDispatcher_h

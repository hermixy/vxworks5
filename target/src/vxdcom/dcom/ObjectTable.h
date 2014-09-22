/* ObjectTable.h - VxDCOM ObjectTable class definition */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*

modification history
--------------------
01o,13jul01,dbs  fix up includes
01n,05mar01,nel  SPR#62130 - implement CoDisconnectObject.
01m,20sep00,nel  Add changes made in T2 since branch.
01l,17aug99,aim  removed copy ctor and operator= from ObjectTable
01k,27jul99,drm  Returning CLSID from interfaceInfoGet()
01j,26jul99,dbs  dont save p/s clsid any longer
01i,16jul99,dbs  convert map/set with long long key to use new macros
01h,29jun99,dbs  remove const-ness warnings
01g,29jun99,dbs  remove ifdef DEBUG around ostream operators
01f,29jun99,dbs  make StdStub a member of ObjectTableEntry
01e,17jun99,aim  uses new SCM
01d,10jun99,dbs  remove inclusion of comNew.h
01c,03jun99,dbs  no return value from mutex lock
01b,03jun99,dbs  fix includes for VxMutex class
01a,02jun99,dbs  created

*/


#ifndef __INCObjectTable_h
#define __INCObjectTable_h

#include "dcomLib.h"			// basic types, structs, etc
#include "RpcDispatchTable.h"		// for table class
#include "MemoryStream.h"		// for IStream impl
#include "private/comMisc.h"            // for VxMutex class
#include "StdStub.h"                    // StdStub class


////////////////////////////////////////////////////////////////////////////
//
// ObjectTableEntry - each object exported by an Object Exporter
// requires a record of this form. It records the useful information
// relating to the object, and allows incoming ORPCs to be dispatched
// against the object.
//
// The table-entry always adds a reference to the stream holding the
// marshaled interface ptr, and releases it upon destruction. It is up
// to the user (usually CoMarshalInterface()) to determine whether an
// extra ref is needed for 'strong' marshaling.
//

#define VXDCOM_PING_TIMEOUT (120 * 3)

struct ObjectTableEntry
    {
    IStream*    pstmMarshaledItfPtr;	// stream with marshaled itf ptr
    VxStdStub	stdStub;                // std-mshl stub object
    OID         oid;			// OID of marshaled object
    CLSID       clsid;			// CLSID of object
    IUnknown*	punkCF;			// class-factory ptr
    DWORD	dwRegToken;		// factory reg token
    long	pingTimeout;		// ping timeout (secs)
    bool	noPing;			// ping defeat mechanism
    
    ObjectTableEntry ();
    ObjectTableEntry (OID o, REFCLSID psc, IStream* pstm, bool);
    ~ObjectTableEntry ();

    void printOn (ostream&) const;

  private:

    // unsupported
    ObjectTableEntry& operator= (const ObjectTableEntry&);
    ObjectTableEntry (const ObjectTableEntry&);
    };

////////////////////////////////////////////////////////////////////////////
//
// VxObjectTable -- this class implements a type of RpcDispatchTable
// which records all exported objects owned by an Object Exporter.
//

class VxObjectTable : public RpcDispatchTable
    {
    typedef STL_MAP_LL(ObjectTableEntry*) OBJECTMAP;

  public:

    typedef OBJECTMAP::iterator iterator;
    typedef OBJECTMAP::const_iterator const_iterator;
    
    VxObjectTable () {}
    virtual ~VxObjectTable () {}

    // methods inherited from RpcDispatchTable base-class, to look-up
    // interface IDs, and interface pointers
    bool    supportsInterface (REFIID);
    HRESULT interfaceInfoGet (REFIID riid, REFIPID, ULONG, IUnknown**, PFN_ORPC_STUB*, CLSID &);

    // methods to manage exported objects
    ObjectTableEntry* objectRegister (IStream*, REFCLSID, bool noPing);
    ObjectTableEntry* objectFindByOid (OID o);
    ObjectTableEntry* objectFindByStream (IStream* pstm);
    ObjectTableEntry* objectFindByToken (DWORD dwToken);
    ObjectTableEntry* objectFindByIUnknown (IUnknown* punk);
    bool              objectUnregister (OID o);

    iterator begin () { return m_objectTable.begin (); }
    iterator end () { return m_objectTable.end (); }

    const_iterator begin () const { return m_objectTable.begin (); }
    const_iterator end () const { return m_objectTable.end (); }

    void lock () { m_mutex.lock (); }
    void unlock () { m_mutex.unlock (); }

    void printOn (ostream&) const;

  private:

    VxMutex		m_mutex;	// for task safety
    OBJECTMAP		m_objectTable;	// OID -> ObjectTableEntry

    };

inline ostream& operator << (ostream& os, const ObjectTableEntry& ote)
    {
    ote.printOn (os);
    return os;
    }

inline ostream& operator << (ostream& os, const VxObjectTable& ot)
    {
    ot.printOn (os);
    return os;
    }

#endif



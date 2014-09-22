/* comMisc.h - COM miscellaneous support functions */

/* Copyright (c) 1999-2001 Wind River Systems, Inc. */

/*

modification history
--------------------
01q,08nov01,nel  SPR#71582. Type case error return in vxcom_mbstowcs to
                 size_t.
01p,30oct01,nel  Correct problem with buffer restricted strings in
                 vxcom_wcstombs.
01o,30oct01,nel  SPR#71311. vxcom_wcstombs shouldn't count the terminator as a
                 converted character.
01n,18jul01,dbs  add ostream operator for GUIDs
01m,16jul01,dbs  add copy-ctor and op= to sync classes
01l,13jul01,dbs  remove unwanted includes
01k,28jun01,dbs  move VxMutex to private header
01j,21jun01,dbs  convert to use VxWorks semaphores
01i,08feb01,nel  SPR#63885. SAFEARRAYs added. 
01h,29jul99,aim  added mutex accessor to VxMutex
01g,17jun99,dbs  move COM_MEM_ALLOC to vxdcom.h
01f,08jun99,dbs  add CritSec to assist with use of Mutex
01e,03jun99,dbs  make mutex lock return void
01d,03jun99,dbs  add VxMutex class
01c,04may99,drm  added function prototype for CLSIDFromAscii()
01b,28apr99,dbs  add mem-alloc funcs
01a,19apr99,dbs  created

*/

/*
DESCRIPTION:

This file defines private data structures and utility routines used by
the VxWorks COM implementation.
  
*/

#ifndef __INCcomMisc_h
#define __INCcomMisc_h

#include <vxWorks.h>
#include "sysLib.h"
#include "comLib.h"

#ifdef __cplusplus

#include <iostream>

/* ostream operator to allow GUIDs to be easily streamed to text */
inline ostream& operator<< (ostream& os, const GUID& g)
    {
    os << ::vxcomGUID2String (g);
    return os;
    }

#define MS2TICKS(ms) (((ms) * sysClkRateGet()) / 1000)

#define TICKS2MS(tk) ((1000 * tk) / sysClkRateGet())

void comAssertFailed (const char *, const char *, int);

#define COM_ASSERT(expr) \
    if (!(expr)) { comAssertFailed(#expr, __FILE__, __LINE__); }


#define	DELZERO(X) (void)((delete X, X = 0), X)

#define DECLARE_IUNKNOWN_METHODS                \
        STDMETHOD_(ULONG, AddRef) ();           \
        STDMETHOD_(ULONG, Release) ();          \
        STDMETHOD(QueryInterface) (REFIID riid, void** ppv);

////////////////////////////////////////////////////////////////////////////
//
// Various inline wide/narrow string conversion routines...
//
////////////////////////////////////////////////////////////////////////////

inline size_t vxcom_wcstombs
    (
    char*           pAsciiString,
    const OLECHAR*  pWideString,
    size_t          maxChars
    )
    {
    // Check input parameters.
    if (!pAsciiString)
	return (size_t)-1;
    if (!pWideString)
	return (size_t)-1;
    // Convert to plain-char...
    size_t  n;
    for (n=0; n < maxChars; ++n)
        {
        *pAsciiString++ = (char) *pWideString++;
        if (*pWideString == 0)
	    {
	    n++;
	    break;
	    }
        }
    *pAsciiString = 0;
    return n;
    }

////////////////////////////////////////////////////////////////////////////

inline size_t vxcom_mbstowcs
    (
    OLECHAR*        pWideString,
    const char*     pAsciiString,
    size_t          maxChars
    )
    {
    // Check input parameters.
    if (!pAsciiString)
	return (size_t)-1;
    if (!pWideString)
	return (size_t)-1;
    // Convert to wide-char...
    size_t  n;
    for (n=0; n < maxChars; ++n)
        {
        *pWideString++ = (OLECHAR) *pAsciiString++;
        if (*pAsciiString == 0)
            break;
        }
    *pWideString = 0;
    return n;
    }

////////////////////////////////////////////////////////////////////////////

inline OLECHAR* vxcom_wcscpy (OLECHAR* dst, const OLECHAR* src)
    {
    OLECHAR* p = dst;
    while ((*p++ = *src++))
        {}
    return dst;
    }

////////////////////////////////////////////////////////////////////////////

inline size_t vxcom_wcslen (const OLECHAR* str)
    {
    size_t n=0;
    while (*str++)
        ++n;
    return n;
    }


//////////////////////////////////////////////////////////////////////////
//
// VxMutex - a simple mutex semaphore wrapper class
//
//////////////////////////////////////////////////////////////////////////

class VxMutex
    {
  public:

    enum { INFINITE_WAIT=0xFFFFFFFF };
    
    VxMutex ();
    ~VxMutex ();

    void lock (unsigned long millisecsWait=INFINITE_WAIT) const;
    void unlock () const;

    SEM_ID mutex () const { return m_mutex; };

  private:

    // copy/assignment -- not allowed
    VxMutex (const VxMutex&);
    VxMutex& operator= (const VxMutex&);
    
    SEM_ID m_mutex;
    };

//////////////////////////////////////////////////////////////////////////
//
// VxCondVar - a simple condition-variable wrapper class
//
//////////////////////////////////////////////////////////////////////////

class VxCondVar
    {
  public:

    VxCondVar ();
    ~VxCondVar ();
    void wait (const VxMutex& mtx);
    void signal () const;

  private:

    // copy/assignment -- not allowed
    VxCondVar (const VxCondVar&);
    VxCondVar& operator= (const VxCondVar&);
    
    SEM_ID      m_condvar;
    };

//////////////////////////////////////////////////////////////////////////
//
// VxCritSec -- a simple critical-section implementation
//
//////////////////////////////////////////////////////////////////////////

class VxCritSec
    {
  public:
    VxCritSec (VxMutex& mtx) : m_mutex (mtx)
	{ m_mutex.lock (); }

    ~VxCritSec ()
	{ m_mutex.unlock (); }

  private:

    // copy/assignment -- not allowed
    VxCritSec (const VxCritSec&);
    VxCritSec& operator= (const VxCritSec&);
    
    VxMutex&	m_mutex;
    };


//////////////////////////////////////////////////////////////////////////
//
// VxGenericListElement - Template class for a generic list element in a
// generic list.
//

template <class T> class VxGenericListElement : public T
    {
    public:

    VxGenericListElement () : T () { m_nxt = NULL; m_prev = NULL; }
    ~VxGenericListElement () {}

    VxGenericListElement<T> * getNext () 
        { 
        return m_nxt; 
        }
    VxGenericListElement<T> * getPrev () 
        { 
        return m_prev; 
        }
    void setNext (const VxGenericListElement<T> * ptr) 
        { 
        m_nxt = const_cast<VxGenericListElement<T> *>(ptr); 
        }
    void setPrev (const VxGenericListElement<T> * ptr) 
        { 
        m_prev = const_cast<VxGenericListElement<T> *>(ptr); 
        }

    private:

    VxGenericListElement<T> * m_prev;
    VxGenericListElement<T> * m_nxt;


    };

//////////////////////////////////////////////////////////////////////////
//
// VxGenericList - Template class to implement a generic linked list 
// class.
//

template <class E, class K, class Mx> class VxGenericList
    {
    public:

    VxGenericList ()
        {
        m_head.setNext (NULL);
        m_current = NULL;
        }

    virtual ~VxGenericList () 
       {
       }

    void pushHead (E * ptr)
        {
        m_mutex.lock ();
        ptr->setPrev (&m_head);
        ptr->setNext (m_head.getNext ());
        if (m_head.getNext () != NULL)
            {
            m_head.getNext ()->setPrev (const_cast<const E *>(ptr));
            }
        m_head.setNext (const_cast<const E *>(ptr));
        m_mutex.unlock ();
        }

    E * popHead ()
        {
        E *     ptr = NULL;
        m_mutex.lock ();
        if (m_head.getNext () != NULL)
            {
            ptr = m_head.getNext ();
            m_head.setNext (ptr->getNext ());
            if (ptr->getNext () != NULL)
                {
                ptr->getNext ()->setPrev (&m_head);
                }
            }
        m_mutex.unlock ();
        return ptr;
        }

    E * getHead ()
        {
        return m_head.getNext ();
        }

    int isEmpty () { return m_head.getNext () == NULL; }

    virtual void destroyList () 
        {
         // Discard all objects and destroy all storage
        while (!isEmpty ())
            {
            delete popHead ();
            }
        }

    private:

    void begin () { m_mutex.lock (); m_current = &m_head; }

    E * next () 
        { 
        if (m_current != NULL)
            {
            m_current = m_current.getNext ();
            }
        return m_current;
        }

    void end () { m_mutex.unlock (); }


    E *                     m_current;
    E                       m_head;
    Mx                      m_mutex;
    };

#define SAFEARRAY_SIG 0x53414645

typedef struct tagMEM_SAFEARRAY
    {
    DWORD           signature;
    VxMutex *       pMutex;
    ULONG           dataCount;
    VARTYPE         vt;
    SAFEARRAY       arrayData;
    } MEM_SAFEARRAY;

#define SA2MSA(psa) (reinterpret_cast<MEM_SAFEARRAY *>                      \
                        (reinterpret_cast<BYTE *>(psa) -                    \
                            (sizeof (MEM_SAFEARRAY) - sizeof (SAFEARRAY))))
	

extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////
// XXX
// I've put these here until we sort out the _EXACT_ details of
// Synchronisation classes et al. (aim)
void* vxdcomCondVarCreate ();
void vxdcomCondVarWait (void*, void*);
void vxdcomCondVarDestroy (void*);
void vxdcomCondVarSignal (void*);

HRESULT CLSIDFromAscii
    (
    const char*         s,		// Ascii string to be converted 
    LPCLSID             pClsid		// return value holding CLSID
    );

#ifdef __cplusplus
}
#endif

#endif



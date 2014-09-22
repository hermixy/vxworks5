/* RemoteRegistry.h - DOCM remote registry class */

/* Copyright (c) 2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01a,07aug01,dsellars  created
*/

#ifndef __INCRemoteRegistry_h
#define __INCRemoteRegistry_h

#ifdef __cplusplus

#include "dcomLib.h"
#include "comObjLib.h"
#include "private/comStl.h"

class RemoteRegistry :
    public CComObjectRoot,
    public IRegistry
    {
  public:
    virtual ~RemoteRegistry ();
    RemoteRegistry ();

    // IRegistry implementation...
    HRESULT RegisterClass
        (
        REFCLSID                clsid,
        void *                  pfnGetClassObject
        );
    
    HRESULT IsClassRegistered
        (
        REFCLSID                clsid
        );

    HRESULT CreateInstance
        (
        REFCLSID                clsid,
        IUnknown *              pUnkOuter,
        DWORD                   dwClsContext,
        const char *            hint,
        ULONG                   cMQIs,
        MULTI_QI *              pMQIs
        );

    HRESULT GetClassObject
        (
        REFCLSID                clsid,
        REFIID                  iid,
        DWORD                   dwClsContext,
        const char *            hint,
        IUnknown **             ppClsObj
        );

    HRESULT GetClassID
        (
        DWORD                   dwIndex,
        LPCLSID                 pclsid
        );        
    

    BEGIN_COM_MAP(RemoteRegistry)
        COM_INTERFACE_ENTRY(IRegistry)
    END_COM_MAP()

  private:

    // Not implemented
    RemoteRegistry (const RemoteRegistry& other);
    RemoteRegistry& operator= (const RemoteRegistry& rhs);

    // universal private instance-creation function
    HRESULT instanceCreate
        (
        bool                    classMode,
        REFCLSID                clsid,
        IUnknown *              pUnkOuter,
        DWORD                   dwClsContext,
        const char *            hint,
        ULONG                   cMQIs,
        MULTI_QI *              pMQIs
        );

    // In future, we may want to register specific classes on specific
    // remote servers, however, that functionality is not yet
    // supported, hence the 'entry' class is not really used...
    struct entry
        {
        CLSID   clsid;
        };
    
    typedef STL_MAP(CLSID, entry)       RegMap_t;

    RegMap_t    m_regmap;
    };

typedef CComObject<RemoteRegistry> RemoteRegistryClass;

extern "C" {
#endif

int dcomRemoteRegistryInit (void);

#ifdef __cplusplus
}
#endif

#endif /* __INCRemoteRegistry_h */

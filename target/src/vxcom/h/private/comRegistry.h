/* comRegistry.h - COM core registry for in-memory coclasses */

/* Copyright (c) 2001, Wind River Systems, Inc. */

/*
modification history
--------------------
01d,07aug01,dbs  return multiple interfaces during creation
01c,06aug01,dbs  add registry-show capability, remove C++ impl
01b,13jul01,dbs  fix warnings on IUnknown methods
01a,20jun01,dbs  written
*/

/*
DESCRIPTION

This class implements the IRegistry interface, providing a registry
for in-memory coclasses, i.e. those already in the program image.

Thus, it is vital that this registry is registered with comCoreLib
prior to any external coclasses trying to use it.
*/

#ifndef __INCcomRegistry_h
#define __INCcomRegistry_h

#include <semLib.h>
#include "comCoreTypes.h"               /* base types */
#include "comCoreLib.h"                 /* core COM funcs */

EXTERN_C int comRegistryInit (void);

/*
 * COM_REG_ENTRY - each registry entry is kept in a structure like
 * this, and they are linked together in a list held by the registry
 * object.
 */

typedef struct _comRegEntry
    {
    struct _comRegEntry *pNext;         /* list linkage */
    CLSID               clsid;          /* class ID */
    PFN_GETCLASSOBJECT  pfnGCO;         /* ptr to GCO func */
    } COM_REG_ENTRY;

/*
 * COM_REG_CLASS - the actual registry implementation class is defined
 * by this structure, which implements the IRegistry interface.
 */

typedef struct _comRegClass
    {
    const IRegistryVtbl *lpVtbl;        /* vptr in C++ terms */
    DWORD               dwRefCount;     /* reference counter */
    COM_REG_ENTRY *     pEntries;       /* list of reg entries */
    } COM_REG_CLASS;

/* IUnknown implementation */
HRESULT comRegistry_QueryInterface
    (
    IUnknown *          pThis,
    REFIID              riid,
    void**              ppvObject
    );

ULONG comRegistry_AddRef
    (
    IUnknown *          pThis
    );

ULONG comRegistry_Release
    (
    IUnknown *          pThis
    );

/* IRegistry implementation */
HRESULT comRegistry_RegisterClass
    (
    IRegistry *         pThis,
    REFCLSID            clsid,
    void*               pfnGetClassObject
    );

HRESULT comRegistry_IsClassRegistered
    (
    IRegistry *         pThis,
    REFCLSID            clsid
    );

HRESULT comRegistry_CreateInstance
    (
    IRegistry *         pThis,
    REFCLSID            clsid,
    IUnknown*           pUnkOuter,
    DWORD               dwClsContext,
    const char*         hint,
    ULONG               cMQIs,
    MULTI_QI *          pMQIs
    );

HRESULT comRegistry_GetClassObject
    (
    IRegistry *         pThis,
    REFCLSID            clsid,
    REFIID              iid,
    DWORD               dwClsContext,
    const char*         hint,
    IUnknown**          ppClsObj
    );

HRESULT comRegistry_GetClassID
    (
    IRegistry *         pThis,
    DWORD               dwIndex,
    LPCLSID             pclsid
    );

#endif /* __INCcomRegistry_h */




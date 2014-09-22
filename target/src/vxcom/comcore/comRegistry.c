/* comRegistry.c - COM core registry for in-memory coclasses */

/* Copyright (c) 2001, Wind River Systems, Inc. */

/*
modification history
--------------------
01g,07aug01,dbs  return multiple interfaces during creation
01f,06aug01,dbs  add registry-show capability
01e,13jul01,dbs  fix warnings on IUnknown methods
01d,27jun01,dbs  fix include filenames
01c,25jun01,dbs  rename vtbl macros to COM_VTBL_XXX
01b,22jun01,dbs  remove commas from vtbl macros
01a,20jun01,dbs  written
*/

#include <string.h>
#include "private/comRegistry.h"
#include "private/comSysLib.h"

#define COM_REG_NAME "COM Core Registry"

COM_VTABLE(IRegistry) _reg_vtbl = {
    COM_VTBL_HEADER
    /* IUnknown methods */
    COM_VTBL_METHOD (&comRegistry_QueryInterface),
    COM_VTBL_METHOD (&comRegistry_AddRef),
    COM_VTBL_METHOD (&comRegistry_Release),
    /* IRegistry methods */
    COM_VTBL_METHOD (&comRegistry_RegisterClass),
    COM_VTBL_METHOD (&comRegistry_IsClassRegistered),
    COM_VTBL_METHOD (&comRegistry_CreateInstance),
    COM_VTBL_METHOD (&comRegistry_GetClassObject),
    COM_VTBL_METHOD (&comRegistry_GetClassID)
};

/**************************************************************************
*
* comRegistryInit - init function for this module
*
* This function must be called at system init time to add this
* registry implementation to comCoreLib.
*
* RETURNS: 0 on success, -1 on failure
*
* NOMANUAL
*/

int comRegistryInit (void)
    {
    COM_REG_CLASS *     pReg;
    HRESULT             hr;

    /* Create an instance of the registry class */
    pReg = (COM_REG_CLASS*) comSysAlloc (sizeof (COM_REG_CLASS));
    if (pReg == NULL)
        return -1;

    /* Initialize it */
    pReg->lpVtbl = &_reg_vtbl;
    pReg->dwRefCount = 0;
    pReg->pEntries = NULL;

    /* Register it with comCoreLib */
    hr = comRegistryAdd (COM_REG_NAME,
                         CLSCTX_INPROC_SERVER,
                         (IRegistry*) pReg);
    if (FAILED (hr))
        return -1;
    
    return 0;
    }

/**************************************************************************
*
* RegisterClass - add get-class-object function to table
*
* RETURNS: S_OK always.
*
* NOMANUAL
*/

HRESULT comRegistry_RegisterClass
    (
    IRegistry *         pReg,
    REFCLSID            clsid,
    void*               vpfnGetClassObject
    )
    {
    COM_REG_CLASS *     pThis = (COM_REG_CLASS*) pReg;
    COM_REG_ENTRY *     pEntry;

    /* Create a new entry */
    pEntry = (COM_REG_ENTRY*) comSysAlloc (sizeof (COM_REG_ENTRY));
    if (pEntry == NULL)
        return E_OUTOFMEMORY;

    /* Fill it in */
    memcpy (&pEntry->clsid, clsid, sizeof (CLSID));
    pEntry->pfnGCO = (PFN_GETCLASSOBJECT) vpfnGetClassObject;

    /* Link it into the list */
    pEntry->pNext = pThis->pEntries;
    pThis->pEntries = pEntry;

    /* Done... */
    return S_OK;
    }


/**************************************************************************
*
* IsClassRegistered - look for class ID in map
*
* RETURNS: S_OK if it is registered, REGDB_E_CLASSNOTREG if not.
*
* NOMANUAL
*/

HRESULT comRegistry_IsClassRegistered
    (
    IRegistry *         pReg,
    REFCLSID            clsid
    )
    {
    COM_REG_CLASS *     pThis = (COM_REG_CLASS*) pReg;
    COM_REG_ENTRY *     pEntry;

    /* Iterate over the list looking for the CLSID */
    for (pEntry = pThis->pEntries; pEntry != NULL; pEntry = pEntry->pNext)
        {
        if (comGuidCmp (&pEntry->clsid, clsid))
            return S_OK;
        }

    /* Not found... */
    return REGDB_E_CLASSNOTREG;
    }


/**************************************************************************
*
* CreateInstance - tries to create an instance of the coclass
*
* RETURNS: S_OK if the creation succeeded and all interfaces were
*          present, or CO_S_NOTALLINTERFACES if some of the interfaces
*          were not found.
*
* NOMANUAL
*/

HRESULT comRegistry_CreateInstance
    (
    IRegistry *         pReg,
    REFCLSID            clsid,
    IUnknown*           pUnkOuter,
    DWORD               dwClsCtx,
    const char*         hint,
    ULONG               cMQIs,
    MULTI_QI *          pMQIs
    )
    {
    IClassFactory *     pFactory = NULL;
    HRESULT             hr;
    HRESULT             hrTotal = S_OK;
    IUnknown *          pUnk = NULL;
    int                 i;
    int                 n;
    
    /* Get class object... */
    hr = comRegistry_GetClassObject (pReg,
                                     clsid,
                                     &IID_IClassFactory,
                                     dwClsCtx,
                                     hint,
                                     (IUnknown**) &pFactory);
    if (FAILED (hr))
        return hr;
    
    /* Create an instance... */
    hr = IClassFactory_CreateInstance (pFactory,
                                       pUnkOuter,
                                       &IID_IUnknown,
                                       (void**) &pUnk);
    if (FAILED (hr))
        {
        IUnknown_Release (pFactory);
        return hr;
        }
    
    /* Get the multiple interfaces... */
    n = 0;
    for (i=0; i < cMQIs; ++i)
        {
        hr = IUnknown_QueryInterface (pUnk,
                                      pMQIs[i].pIID,
                                      (void**) &pMQIs[i].pItf);
        if (SUCCEEDED (hr))
            ++n;
        else
            hrTotal = CO_S_NOTALLINTERFACES;
        pMQIs[i].hr = hr;
        }

    /* Check we got some of the interfaces... */
    if (n == 0)
        hrTotal = E_NOINTERFACE;
    
    /* Release the IUnknown, and the factory, and return... */
    IUnknown_Release (pUnk);
    IUnknown_Release (pFactory);    

    return hrTotal;
    }


/**************************************************************************
*
* GetClassObject - gets the class-object for the given coclass
*
* RETURNS: S_OK on success, one of other HRESULT codes on failure
*
* NOMANUAL
*/

HRESULT comRegistry_GetClassObject
    (
    IRegistry *         pReg,
    REFCLSID            clsid,
    REFIID              iid,
    DWORD               dwClsCtx,
    const char*         hint,
    IUnknown**          ppClsObj
    )
    {
    COM_REG_CLASS *     pThis = (COM_REG_CLASS*) pReg;
    COM_REG_ENTRY *     pEntry;
    PFN_GETCLASSOBJECT  pfnGCO = NULL;

    /* Validate args */
    if (ppClsObj == NULL)
	return E_INVALIDARG;
    if (dwClsCtx & (CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER))
	return E_NOTIMPL;

    /* Search for the matching entry */
    for (pEntry = pThis->pEntries; pEntry != NULL; pEntry = pEntry->pNext)
        {
        if (comGuidCmp (&pEntry->clsid, clsid))
            {
            /* Get GCO function pointer */
            pfnGCO = pEntry->pfnGCO;

            /* Return class-factory object... */
            return (*pfnGCO) (clsid, iid, (void**) ppClsObj);
            }
        }

    /* Not found... */
    return REGDB_E_CLASSNOTREG;
    }


/**************************************************************************
*
* 
*/

HRESULT comRegistry_GetClassID
    (
    IRegistry *         pReg,
    DWORD               dwIndex,
    LPCLSID             pclsid
    )
    {
    COM_REG_CLASS *     pThis = (COM_REG_CLASS*) pReg;
    COM_REG_ENTRY *     pEntry;
    DWORD               ix = 0;
    
    /* Search for the N'th entry */
    pEntry = pThis->pEntries;
    while ((pEntry != NULL) && (ix < dwIndex))
        {
        pEntry = pEntry->pNext;
        ++ix;
        }

    /* Did we find it? */
    if ((pEntry == NULL) || (ix != dwIndex))
        return E_FAIL;

    /* We found it... */
    memcpy (pclsid, &pEntry->clsid, sizeof (CLSID));
    return S_OK;
    }


/**************************************************************************
*
*/

ULONG comRegistry_AddRef
    (
    IUnknown *          pReg
    )
    {
    COM_REG_CLASS *     pThis = (COM_REG_CLASS*) pReg;

    return ++pThis->dwRefCount;
    }


/**************************************************************************
*
*/

ULONG comRegistry_Release
    (
    IUnknown *          pReg
    )
    {
    COM_REG_CLASS *     pThis = (COM_REG_CLASS*) pReg;
    COM_REG_ENTRY *     pEntry;
    DWORD               n;

    n = --pThis->dwRefCount;
    if (n == 0)
        {
        /* Destroy this object */
        pEntry = pThis->pEntries;
        while (pEntry != NULL)
            {
            COM_REG_ENTRY *p = pEntry;
            pEntry = pEntry->pNext;
            comSysFree (p);
            }
        comSysFree (pThis);
        }
    return n;
    }

/**************************************************************************
*
*/

HRESULT comRegistry_QueryInterface
    (
    IUnknown *          pReg,
    REFIID	        iid,
    void**	        ppv
    )
    {
    /* Is it one of our own interfaces? */
    if (comGuidCmp (iid, &IID_IUnknown) || comGuidCmp (iid, &IID_IRegistry))
	{
	*ppv = pReg;
        IUnknown_AddRef (pReg);
	return S_OK;
	}
    return E_NOINTERFACE;
    }

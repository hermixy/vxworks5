/* comCoreLib.c - core COM support */

/* Copyright (c) 2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01n,10dec01,dbs  diab build
01m,06dec01,nel  Correct man pages.
01l,02nov01,nel  SPR#71427. Correct error in comAddRegistry when null
                 terminator wasn't being copied into registry entry.
01k,09aug01,dbs  check args to comClassObjectGet
01j,07aug01,dbs  return multiple interfaces during creation
01i,06aug01,dbs  add registry-show capability
01h,20jul01,dbs  add self-initialisation
01g,28jun01,dbs  add interlocked inc/dec funcs
01f,27jun01,dbs  ensure registry init
01e,25jun01,dbs  change WIDL vtable macro name to COM_xxx
01d,22jun01,dbs  add vtable-format symbol
01c,22jun01,dbs  add const to args for comGuidCmp
01b,21jun01,nel  Add comGuidCmp function.
01a,20jun01,dbs  written
*/

/*
DESCRIPTION

This library provides basic COM support in VxWorks. It implements an
extensible registry model, and provides functions to create instances
of classes via their class objects. Support for the basic COM
interfaces IUnknown and IClassFactory is present, and it can be used
either from C or C++. The vtable format used for COM interfaces is
defined by the C/C++ compiler and is detailed in comBase.h where each
of the known compilers is enumerated and macros are defined for
generating vtable structures, and filling in vtable instances.

The registration of coclasses is handled by a table of registry
instances within this library. When comCoreLib is asked to create an
instance of a coclass, it searches the table of known registries (each
of which is itself a COM object, exposing the IRegistry interface),
asking each in turn if it has the requested coclass (identified by
CLSID) registered. If so, it will then ask the registry to create an
instance of the coclass.
*/

/* includes */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "taskLib.h"
#include "comCoreLib.h"
#include "comErr.h"
#include "private/comRegistry.h"

/* defines */

#define COM_CORE_MAX_REGISTRIES 8

#define COM_CORE_LIB_INIT()             \
    if (comCoreLibInitFlag == 0)        \
        { comCoreLibInit (); }

/* globals */

/* This flag records whether comCoreLib has been initialized. Since it
 * is possible that some of the API functions (particularly the
 * registration functions) may be called during program
 * initialisation, it is important that this library allows those
 * calls to succeed even if its init routine hasn't been called by the
 * normal startup (in VxWorks, at least). Subsequent calls to the init
 * routine simply return OK and carry on as normal.
 */

static int comCoreLibInitFlag = 0;


/* This causes a special symbol, whose name is derived from the vtable
 * format determined by the WIDL_XXX macros in comBase.h, to be
 * declared at this point as a global. In all other modules it will be
 * defined as an external and so will only link if all the modules in
 * the program are compiled with the same vtable format defined.
 */

DECLARE_COM_VTBL_SYMBOL_HERE;

/*
 * This global is here to supply a default for the DCOM priority
 * propagation feature, when building coclasses with WOTL.
 */

int g_defaultServerPriority = 100;


/* Array of reg-entries */

static struct _comCoreRegEntry
    {
    char *              regName;        /* name of registry */
    DWORD               dwClsCtx;       /* inproc/local/remote */
    IRegistry *         pRegistry;      /* registry interface */
    } comCoreRegistries [COM_CORE_MAX_REGISTRIES];


/**************************************************************************
*
* comCoreLibInit - Initialize the comCore library
*
* Initialize the comCore library.
*
* RETURNS: OK if init succeeded, ERROR if not
*/

STATUS comCoreLibInit (void)
    {
    if (comCoreLibInitFlag == 0)
        {
        ++comCoreLibInitFlag;
        return comRegistryInit ();
        }
    
    return OK;
    }


/**************************************************************************
*
* comCreateAny - creates an instance of a class or class-object
*
* This function scans the registry table, looking for a registry
* instance with the right class-context (inproc, local or remote) and
* which has the class ID <clsid> registered.
*
* RETURNS: S_OK on success, some other failure code otherwise.
*
* NOMANUAL
*/

static HRESULT comCreateAny
    (
    REFCLSID            clsid,          /* class ID */
    IUnknown *          pUnkOuter,      /* aggregating interface */
    DWORD               dwClsCtx,       /* class context */
    const char *        hint,           /* service hint */
    int                 classMode,      /* get class object? */
    ULONG               cMQIs,          /* number of MQIs */
    MULTI_QI *          pMQIs           /* MQI array */
    )
    {
    HRESULT hr = REGDB_E_CLASSNOTREG;
    int i;
    IRegistry * pReg;

    /* Make sure the library is initialized... */
    COM_CORE_LIB_INIT ();
    
    /* Work backwards through the table of known registries, ensuring
     * we catch the most-recently added registry first.
     */
    
    for (i = COM_CORE_MAX_REGISTRIES-1; i >= 0; --i)
        {
        if ((comCoreRegistries[i].pRegistry != NULL) &&
            ((comCoreRegistries[i].dwClsCtx & dwClsCtx) != 0))
            {
            pReg = comCoreRegistries[i].pRegistry;

            /* Is the class registered? */
            hr = IRegistry_IsClassRegistered (pReg, clsid);
            if (SUCCEEDED (hr))
                {
                /* Create instance or Class-object? */
                if (classMode)
                    {
                    hr = IRegistry_GetClassObject (pReg,
                                                   clsid,
                                                   pMQIs[0].pIID,
                                                   dwClsCtx,
                                                   hint,
                                                   &pMQIs[0].pItf);
                    }
                else
                    {
                    hr = IRegistry_CreateInstance (pReg,
                                                   clsid,
                                                   pUnkOuter,
                                                   dwClsCtx,
                                                   hint,
                                                   cMQIs,
                                                   pMQIs);
                    }

                /* We can just exit the loop, and return whatever
                 * result came back from the registry...
                 */
                break;
                }
            }
        }
    
    return hr;
    }


/**************************************************************************
*
* comInstanceCreate - creates an instance of a coclass
*
* This function returns an instance of the coclass identified by
* <clsid>, asking for the interface <iid>. It may be being aggregated,
* in which case the argument <pUnkOuter> holds the controlling
* IUnknown interface of the aggregating object.
*
* RETURNS: S_OK on success, or an error indication if failure occurs
*          for some reason.
*/

HRESULT comInstanceCreate
    (
    REFCLSID            clsid,          /* class ID */
    IUnknown *          pUnkOuter,      /* aggregating interface */
    DWORD               dwClsCtx,       /* class context */
    const char *        hint,           /* service hint */
    ULONG               cMQIs,          /* number of MQIs */
    MULTI_QI *          pMQIs           /* MQI array */
    )
    {
    return comCreateAny (clsid,
                         pUnkOuter,
                         dwClsCtx,
                         hint,
                         FALSE,
                         cMQIs,
                         pMQIs);
    }


/**************************************************************************
*
* comClassObjectGet - returns a class object instance
*
* This function returns the class-object for the coclass identified by
* <clsid>, and asks for the interface <iid> which will usually, but
* not always, be IClassFactory.
*
* RETURNS:
* \is
* \i E_POINTER
* ppvClsObj doesn't point to a valid piece of memory.
* \i E_NOINTERFACE
* The requested interface doesn't exist.
* \i S_OK
* On success
* \ie
*/

HRESULT comClassObjectGet
    (
    REFCLSID            clsid,          /* class ID */
    DWORD               dwClsCtx,       /* class context */
    const char *        hint,           /* service hint */
    REFIID              iid,            /* class-obj IID */
    void **             ppvClsObj       /* resulting interface ptr */
    )
    {
    HRESULT hr;
#ifdef __DCC__
    MULTI_QI mqi[1];
    mqi[0].pIID = iid;
    mqi[0].pItf = NULL;
    mqi[0].hr = S_OK;
#else
    MULTI_QI mqi[] = { { iid, NULL, S_OK } };
#endif
    if (! ppvClsObj)
        return E_POINTER;
    
    hr = comCreateAny (clsid,
                       NULL,
                       dwClsCtx,
                       hint,
                       TRUE,
                       1,
                       mqi);

    if (FAILED (hr))
        return hr;

    if (ppvClsObj)
        *ppvClsObj = mqi[0].pItf;
    else
        hr = E_INVALIDARG;

    return hr;
    }


/**************************************************************************
*
* comRegistryAdd - add a registry instance to the COM core
*
* This function adds a registry instance to the table of all known
* registries in the COM core. If there are no more spaces in the
* registry table, it returns an error, otherwise the registry is added
* to the end of the table.
*
* RETURNS: S_OK if the registry instance was added, or some other error
* code if not.
*/

HRESULT comRegistryAdd
    (
    const char *        regName,        /* registry name */
    DWORD               dwClsCtx,       /* class context */
    IRegistry*          pReg            /* registry to add */
    )
    {
    HRESULT hr = REGDB_E_CLASSNOTREG;
    int i;
    
    /* Make sure the library is initialized... */
    COM_CORE_LIB_INIT ();
    
    /* Work forwards through the table of known registries, ensuring
     * we add this registry instance at the end of the table.
     */
    
    for (i = 0; i < COM_CORE_MAX_REGISTRIES; ++i)
        {
        if (comCoreRegistries[i].pRegistry == NULL)
            {
            size_t len;
            
            /* Add a reference-count, and store the info. */
            IUnknown_AddRef (pReg);
            
            len = strlen (regName);
            comCoreRegistries[i].regName = malloc (len + 1);
            strcpy (comCoreRegistries[i].regName, regName);
            
            comCoreRegistries[i].dwClsCtx = dwClsCtx;

            comCoreRegistries[i].pRegistry = pReg;

            /* We can leave now... */
            hr = S_OK;
            break;
            }
        }    
    
    return hr;
    }


/**************************************************************************
*
* comClassRegister - registers a 'get-class-object' function for a CLSID
*
* This function stores an association between the class ID <clsid> and
* the 'get class object' function pointer <pfnGCO> so that instances
* of the coclass can be created. The entry is made in the first known
* registry that can deal with the class context given in <dwClsCtx>.
*
* RETURNS: S_OK if the requested CLSID is found, or REGDB_E_CLASSNOTREG
* if it is not present.
*/

HRESULT comClassRegister
    (
    REFCLSID	        clsid,		/* class ID */
    DWORD               dwClsCtx,       /* class context */
    PFN_GETCLASSOBJECT  pfnGCO,         /* GetClassObject() func */
    VXDCOMPRIORITYSCHEME priScheme,     /* priority scheme */
    int                 priority	/* priority assoc. with scheme */
    )
    {
    HRESULT hr = REGDB_E_CLASSNOTREG;
    int i;
    IRegistry * pReg;
    
    /* Make sure the library is initialized... */
    COM_CORE_LIB_INIT ();
    
    /* Work forwards through the table of known registries, looking
     * for the first registry that can deal with this class.
     */
    
    for (i = 0; i < COM_CORE_MAX_REGISTRIES; ++i)
        {
        if ((comCoreRegistries[i].dwClsCtx & dwClsCtx) != 0)
            {
            /* Add to this registry. */
            pReg = comCoreRegistries[i].pRegistry;
            hr = IRegistry_RegisterClass (pReg, clsid, pfnGCO);
            break;
            }
        }
    
    return hr;
    }


/**************************************************************************
*
* comCoreGUID2String - convert GUID to a printable string
*
* Converts a GUID to a printable string.
*
* RETURNS: S_OK
*
*/

HRESULT comCoreGUID2String
    (
    const GUID *        pGuid,          /* GUID to print */
    char *              buf             /* GUID_STRING_LEN */
    )
    {
    sprintf (buf,
             "{%8.8X-%4.4X-%4.4X-%2.2X%2.2X-%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X}",
             (unsigned int) pGuid->Data1,
             (unsigned short)pGuid->Data2,
             (unsigned short)pGuid->Data3,
             pGuid->Data4[0],
             pGuid->Data4[1], 
             pGuid->Data4[2],
             pGuid->Data4[3], 
             pGuid->Data4[4],
             pGuid->Data4[5], 
             pGuid->Data4[6],
             pGuid->Data4[7]);

    return S_OK;
    }


/**************************************************************************
*
* comCoreRegShow - Displays a human readable version of the registry.
*
* RETURNS: OK
* NOMANUAL
*/

STATUS comCoreRegShow (void)
    {
    int i;
    
    /* Make sure the library is initialized... */
    COM_CORE_LIB_INIT ();
    
    /* Work forwards through the table of known registries, getting
     * the CLSIDs out of each one and printing them.
     */
    
    for (i = 0; i < COM_CORE_MAX_REGISTRIES; ++i)
        {
        IRegistry * pReg = comCoreRegistries[i].pRegistry;

        if (pReg != NULL)
            {
            DWORD       ix = 0;
            HRESULT     hr = S_OK;
            CLSID       clsid;
            
            /* Add a ref while we use it... */
            IUnknown_AddRef (pReg);

            /* Print the title... */
            printf ("%s:\n", comCoreRegistries[i].regName);
            
            /* Iterate over the CLSIDs... */
            while (hr == S_OK)
                {
                char buf [GUID_STRING_LEN];
                
                hr = IRegistry_GetClassID (pReg, ix++, &clsid);
                if (SUCCEEDED (hr))
                    {
                    comCoreGUID2String (&clsid, buf);
                    printf ("%s\n", buf);
                    }
                }

            /* Done - release the registry interface... */
            IUnknown_Release (pReg);
            }
        }

    return OK;
    }


/**************************************************************************
*
* comGuidCmp - compare two GUIDs
*
* This function compares two GUIDs and returns a true/false result.
*
* RETURNS: TRUE if the GUIDs are identical, FALSE otherwise.
*
*/

BOOL comGuidCmp
    (
    const GUID *	g1,		/* Source GUID */
    const GUID *	g2		/* GUID to compare to */
    )
    {
    return memcmp (g1, g2, sizeof (GUID)) == 0;
    }


/**************************************************************************
*
* comSafeInc - increments a variable in a task-safe way
*
* This function increments a variable, guaranteeing that no other task
* can simultaneously be modifying its value.
*
* RETURNS: incremented value
*/

long comSafeInc
    (
    long *      pVar
    )
    {
    long n;
    
    TASK_LOCK ();
    n = ++(*pVar);
    TASK_UNLOCK ();
    return n;
    }


/**************************************************************************
*
* comSafeDec - decrements a variable in a task-safe way
*
* This function decrements a variable, guaranteeing that no other task
* can simultaneously be modifying its value.
*
* RETURNS: decremented value
*/

long comSafeDec
    (
    long *      pVar
    )
    {
    long n;
    
    TASK_LOCK ();
    n = --(*pVar);
    TASK_UNLOCK ();
    return n;
    }

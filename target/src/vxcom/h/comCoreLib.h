/* comCoreLib.h - core COM definitions */

/* Copyright (c) 2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01f,07aug01,dbs  return multiple interfaces during creation
01e,06aug01,dbs  add registry-show capability
01d,28jun01,dbs  add interlocked inc/dec funcs
01c,27jun01,dbs  add GUID string len definition
01b,22jun01,dbs  add const to args for comGuidCmp
01a,20jun01,dbs  created
*/

#ifndef __INCcomCoreLib_h
#define __INCcomCoreLib_h

/* includes */
    
#include "comCoreTypes.h"               /* IDL-derived types */
#include "comErr.h"                     /* error codes & macros */
    
#ifdef __cplusplus

#include <string.h>

inline bool operator < (const GUID& a, const GUID& b)
    {
    return (memcmp (&a, &b, sizeof (GUID)) < 0);
    }

inline bool operator == (const GUID& a, const GUID& b)
    {
    return (memcmp (&a, &b, sizeof (GUID)) == 0);
    }

inline bool operator != (const GUID& a, const GUID& b)
    {
    return ! (a == b);
    }

extern "C"
{
#endif

/* defines */

#define GUID_STRING_LEN 64


/* types */

typedef HRESULT (*PFN_GETCLASSOBJECT) (REFCLSID, REFIID, void**);

typedef enum
    {
    PS_DEFAULT=0,                       /* use system default priority */
    PS_SVR_ASSIGNED,                    /* use priority in reg. info */
    PS_CLNT_PROPAGATED                  /* use priority from client */
    } VXDCOMPRIORITYSCHEME;

/* prototypes */

STATUS comCoreLibInit (void);

STATUS comCoreRegShow (void);
    
HRESULT comCoreGUID2String
    (
    const GUID *        pGuid,          /* GUID to print */
    char *              buf             /* GUID_STRING_LEN */
    );
    
HRESULT comInstanceCreate
    (
    REFCLSID            clsid,          /* class ID */
    IUnknown *          pUnkOuter,      /* aggregating interface */
    DWORD               dwClsCtx,       /* class context */
    const char *        hint,           /* service hint */
    ULONG               cMQIs,          /* number of MQIs */
    MULTI_QI *          pMQIs           /* MQI array */
    );

HRESULT comClassObjectGet
    (
    REFCLSID            clsid,          /* class ID */
    DWORD               dwClsCtx,       /* class context */
    const char *        hint,           /* service hint */
    REFIID              iid,            /* class-obj IID */
    void **             ppvClsObj       /* resulting interface ptr */
    );

HRESULT comRegistryAdd
    (
    const char *        regName,        /* registry name */
    DWORD               dwClsCtx,       /* class context */
    IRegistry*          pReg            /* registry to add */
    );

HRESULT comClassRegister
    (
    REFCLSID	        clsid,		/* class ID */
    DWORD               dwClsCtx,       /* class context */
    PFN_GETCLASSOBJECT  pfnGCO,         /* GetClassObject() func */
    VXDCOMPRIORITYSCHEME priScheme,     /* priority scheme */
    int                 priority	/* priority assoc. with scheme */
    );

BOOL comGuidCmp
    (
    const GUID *        a,              /* one GUID */
    const GUID *        b               /* another GUID */
    );
    
long comSafeInc
    (
    long *              pVar            /* variable to increment */
    );
    
long comSafeDec
    (
    long *              pVar            /* variable to decrement */
    );
    
#ifdef __cplusplus
}
#endif

#endif /* __INCcomCoreLib_h */



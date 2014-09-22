/* objLib.c - generic object management library */

/* Copyright 1984-1992 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01n,24jun96,sbs  made windview instrumentation conditionally compiled
01m,10dec93,smb  added instrumentation details CLASS_RESOLVE
01l,23aug92,jcf  commented out unused routines to conserve space.
01k,18jul92,smb  Changed errno.h to errnoLib.h.
01j,04jul92,jcf  private header files; removed printf calls
01i,26may92,rrr  the tree shuffle
01h,04dec91,rrr  removed VARARG_OK, no longer needed with ansi c.
01g,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -fixed #else and #endif
		  -changed VOID to void
		  -changed copyright notice
01f,18may91,gae  fixed varargs for 960 with conditional VARARG_OK,
		 namely: objCreate() & objInit().
01e,29sep90,jcf  documentation.
01d,15jul90,dnw  changed objAlloc() to return (void *) instead of (char *)
		 deleted 'extra' arg from objAlloc() and added objAllocExtra()
01c,03jul90,jcf  fixed NULL pointer reference in objTerminate.
01b,26jun90,jcf  added objAlloc()/objFree ().
		 added object alloc count/free count.
01a,09dec89,jcf  written.
*/

/*
DESCRIPTION
This library contains class object management routines.  Classes of objects
control object methods suchas creation, initialization, deletion, and
termination.

Many objects in VxWorks are managed with class organization.  These include
tasks, semaphores, watchdogs, memory partitions, symbol tables, hash tables,
message queues, and, recursively, classes themselves.

INCLUDE FILE: objLib.h

NOMANUAL
*/

#include "vxWorks.h"
#include "objLib.h"
#include "classLib.h"
#include "taskLib.h"
#include "memLib.h"
#include "intLib.h"
#include "errnoLib.h"
#include "stdarg.h"
#include "stdio.h"

#define	OBJ_MAX_ARGS	16	/* max number of variable arguments */

#define CLASS_OBJ_TO_TID(pObj)	((OBJ_ID)((int)pObj + taskClassId->coreOffset))

#ifdef WV_INSTRUMENTATION
/* This macro was modified to reflect changes for windview */
#define CLASS_RESOLVE(pObj) \
	(((CLASS_OBJ_TO_TID(pObj)->pObjClass == taskClassId) ||  \
	 (CLASS_OBJ_TO_TID(pObj)->pObjClass ==                  \
		 (CLASS_ID)(((struct obj_class *)(taskClassId))->initRtn))) ? \
         (taskClassId) : (((OBJ_ID)pObj)->pObjClass))

#else

#define CLASS_RESOLVE(pObj) \
	((CLASS_OBJ_TO_TID(pObj)->pObjClass == taskClassId) ? \
         (taskClassId) : (((OBJ_ID)pObj)->pObjClass))

#endif

#if FALSE       /* may utilize someday */

/*******************************************************************************
*
* objCreate - allocate and initialize an object
*
* This routine creates an object of the specified class via the classes create
* method.
*
* RETURNS: object id, or NULL if object couldn't be created.
*
* VARARGS1
*/

OBJ_ID objCreate
    (
    CLASS_ID classId,   /* object class of object to create */
    ...              /* optional arguments to create routine */
    )
    {
    va_list pArg;	/* traverses argument list */
    int ix;		/* handy index */
    int a[OBJ_MAX_ARGS];/* indigenous variables */

    if (OBJ_VERIFY (classId, classClassId) != OK)
	return (NULL);

    va_start (pArg, classId);
    for (ix = 0; ix < OBJ_MAX_ARGS; ++ix)
	a[ix] = va_arg (pArg, int);	/* varargs into indigenous variables */
    va_end (pArg);

    return ((OBJ_ID)((* classId->createRtn) (a[0], a[1], a[2], a[3], a[4],
					     a[5], a[6], a[7], a[8], a[9],
					     a[10], a[11], a[12], a[13],
					     a[14], a[15]))); /* XXX MAX_ARGS */
    }

/*******************************************************************************
*
* objInit - initialize an object
*
* This routine initializes an object to the specified class via the class
* initialization method.
*
* RETURNS: OK, or ERROR if object could not be initialized.
*
* VARARGS2
*/

STATUS objInit
    (
    CLASS_ID classId,   /* object class of object to create */
    OBJ_ID   objId,     /* pointer to object to initialize */
    ...              /* optional arguments to create routine */
    )
    {
    va_list pArg;	/* traverses argument list */
    int ix;		/* handy index */
    int a[OBJ_MAX_ARGS];/* indigenous variables */

    if (OBJ_VERIFY (classId, classClassId) != OK)
	return (ERROR);

    va_start (pArg, objId);
    for (ix = 0; ix < OBJ_MAX_ARGS; ++ix)
	a[ix] = va_arg (pArg, int);	/* varargs into indigenous variables */
    va_end (pArg);

    return ((* classId->initRtn) (objId, a[0], a[1], a[2], a[3], a[4], a[5],
				  a[6], a[7], a[8], a[9], a[10], a[11], a[12],
				  a[13], a[14], a[15])); /* XXX MAX_ARGS */
    }

/*******************************************************************************
*
* objDelete - delete object and deallocate all associated memory
*
* Deallocate memory associated with an object.
*
* RETURNS: OK, or ERROR if object could not be deleted.
*/

STATUS objDelete
    (
    OBJ_ID objId        /* object to delete */
    )
    {
    return (objDestroy (objId, TRUE, WAIT_FOREVER));
    }

/*******************************************************************************
*
* objTerminate - terminate object
*
* Terminate an object.
*
* RETURNS: OK, or ERROR if object could not be terminated.
*/

STATUS objTerminate
    (
    OBJ_ID objId        /* object to terminate */
    )
    {
    return (objDestroy (objId, FALSE, WAIT_FOREVER));
    }

/*******************************************************************************
*
* objDestroy - destroy an object
*
* Destroy an object, and optionally deallocate associated memory.
*
* RETURNS: OK, or ERROR if object could not be destroyed.
*/

STATUS objDestroy
    (
    OBJ_ID objId,       /* object to terminate */
    BOOL   dealloc,     /* deallocate memory associated with memory */
    int    timeout      /* timeout */
    )
    {
    CLASS_ID	pObjClass = CLASS_RESOLVE (objId);

    if (OBJ_VERIFY (&pObjClass->objCore, classClassId) != OK)
	return (ERROR);

    return ((* pObjClass->destroyRtn) (objId, dealloc, timeout));
    }

#endif /* may utilize someday */

/*******************************************************************************
*
* objShow - show information on an object
*
* Call class attached show routine for an object.
*
* RETURNS: OK, or ERROR if information could not be shown.
*/

STATUS objShow
    (
    OBJ_ID objId,       /* object to show information on */
    int    showType     /* show type */
    )
    {
    CLASS_ID	pObjClass = CLASS_RESOLVE (objId);

    if (OBJ_VERIFY (&pObjClass->objCore, classClassId) != OK)
	return (ERROR);

    if (pObjClass->showRtn == NULL)
	{
	errnoSet (S_objLib_OBJ_NO_METHOD);
	return (ERROR);
	}

    return ((* pObjClass->showRtn) (objId, showType));
    }

#if FALSE       /* may utilize someday */

/*******************************************************************************
*
* objHelp - provide help for an object
*
* Call class attached help routine for an object.
*
* RETURNS: OK, or ERROR if help could not be provided.
*/

STATUS objHelp
    (
    OBJ_ID objId,       /* object to get help with */
    int    helpType     /* help type */
    )
    {
    CLASS_ID    pObjClass = CLASS_RESOLVE (objId);

    if (OBJ_VERIFY (&pObjClass->objCore, classClassId) != OK)
        return (ERROR);

    if (pObjClass->helpRtn == NULL)
	{
	errnoSet (S_objLib_OBJ_NO_METHOD);
	return (ERROR);
	}

    return ((* pObjClass->helpRtn) (objId, helpType));
    }

#endif
/*******************************************************************************
*
* objAllocExtra - allocate an object from the object's partition with extra
*
* Allocate memory from a class memory partition, for an object.  Extra bytes
* may be requested.
*
* RETURNS: pointer to object, or NULL if object could not allocated.
*/

void *objAllocExtra
    (
    FAST CLASS_ID classId,      /* object class id */
    unsigned nExtraBytes,       /* additional bytes needed beyond object size */
    void ** ppExtra             /* where to return ptr to extra bytes */
                                /*   (NULL == don't return ptr) */
    )
    {
    void *pObj;

    /* return NULL if called from ISR or classId is invalid */

    if ((INT_RESTRICT () != OK) || (OBJ_VERIFY (classId, classClassId) != OK))
	return (NULL);

    pObj = memPartAlloc (classId->objPartId, classId->objSize + nExtraBytes);
    if (pObj != NULL)
	{
	classId->objAllocCnt ++;	/* keep track of allocation count */

	if (ppExtra != NULL)
	    *ppExtra = (void *) (((char *) pObj) + classId->objSize);
	}

    return (pObj);
    }

/*******************************************************************************
*
* objAlloc - allocate an object from the object's partition
*
* Allocate memory from a class memory partition, for an object.
*
* RETURNS: pointer to object, or NULL if object could not allocated.
*/

void *objAlloc
    (
    FAST CLASS_ID classId       /* object class id */
    )
    {
    return (objAllocExtra (classId, 0, (void **) NULL));
    }

/*******************************************************************************
*
* objFree - deallocate an object from the object's partition
*
* Deallocate memory from a class memory partition associated with an object.

* RETURNS: OK, or ERROR if object could not deallocated.
*/

STATUS objFree
    (
    CLASS_ID classId,   /* class id of object */
    char     *pObject   /* point to object memory to free */
    )
    {
    if ((OBJ_VERIFY (classId, classClassId) != OK) ||
        (memPartFree (classId->objPartId, pObject) != OK))
	return (ERROR);

    classId->objFreeCnt ++;		/* keep track of free count */

    return (OK);
    }

/*******************************************************************************
*
* objCoreInit - initialize object core
*
* Initialize and validate object core.
*/

void objCoreInit
    (
    OBJ_CORE    *pObjCore,      /* pointer to object core to initialize */
    OBJ_CLASS   *pObjClass      /* pointer to object class */
    )
    {
    pObjCore->pObjClass = pObjClass;		/* validate object */
    pObjClass->objInitCnt ++;			/* keep track of object count */
    }

/*******************************************************************************
*
* objCoreTerminate - terminate object core
*
* This routine terminates and invalidates an object.
*/

void objCoreTerminate
    (
    OBJ_CORE *pObjCore          /* pointer to object to invalidate */
    )
    {
    pObjCore->pObjClass->objTerminateCnt ++;	/* keep track of terminates */
    pObjCore->pObjClass = NULL;			/* invalidate object */
    }

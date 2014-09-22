/* classLib.c - VxWorks object class management library */

/* Copyright 1984-1992 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01n,12mar99,c_c  Doc: fixed SPR #7353.
01m,21feb99,jdi  doc: listed errno for classDestroy().
01l,24jun96,sbs  made windview instrumentation conditionally compiled
01k,14dec93,smb  corrected initRtn of inst class to point to non-inst class
01j,10dec93,smb  added instrumentation
01i,04jul92,jcf  private header files.
01h,26may92,rrr  the tree shuffle
01g,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -changed copyright notice
01f,28sep90,jcf   documentation.
01e,18jul90,jcf   validate partition in classMemPartIdSet().
01d,17jul90,dnw   changed to new objAlloc() call
01c,10jul90,jcf   documentation.
01b,26jun90,jcf   added objAlloc()/objFree().
		  added object create/delete/init/terminate counting.
		  fixed class object core initialization.
01a,14feb90,jcf   written.
*/

/*
DESCRIPTION
This library contains class object management routines.  Classes of objects
control object methods suchas creation, initialization, deletion, and
termination.

Many objects in VxWorks are managed with class organization.  These include
tasks, semaphores, watchdogs, memory partitions, symbol tables, hash tables,
message queues, and, recursively, classes themselves.

INCLUDE FILE: classLib.h

SEE ALSO: objLib(1).

NOMANUAL
*/

#include "vxWorks.h"
#include "classLib.h"
#include "objLib.h"
#include "intLib.h"
#include "errno.h"
#include "private/memPartLibP.h"
#include "private/eventP.h"	

/* locals */

LOCAL OBJ_CLASS classClass;

/* globals */

CLASS_ID classClassId = &classClass;

/*******************************************************************************
*
* classLibInit - initialize the class support library
*
* This routine initializes the class management package.
*
* NOMANUAL
*/

STATUS classLibInit (void)
    {
    classInit (classClassId, sizeof (OBJ_CLASS), OFFSET (OBJ_CLASS, objCore),
	       (FUNCPTR) classCreate, (FUNCPTR) classInit,
	       (FUNCPTR) classDestroy);

    return (OK);
    }

/*******************************************************************************
*
* classCreate - allocate and initialize an object class
*
* This routine allocates a OBJ_CLASS structure, and initializes it with
* the specified parameters.
*
* RETURNS: Pointer to a class id, or NULL if allocation failed.
*/

CLASS_ID classCreate
    (
    unsigned    objectSize,     /* size of object */
    int         coreOffset,     /* offset from objCore to object start */
    FUNCPTR     createRtn,      /* object creation routine */
    FUNCPTR     initRtn,        /* object initialization routine */
    FUNCPTR     destroyRtn      /* object destroy routine */
    )
    {
    CLASS_ID classId = (CLASS_ID) objAlloc (classClassId);

    if (classId != NULL)
	classInit (classId, objectSize, coreOffset, createRtn, initRtn,
		   destroyRtn);

    return (classId);				/* return initialized class */
    }

/*******************************************************************************
*
* classInit - initialize an object class
*
* This routine initializes the specified OBJ_CLASS structure with the
* specified parameters.
*
* RETURNS: OK.
*/

STATUS classInit
    (
    OBJ_CLASS   *pObjClass,     /* pointer to object class to initialize */
    unsigned    objectSize,     /* size of object */
    int         coreOffset,     /* offset from objCore to object start */
    FUNCPTR     createRtn,      /* object creation routine */
    FUNCPTR     initRtn,        /* object initialization routine */
    FUNCPTR     destroyRtn      /* object destroy routine */
    )
    {

    /* default memory partition is system partition */

    pObjClass->objPartId	= memSysPartId;	/* partition to allocate from */
    pObjClass->objSize		= objectSize;	/* record object size */
    pObjClass->objAllocCnt	= 0;		/* initially no objects */
    pObjClass->objFreeCnt	= 0;		/* initially no objects */
    pObjClass->objInitCnt	= 0;		/* initially no objects */
    pObjClass->objTerminateCnt	= 0;		/* initially no objects */
    pObjClass->coreOffset	= coreOffset;	/* set offset from core */

    /* initialize object methods */

    pObjClass->createRtn	= createRtn;	/* object creation routine */
    pObjClass->initRtn		= initRtn;	/* object init routine */
    pObjClass->destroyRtn	= destroyRtn;	/* object destroy routine */
    pObjClass->showRtn		= NULL;		/* object show routine */
    pObjClass->instRtn		= NULL;		/* object inst routine */

    /* initialize class as valid object */

    objCoreInit (&pObjClass->objCore, classClassId);

    return (OK);
    }

/*******************************************************************************
*
* classDestroy - destroy class
*
* Class destruction is not currently supported.
*
* RETURNS: ERROR always.
*
* ERRNO:
* S_classLib_NO_CLASS_DESTROY
*
* ARGSUSED
*/

STATUS classDestroy
    (
    CLASS_ID classId    /* object class to terminate */
    )
    {
    errno = S_classLib_NO_CLASS_DESTROY;

    return (ERROR);
    }

/*******************************************************************************
*
* classShowConnect - connect an arbitrary show routine to an object class
*
* This routine is used to attach an arbitrary show routine to an object class.
* The specified routine will be invoked by objShow().
*
* RETURNS: OK, or ERROR if invalid class.
*/

STATUS classShowConnect
    (
    CLASS_ID    classId,        /* object class to attach show routine to */
    FUNCPTR     showRtn         /* object show routine */
    )
    {
    if (OBJ_VERIFY (classId, classClassId) != OK)
	return (ERROR);
    classId->showRtn = showRtn;			/* attach inst routine */

#ifdef WV_INSTRUMENTATION
    /* windview
     * Attach showRtn to instrumented class. If the classId is non-instrumented
     * then initRtn will point to the instrumented class and vica versa. 
     */
    if ((wvInstIsOn) && (classId->initRtn != (FUNCPTR) NULL))
   	{
	((CLASS_ID) (classId->initRtn))->showRtn = showRtn;
	}
#endif

    return (OK);
    }

#ifdef WV_INSTRUMENTATION
/*******************************************************************************
*
* classInstConnect - connect an arbitrary instrument routine to an object class
*
* This routine is used to attach an instrument routine to an object 
* class. 
*
* RETURNS: OK, or ERROR if invalid class.
* NOMANUAL
*/

STATUS classInstConnect
    (
    CLASS_ID    classId,        /* object class to attach inst routine to */
    FUNCPTR     instRtn         /* object inst routine */
    )
    {
    if (OBJ_VERIFY (classId, classClassId) != OK)
	return (ERROR);

    classId->instRtn = instRtn;			/* attach inst routine */

    return (OK);
    }

#else

/*******************************************************************************
*
* classHelpConnect - connect an arbitrary help routine to an object class
*
* This routine is used to attach an arbitrary help routine to an object class.
* The specified routine will be invoked by objHelp().
*
* RETURNS: OK, or ERROR if invalid class.
*/

STATUS classHelpConnect
    (
    CLASS_ID    classId,        /* object class to attach help routine to */
    FUNCPTR     helpRtn         /* object help routine */
    )
    { 
    if (OBJ_VERIFY (classId, classClassId) != OK)
	return (ERROR);

    classId->instRtn = helpRtn;                 /* attach help routine */
				     
    return (OK);
    }

#endif

/*******************************************************************************
*
* classMemPartIdSet - set the object class memory allocation partition
*
* This routine is used to change an object class memory allocation partition
* from its default of the system memory pool to the specified partition.  The
* routine objAlloc() utilizes this partition as the basis for its allocation.
*
* RETURNS: OK, or ERROR if invalid class, or memory partition.
*/

STATUS classMemPartIdSet
    (
    CLASS_ID    classId,        /* object class to set memory partition for */
    PART_ID     memPartId       /* partition id to allocate objects from */
    )
    {
    if ((OBJ_VERIFY (classId, classClassId) != OK) ||
        (OBJ_VERIFY (memPartId, memPartClassId) != OK))
	return (ERROR);

    classId->objPartId = memPartId;		/* set partition id */

    return (OK);
    }

#ifdef WV_INSTRUMENTATION
/*******************************************************************************
*
* classInstrument - initialize the instrumented class 
*
* This routine initializes the instrumented class
*
* NOMANUAL
*/

STATUS classInstrument 
    (
    OBJ_CLASS * pObjClass, 
    OBJ_CLASS * pObjInstClass
    )
    {
    if (OBJ_VERIFY (pObjClass, classClassId) != OK) 
	return (ERROR);

    pObjInstClass->objPartId       = memSysPartId;
    pObjInstClass->objSize         = pObjClass->objSize;
    pObjInstClass->objAllocCnt     = 0;
    pObjInstClass->objFreeCnt      = 0;
    pObjInstClass->objInitCnt      = 0;
    pObjInstClass->objTerminateCnt = 0;
    pObjInstClass->coreOffset      = pObjClass->coreOffset;

    /* initialize object methods */

    pObjInstClass->createRtn       = pObjClass->createRtn;
    pObjInstClass->initRtn         = pObjClass;
    pObjInstClass->destroyRtn      = pObjClass->destroyRtn;
    pObjInstClass->showRtn         = NULL;
    pObjInstClass->instRtn         = (FUNCPTR) _func_evtLogOIntLock;

    /* initialize class as valid object */

    objCoreInit (&pObjInstClass->objCore, classClassId);

    return (OK);
    }

#endif


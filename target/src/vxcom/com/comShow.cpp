/* comShow.cpp - main COM show routine library module */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*

modification history
--------------------
01l,17dec01,nel  Add include symbol for diab build.
01k,06dec01,nel  Add docs comments.
01j,25sep01,nel  Add method to translate a guid to a string.
01i,06aug01,dbs  add registry-show capability
01h,30jul01,dbs  add vxdcomShow function
01g,27jun01,dbs  fix include paths and names
01f,30mar00,nel  Added extern c to comTrackShow
01e,06mar00,nel  Added VxComTrack class
01d,26apr99,aim  added TRACE_CALL
01c,14apr99,dbs  export registry-instance function to comShow
01b,09apr99,drm  added diagnostic output
01a,24mar99,drm  created

*/

#include "comShow.h"
#include "comLib.h"
#include "private/comMisc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
DESCRIPTION

This library implements a number of useful show functions that can
be used to display information about user CoClasses.

*/

/* Include symbol for diab */

extern "C" int include_vxcom_comShow (void)
    {
    return 0;
    }

/**************************************************************************
*
* comShowInit - VxWorks COM show library init function
*
* This function initializes the VxWorks COM show library.
*
* RETURNS: OK
*
*/

extern "C" int comShowInit ()
    {
    return 0;
    }

/**************************************************************************
*
* vxdcomShow - Displays the build date of the library.
*
* Displays the build date of the library for patching purposes.
*
* RETURNS: OK
* NOMANUAL
*/

extern "C" int vxdcomShow ()
    {
    printf ("VxDCOM (Bubbles) Built %s\n", __DATE__);
    return 0;
    }

/**************************************************************************
*
* comRegShow - Prints the VxCOM registry
*
* Displays a list of all the currently registered CoClasses in the VxCOM
* registry. This includes any CoClasses registered by VxDCOM.
*
* RETURNS: OK
*/

extern "C" int comRegShow ()
    {
    return comCoreRegShow ();
    }

/**************************************************************************
*
* comTrackShow - List of all instances of all instrumented CoClasses.
*
* This function displays a list of all the current instances of all
* instrumented CoClasses. To instrument a CoClass add the following
* to the build settings:
*
* -DVXDCOM_COMTRACK_LEVEL=1
*
* This build option adds a small amount of code to all the COM MAP macros
* so there is a small code size and throughput penalty.
*
* RETURNS: OK
*/

extern "C" int comTrackShow ()
    {
    return VxComTrack::theInstance ()->print ();
    }

#if defined (VXDCOM_PLATFORM_SOLARIS) || defined (VXDCOM_PLATFORM_LINUX) || (VXDCOM_PLATFORM_VXWORKS == 5)
unsigned long pdIdSelf (void)
    {
    return 0x12345678;
    }
#else
#include "pdLib.h"
#endif

extern "C" void setComTrackEntry (unsigned long, void * , void *);

static VxMutex trackMutex;

static VxComTrack :: INTERFACE comTrackInterfaces = { 0, 0, 0, };
static VxComTrack :: CLASS comTrackClasses = { 0, 0, };

VxComTrack :: VxComTrack ()
    {
    setComTrackEntry ((unsigned long)pdIdSelf (), 
                      (void *)(&comTrackClasses), 
                      (void *)(&comTrackInterfaces));
    }

VxComTrack :: ~VxComTrack ()
    {
    setComTrackEntry (0, NULL, NULL);
    }

VxComTrack * VxComTrack :: theInstance ()
    {
    static VxComTrack * s_pTheList = NULL;

    VxCritSec cs (trackMutex);

    if (s_pTheList == NULL)
        s_pTheList = new VxComTrack ();

    return s_pTheList;
    }

int VxComTrack :: print ()
    {
    VxCritSec cs (trackMutex);
    CLASS * classPtr = comTrackClasses.next;

    while (classPtr != NULL)
        {
        printf ("Class:%s - %s\n", classPtr->name, classPtr->guid);
        printf ("Instance:%p\nRefCount:%ld\n", 
                classPtr->thisPtr, classPtr->refCount);
        INTERFACE * interPtr = classPtr->interfaces;

        while (interPtr != NULL)
            {
            printf("\t%s - %s\n", interPtr->name, interPtr->guid);
            interPtr = interPtr->next;
            }
        printf("\n");
        classPtr = classPtr->next;
        }
    return 0;
    }

const char * VxComTrack :: findGUID (REFGUID guid)
    {
    VxCritSec cs (trackMutex);
    CLASS * classPtr = comTrackClasses.next;

    while (classPtr != NULL)
        {
	if (guid == classPtr->cls)
	    return classPtr->name;
	    
        INTERFACE * interPtr = classPtr->interfaces;

        while (interPtr != NULL)
            {
	    if (interPtr->iid == guid)
		return interPtr->name;
            interPtr = interPtr->next;
            }
        classPtr = classPtr->next;
        }
    return 0;
    }

VxComTrack :: CLASS * VxComTrack :: findClass 
    (
    void * thisPtr
    )
    {
    CLASS * ptr = comTrackClasses.next;

    while (ptr != NULL)
        {
        if (ptr->thisPtr == thisPtr)
            {
            return ptr;
            }
        ptr = ptr->next;
        }
    return NULL;
    }

void * VxComTrack :: addClassInstance
    (
    void          * thisPtr,
    char          * guid,
    char          * name,
    unsigned long   refCount,
    REFCLSID	    cls 
    )
    {
    VxCritSec cs (trackMutex);

    CLASS * ptr = findClass (thisPtr);

    /* if the class hasn't been tracked before set-up an entry */
    if (ptr == NULL)
        {
        ptr = new CLASS ();

        /* link into list */
        ptr->next = comTrackClasses.next;
        ptr->prev = &comTrackClasses;
        if (comTrackClasses.next != NULL)
            {
            comTrackClasses.next->prev = ptr;
            }
        comTrackClasses.next = ptr;

        /* fill in details */
        ptr->thisPtr = thisPtr;
        ptr->guid = new char [strlen (guid) + 1];
        strcpy (ptr->guid, guid);
        ptr->name = new char [strlen (name) + 1];
        strcpy (ptr->name, name);
        ptr->owner = (unsigned long)pdIdSelf ();
        ptr->refCount = refCount;
        ptr->magic1 = MAGIC1;
	ptr->cls = cls;

        /* initially there will be no interfaces listed */
        ptr->interfaces = NULL;

        return (void *)ptr;
        }
    ptr->refCount = refCount;
    return NULL;
    }

void VxComTrack :: updateClassInstance
    (
    void          * thisPtr,
    unsigned long   refCount
    )
    {
    VxCritSec cs (trackMutex);

    CLASS * classPtr = findClass (thisPtr);

    if (classPtr != NULL)
        {
        if (refCount > 0)
            {
            classPtr->refCount = refCount;
            }
        else
            {
            // delete chain of interfaces
            INTERFACE * interPtr = classPtr->interfaces;

            while (interPtr != NULL)
                {
                INTERFACE * tmp;

                interPtr->listPrev->listNext = interPtr->listNext;
                if (interPtr->listNext != NULL)
                    interPtr->listNext->listPrev = interPtr->listPrev;
                tmp = interPtr;
                interPtr = interPtr->next;
                delete tmp->name;
                delete tmp->guid;
                delete tmp;
                }

            // delete class
            classPtr->prev->next = classPtr->next;
            if (classPtr->next != NULL)
                classPtr->next->prev = classPtr->prev;
            delete classPtr->name;
            delete classPtr->guid;
            delete classPtr;
            }
        }
    }

VxComTrack :: INTERFACE * VxComTrack :: findInterface
    (
    CLASS * classPtr,
    char * guid
    )
    {
    if (classPtr != NULL)
        {
        INTERFACE * interPtr = classPtr->interfaces;

        while (interPtr != NULL)
            {
            if (!strcmp(interPtr->guid, guid))
                {
                return interPtr;
                }
            interPtr = interPtr->next;
            }
        }
    return NULL;
    }

void VxComTrack :: addInterface
    (
    void              * thisPtr,
    char              * guid,
    char              * name,
    REFIID		iid
    )
    {
    VxCritSec cs (trackMutex);

    CLASS * parent = findClass (thisPtr);

    if (parent != NULL)
        {
        if (findInterface (parent, guid) == NULL)
            {
            INTERFACE * interPtr;

            interPtr = new INTERFACE ();

            interPtr->name = new char [strlen (name) + 1];
            strcpy (interPtr->name, name);
            interPtr->guid = new char [strlen (guid) + 1];
            strcpy (interPtr->guid, guid);
            interPtr->owner = (unsigned long)parent;
            interPtr->magic1 = MAGIC1;
	    interPtr->iid = iid;

            interPtr->next = parent->interfaces;
            parent->interfaces = interPtr;

            interPtr->listNext = comTrackInterfaces.listNext;
            interPtr->listPrev = &comTrackInterfaces;
            if (comTrackInterfaces.listNext != NULL)
                {
                comTrackInterfaces.listNext->listPrev = interPtr;
                }
            comTrackInterfaces.listNext = interPtr;
            }
        }
    }


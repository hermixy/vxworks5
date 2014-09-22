/* ifIndexLib.c - interface index library */

/* Copyright 1984 - 2000 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01b,11jul01,jpf  Fixed a problem with the init routine resetting the index
01a,29mar01,spm  file creation: copied from version 01b 0f tor2_0.open_stack
                 branch (wpwr VOB) for unified code base
*/

/*
DESCRIPTION
This library allocates unique interface indexes.  Indexes start from  
1 and increase monotonically.  

ifIndexLibInit() must be called before any other functions.

ifIndexAlloc() and ifIndexTest() are reentrant,
ifIndexLibInit() and ifIndexLibShutdown() are not.

INCLUDE FILES: ifIndexLib.h

.pG "Network"

NOMANUAL
*/

/* includes */
#include "vxWorks.h"
#include "semLib.h"
#include "ifIndexLib.h"

/* externs */

/* globals */

/* defines */

/* typedefs */


/* locals */
LOCAL int ifIndexValue = 0;     /* 0 = uninitialized, -1 = overflowed, other = OK */
LOCAL SEM_ID ifIndexSem;

/* forward declarations */

/***************************************************************************
*
* ifIndexLibInit - initializes library variables
*
* ifIndexLibInit() resets library internal state.  This function must be called
* before any other functions in this library.
*
* RETURNS: N/A
*/

void ifIndexLibInit (void)
    {
    if (ifIndexValue == 0)
        {
        /* library uninitialized */
        ifIndexSem = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE);
        ifIndexValue = 1;
        }
    }
    
/***************************************************************************
*
* ifIndexLibShutdown - frees library variables
*
* ifIndexLibShutdown() frees library internal structures.  
* ifIndexLibInit() must be called before the library can be used again.
*
* RETURNS: N/A
*/

void ifIndexLibShutdown (void)
    {
    if (ifIndexValue != 0)
        {
        /* library initialized */
        semTake(ifIndexSem, WAIT_FOREVER);
        ifIndexValue = 0;
        semDelete(ifIndexSem);
        }
    /* else do nothing */
    }

/***************************************************************************
*
* ifIndexAlloc - return a unique interface index
*
* ifIndexAlloc() returns a unique integer to be used as an interface
* index.  The first index returned is 1.  ERROR is returned if the library
* has not been initialized by a call to ifIndexLibInit();
*
* RETURNS: interface index or ERROR
*/

int ifIndexAlloc (void)
    {
    int ret;

    if (ifIndexValue == 0)
        {
        /* library not initialized */
        return ERROR;
        }
    semTake(ifIndexSem, WAIT_FOREVER);
    if (ifIndexValue == -1)
        {
        /* overflow after 4 billion allocs */
        semGive(ifIndexSem);
        return ERROR;
        }
    ret = ifIndexValue++;
    semGive(ifIndexSem);
    return ret;
    }

/***************************************************************************
*
* ifIndexTest - returns true if an index has been allocated.
*
* ifIndexTest() returns TRUE if <index> has already been allocated by
* ifIndexLibAlloc().  Otherwise returns FALSE.  If the library has not
* been initialized returns FALSE.  This function does not check if the
* index actually belongs to a currently valid interface.
*
* RETURNS: TRUE or FALSE
*/

BOOL ifIndexTest
    (
    int ifIndex    /* the index to test */
    )
    {
    if (ifIndex == -1 || ifIndex == 0)
        {
        /* these values are never valid */
        return FALSE;
        }
    semTake(ifIndexSem, WAIT_FOREVER);
    if ((unsigned int)ifIndex < (unsigned int)ifIndexValue)
        {
        semGive(ifIndexSem);
        return TRUE;
        }
    semGive(ifIndexSem);
    return FALSE;
    }


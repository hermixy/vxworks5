/* comSysLib.c - vxcom OS abstraction layer  */

/* Copyright (c) 2001 Wind River Systems, Inc. */

/*

modification history
--------------------
01d,29jun01,nel  Add wrapper function to call time function from either ntp of
                 system time module.
01c,27jun01,dbs  add realloc function
01b,22jun01,dbs  add alloc/free funcs
01a,20jun01,nel  created

*/

/*
DESCRIPTION

This library provides an OS abstraction layer to VxCOM. This allows VxCOM
to be executed on a number of different host OS whilst still using the 
standard VxWorks library functions.

This library also includes a number of system wide generic functions that
need to be run in kernel mode on VxWorks.

*/

/* includes */

#include <stdlib.h>
#include "private/comSysLib.h"
#include "time.h"

#ifdef __cplusplus
extern "C" {
#endif

/* locals */

unsigned long (*pSysTimeGet)(void) = NULL;

/* OS specific support */

#ifdef VXDCOM_PLATFORM_VXWORKS
#include "VxWorks/comVxWorksSupport.c"
#endif

#ifdef VXDCOM_PLATFORM_SOLARIS
#include "sun4-solaris2/comUnixSupport.c"
#endif

#ifdef VXDCOM_PLATFORM_LINUX
#include "x86-linux2/comUnixSupport.c"
#endif

/* Generic functions */

#include "generic/comGuidLib.c"
#include "generic/comTrackLib.c"

/**************************************************************************
*
* comSysAlloc - generic memory allocation function
*/
    
void * comSysAlloc (unsigned long nb)
    {
    return malloc (nb);
    }

/**************************************************************************
*
* comSysRealloc - generic memory re-allocation function
*/
    
void * comSysRealloc (void* pv, unsigned long nb)
    {
    return realloc (pv, nb);
    }

/**************************************************************************
*
* comSysFree - generic memory free function
*/

void comSysFree (void* pv)
    {
    free (pv);
    }

/**************************************************************************
*
* comSysSetTimeGetPtr - Set's an alternate time handler routine.
*
* This method installs an alternative time handlering routine to replace
* the simple call to time, i.e. The NTP module.
*
* RETURNS: N/A
*.NOMANUAL
*/


void comSysSetTimeGetPtr (unsigned long (*pPtr)(void))
    {
    pSysTimeGet = pPtr;
    }

/**************************************************************************
*
* comSysTimeGet - returns the time in seconds since Jan 1, 1970
*
* This function returns the time in seconds since Jan 1, 1970. 
*
* RETURNS: the time in seconds since Jan 1, 1970.
*
*/

unsigned long comSysTimeGet (void)
    {
    if (NULL != pSysTimeGet)
        {
        return pSysTimeGet ();
        }
    return (unsigned long)time (NULL);
    }

#ifdef __cplusplus
	}
#endif

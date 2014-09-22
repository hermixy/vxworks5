/* cplusLib.c - core library initialization and runtime support */

/* Copyright 1993 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02c,21jan02,sn   remove call to cplusCtorsLink; static initializers are now 
                 run as a side-effect of including INCLUDE_CTORS_DTORS
02b,17sep98,ms   removed call to cplusDemanglerInit (so LOADER not forced in)
02a,10apr98,sn   added call to cplusDemanglerInit
01l,02apr96,srh  reactivate call to cplusCtorsLink.
01k,30nov95,srh  remove call to cplusCtorsLink (have developers call
		   it themselves).
01j,19jun95,srh  rename op new/delete intf for g++.
01i,31oct93,srh  initialized cplusNewHdlMutex
01h,18jun93,jdi  more doc cleanup.
01g,03jun93,srh  doc cleanup
01f,24apr93,srh  undid filename change in 01c; there is no longer
		 a monolithic object file.
01e,23apr93,srh  implemented new force-link/initialization scheme
01d,21apr93,srh  split out loader functions, putting them in cplusLoad.C
01c,04feb93,srh  changed name from cplusLib.C so that cplusLib.o could
                 be used for name of monolithic object file.
01b,02feb93,srh  change nit in modhist.
                 cloned to product cplusLibDoc.C for documentation purposes.
01a,31jan93,srh  written.
*/

/*
DESCRIPTION
This library provides run-time support and shell utilities that support
the development of VxWorks applications in C++.  The run-time support can
be broken into three categories:
.IP - 4
Support for C++ new and delete operators.
.IP -
Support for arrays of C++ objects.
.IP -
Support for initialization and cleanup of static objects.
.LP
Shell utilities are provided for:
.IP - 4
Resolving overloaded C++ function names.
.IP -
Hiding C++ name mangling, with support for terse or complete
name demangling.
.IP -
Manual or automatic invocation of static constructors and destructors.
.LP
The usage of cplusLib is more fully described in the
.I "WindC++ Gateway User's Guide."
*/

/* Includes */

#include "vxWorks.h"
#include "cplusLib.h"
#include "semLib.h"
#include "taskLib.h"

/* Defines */

/* Globals */

/* Locals */

LOCAL BOOL		  cplusLibInitialized = FALSE;

/* Forward declarations */

/*******************************************************************************
*
* cplusLibMinInit - initialize the minimal C++ library (C++)
*
* This routine initializes the C++ library without forcing unused C++ run-time
* support to be linked with the bootable VxWorks image.  If INCLUDE_CPLUS_MIN
* is defined in configAll.h, cplusLibMinInit() is called automatically from
* the root task, usrRoot(), in usrConfig.c.
*
* RETURNS: OK or ERROR.
*/

STATUS cplusLibMinInit (void)
    {
    if (!cplusLibInitialized)
	{
	cplusLibInitialized = TRUE;
	cplusNewHdlMutex = semMCreate (SEM_Q_PRIORITY
				       | SEM_DELETE_SAFE
				       | SEM_INVERSION_SAFE);
	cplusArraysInit ();

        /* 
         * notice that static initializers are run when
         * the INCLUDE_CTORS_DTORS component is initialized
         * rather than here.
         */
 	}  
    return OK;
    }
/****************************************************************
 *
 * __pure_virtual_called - oops, no definition for some pure virtual function
 *
 * This function is called when an inheritance chain fails to provide a
 * definition for a pure virtual function. It logs an error message and
 * suspends the calling task.
 *
 * NOMANUAL
 */

void __pure_virtual_called ()
    {
    extern FUNCPTR _func_logMsg;
    if (_func_logMsg != 0)
	{
	(* _func_logMsg)
	("Task called pure virtual function for which there is no definition\n",
	 0, 0, 0, 0, 0, 0);
	}
    taskSuspend (0);
    }

void __pure_virtual ()
    {
    __pure_virtual_called ();
    }

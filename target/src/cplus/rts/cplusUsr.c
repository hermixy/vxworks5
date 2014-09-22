/* cplusUsr.c - C++ user interface library */

/* Copyright 1993-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01m,22may02,fmk  change comments for cplusCtors and cplusDtors
01l,19mar02,sn   SPR 71699 - Moved cplusXtorStrategy to cplusXtors.c
01k,22jan02,sn   Changed to C file
01j,07dec01,sn   moved demangler functions to cplusDem.cpp
01i,11oct98,ms   reworked 01h fix, since it caused bootrom build to fail
01h,08oct98,sn   moved defn of cplusXtorStrategy to cplusInit.cpp
01g,14jul97,dgp  doc: add windsh x-ref to cplusCtors(), cplusDtors(), 
		      & cplusXtorSet()
01f,08oct96,bjl  changed cplusXtorStrategy from MANUAL to AUTOMATIC
01e,31oct93,srh  fixed spr 2270
01d,18jun93,jdi  more doc cleanup.
01c,03jun93,srh  doc cleanup
01b,23apr93,srh  implemented new force-link/initialization scheme
01a,31jan93,srh  written.
*/

/*
DESCRIPTION
This module provides C++ support routines meant to be called from the
VxWorks shell. It provides utilities for calling constructors and
destructors for static objects, and for setting modes used by the C++
demangler and the static object support routines.

NOMANUAL
*/

/* includes */

#include "vxWorks.h"
#include "cplusLib.h"
#include "moduleLib.h"

extern int (*_func_logMsg) (const char *, ...);

/* defines */

/* typedefs */

/* globals */

char __cplusUsr_o = 0;

/* locals */

/* forward declarations */

/******************************************************************************
*
* cplusXtorSet - change C++ static constructor calling strategy (C++)
*
* This command sets the C++ static constructor calling strategy
* to <strategy>.  The default strategy is 0.
*
* There are two static constructor calling strategies: <automatic>
* and <manual>.  These modes are represented by numeric codes:
*
* .TS
* center,tab(|);
* lf3 lf3
* l l.
* Strategy   | Code
* _
* manual     | 0
* automatic  | 1
* .TE
*
* Under the manual strategy, a module's static constructors and
* destructors are called by cplusCtors() and cplusDtors(), which are
* themselves invoked manually.
* 
* Under the automatic strategy, a module's static constructors are
* called as a side-effect of loading the module using the VxWorks module
* loader.  A module's static destructors are called as a side-effect of
* unloading the module.
*
* NOTE: The manual strategy is applicable only to modules that are
* loaded by the VxWorks module loader.  Static constructors and
* destructors contained by modules linked with the VxWorks image
* are called using cplusCtorsLink() and cplusDtorsLink().
*
* RETURNS: N/A
*
* SEE ALSO
* windsh,
* .tG "Shell"
*/
void cplusXtorSet
    (
    int strategy
    )
    {
    cplusXtorStrategy = (CPLUS_XTOR_STRATEGIES) strategy;
    }
/******************************************************************************
*
* callAllCtors - used by cplusCtors to call ctors for all loaded modules
*
*
* RETURNS: TRUE
*
* NOMANUAL
*/
static BOOL callAllCtors
    (
    MODULE_ID	module,
    int dummy
    )
    {
    if (module->ctors != 0)
	{
	cplusCallCtors (module->ctors);
	}
    return TRUE;
    }
/******************************************************************************
*
* callAllDtors - used by cplusDtors to call dtors for all loaded modules
*
* RETURNS: TRUE
*
* NOMANUAL
*/
static BOOL callAllDtors
    (
    MODULE_ID	module,
    int dummy
    )
    {
    if (module->dtors != 0)
	{
	cplusCallDtors (module->dtors);
	}
    return TRUE;
    }
/****************************************************************
*
* cplusCtors - call static constructors (C++)
*
* This function is used to call static constructors under the manual
* strategy (see cplusXtorSet()).  <moduleName> is the name of an
* object module that was "munched" before loading.  If <moduleName> is 0,
* then all static constructors, in all modules loaded by the VxWorks
* module loader, are called.
*
* EXAMPLES
* The following example shows how to initialize the static objects in
* modules called "applx.out" and "apply.out".
* 
* .CS
*     -> cplusCtors "applx.out"
*     value = 0 = 0x0
*     -> cplusCtors "apply.out"
*     value = 0 = 0x0
* .CE
* 
* The following example shows how to initialize all the static objects that are
* currently loaded, with a single invocation of cplusCtors():
* 
* .CS
*     -> cplusCtors
*     value = 0 = 0x0
* .CE
*
* WARNING
* cplusCtors() should only be called once per module otherwise unpredictable
* behavior may result.
* 
* RETURNS: N/A
*
* SEE ALSO: cplusXtorSet(), windsh,
* .tG "Shell"
*/
void cplusCtors
    (
    const char * moduleName	/* name of loaded module */
    )
    {
    MODULE_ID module;
    
    if (moduleName == 0)
	{
	moduleEach ((FUNCPTR) callAllCtors, 0);
	return;
	}
    else if ((module = moduleFindByName ((char *) moduleName)) == 0)
	{
        if (_func_logMsg != 0)
	    {
	    (* _func_logMsg) ("cplusCtors: can't find module \"%s\"\n",
			      (int) moduleName, 0, 0, 0, 0, 0);
            }
	return;
	}
    
    if (module->ctors != 0)
	{
	cplusCallCtors (module->ctors);
	}
    }
/****************************************************************
*
* cplusDtors - call static destructors (C++)
*
* This function is used to call static destructors under the manual
* strategy (see cplusXtorSet()).  <moduleName> is the name of an
* object module that was "munched" before loading.  If <moduleName> is 0,
* then all static destructors, in all modules loaded by the VxWorks
* module loader, are called.
*
* EXAMPLES
* The following example shows how to destroy the static objects in
* modules called "applx.out" and "apply.out":
* 
* .CS
*     -> cplusDtors "applx.out"
*     value = 0 = 0x0
*     -> cplusDtors "apply.out"
*     value = 0 = 0x0
* .CE
* 
* The following example shows how to destroy all the static objects that are
* currently loaded, with a single invocation of cplusDtors():
* 
* .CS
*     -> cplusDtors
*     value = 0 = 0x0
* .CE
*
* WARNING
* cplusDtors() should only be called once per module otherwise unpredictable
* behavior may result.
* 
* RETURNS: N/A
* 
* SEE ALSO: cplusXtorSet(), windsh,
* .tG "Shell"
*/
void cplusDtors
    (
    const char * moduleName
    )
    {
    MODULE_ID module;

    if (moduleName == 0)
	{
	moduleEach ((FUNCPTR) callAllDtors, 0);
	return;
	}
    else if ((module = moduleFindByName ((char *) moduleName)) == 0)
	{
        if (_func_logMsg != 0)
	    {
	    (* _func_logMsg) ("cplusDtors: can't find module \"%s\"\n",
			      (int) moduleName, 0, 0, 0, 0, 0);
	    }
	return;
	}

    if (module->dtors != 0)
	{
	cplusCallDtors (module->dtors);
	}
    }

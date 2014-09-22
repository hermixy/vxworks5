/* cplusLib/cplusLibDoc.c - basic run-time support for C++ */

/* Copyright 1993-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02e,13mar02,fmk  change comments for cplusCtors and cplusDtors
02d,07dec01,sn   added cplusDemanglerStyleSet
02c,29oct01,cyr  doc: fix SPR 62767 cplusXtorSet
02b,30mar99,jdi  doc: removed troff special chars causing refgen trouble.
02a,14mar99,jdi  added modhist section; switched TUG xref to VPG;
		 doc: removed refs to config.h and/or configAll.h (SPR 25663).
01a,16oct98,fle  created.
*/

/*
DESCRIPTION
This library provides run-time support and shell utilities that support
the development of VxWorks applications in C++.  The run-time support can
be broken into three categories:
.IP - 4
Support for C++ new and delete operators.
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
.I "VxWorks Programmer's Guide: C++ Development."
.SH "SEE ALSO"
.pG "C++ Development"

*/

/*******************************************************************************
*
* cplusCallNewHandler - call the allocation failure handler (C++)
*
* This function provides a procedural-interface to the new-handler.  It
* can be used by user-defined new operators to call the current
* new-handler.  This function is specific to VxWorks and may not be
* available in other C++ environments.
*
* RETURNS: N/A
*
*/

extern void cplusCallNewHandler ()
    {
    ...
    }

/*******************************************************************************
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
* SEE ALSO
* cplusXtorSet()
*/

extern "C" void cplusCtors
    (                         
    const char *  moduleName  /* name of loaded module */
    )                         
    {
    ...
    }

/*******************************************************************************
*
* cplusCtorsLink - call all linked static constructors (C++)
*
* This function calls constructors for all of the static objects linked
* with a VxWorks bootable image.  When creating bootable applications,
* this function should be called from usrRoot() to initialize all static
* objects.  Correct operation depends on correctly munching the C++
* modules that are linked with VxWorks.
*
* RETURNS: N/A
*/

extern "C" void cplusCtorsLink ()
    {
    ...
    }

/*******************************************************************************
*
* cplusDemanglerSet - change C++ demangling mode (C++)
*
* This command sets the C++ demangling mode to <mode>.
* The default mode is 2.
*
* There are three demangling modes, <complete>, <terse>, and <off>.
* These modes are represented by numeric codes:
*
* .TS
* center,tab(|);
* lf3 lf3
* l l.
* Mode     | Code
* _
* off      | 0
* terse    | 1
* complete | 2
* .TE
*
* In complete mode, when C++ function names are printed, the class
* name (if any) is prefixed and the function's parameter type list
* is appended.
*
* In terse mode, only the function name is printed. The class name
* and parameter type list are omitted.
*
* In off mode, the function name is not demangled.
* .SH "EXAMPLES"
* The following example shows how one function name would be printed
* under each demangling mode:
*
* .TS
* center,tab(|);
* lf3 lf3
* l l.
* Mode     | Printed symbol
*_
* off      | _member__5classFPFl_PvPFPv_v
* terse    | _member
* complete | foo::_member(void* (*)(long),void (*)(void*))
* .TE
*
* RETURNS: N/A
*/

extern "C" void cplusDemanglerSet
    (          
    int  mode  
    )          
    {
    ...
    }

/*******************************************************************************
*
* cplusDemanglerStyleSet - change C++ demangling style (C++)
*
* This command sets the C++ demangling mode to <style>.
* The available demangler styles are enumerated in demangler.h.
* The default demangling style depends on the toolchain used 
* to build the kernel. For example if the Diab toolchain is 
* used to build the kernel then the default demangler style 
* is DMGL_STYLE_DIAB. 
*
* RETURNS: N/A
*/

extern "C" void cplusDemanglerStyleSet
    (          
    DEMANGLER_STYLE style
    )          
    {
    ...
    }

/*******************************************************************************
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
* SEE ALSO
* cplusXtorSet()
*/

extern "C" void cplusDtors
    (                         
    const char *  moduleName  
    )                         
    {
    ...
    }

/*******************************************************************************
*
* cplusDtorsLink - call all linked static destructors (C++)
*
* This function calls destructors for all of the static objects linked
* with a VxWorks bootable image.  When creating bootable applications,
* this function should be called during system shutdown to decommission
* all static objects.  Correct operation depends on correctly munching
* the C++ modules that are linked with VxWorks.
*
* RETURNS: N/A
*/

extern "C" void cplusDtorsLink ()
    {
    ...
    }

/*******************************************************************************
*
* cplusLibInit - initialize the C++ library (C++)
*
* This routine initializes the C++ library and forces all C++ run-time support
* to be linked with the bootable VxWorks image.  If the configuration
* macro INCLUDE_CPLUS is defined,
* cplusLibInit() is called automatically from the root task,
* usrRoot(), in usrConfig.c.
*
* RETURNS: OK or ERROR.
*/

extern "C" STATUS cplusLibInit (void)
    {
    ...
    }


/*******************************************************************************
*
* cplusXtorSet - change C++ static constructor calling strategy (C++)
* 
* This command sets the C++ static constructor calling strategy
* to <strategy>.  The default strategy is 1.
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
* NOTE
* The manual strategy is applicable only to modules that are
* loaded by the VxWorks module loader.  Static constructors and
* destructors contained by modules linked with the VxWorks image
* are called using cplusCtorsLink() and cplusDtorsLink().
* 
* RETURNS: N/A
*/

extern "C" void cplusXtorSet
    (              
    int  strategy  
    )              
    {
    ...
    }

/*******************************************************************************
*
* operator;delete - default run-time support for memory deallocation (C++)
*
* This function provides the default implementation of operator delete.
* It returns the memory, previously allocated by operator new, to the
* VxWorks system memory partition.
*
* RETURNS: N/A
*/

extern void operator delete
    (            
    void  *pMem  /* pointer to dynamically-allocated object */
    )            
    {
    ...
    }

/*******************************************************************************
*
* operator;new - default run-time support for operator new (C++)
* 
* This function provides the default implementation of operator new.
* It allocates memory from the system memory partition for the
* requested object.  The value, when evaluated, is a pointer of the
* type `pointer-to-'<T> where <T> is the type of the new object.
* 
* If allocation fails a new-handler, if one is defined, is called.
* If the new-handler returns, presumably after attempting to recover
* from the memory allocation failure, allocation is retried.
* If there is no new-handler an exception of type "bad_alloc" is
* thrown.
*
* RETURNS:
* Pointer to new object.
*
* THROWS:
* std::bad_alloc if allocation failed.
*/

extern void * operator new 
    (
    size_t n  /* size of object to allocate */
    ) throw (std::bad_alloc)
    {
    ...
    }
/*******************************************************************************
*
* operator;new - default run-time support for operator new (nothrow) (C++)
* 
* This function provides the default implementation of operator new (nothrow).
* It allocates memory from the system memory partition for the
* requested object.  The value, when evaluated, is a pointer of the
* type `pointer-to-'<T> where <T> is the type of the new object.
* 
* If allocation fails, a new-handler, if one is defined, is called.
* If the new-handler returns, presumably after attempting to recover
* from the memory allocation failure, allocation is retried.
* If the new_handler throws a bad_alloc exception, the exception
* is caught and 0 is returned. 
* If allocation fails and there is no new_handler 0 is returned.
*
* RETURNS:
* Pointer to new object or 0 if allocation fails.
*
* INCLUDE FILES: `new'
*
*/

extern void * operator new 
    (
    size_t n,             /* size of object to allocate */
	const nothrow_t &      /* supply argument of "nothrow" here */
    ) throw ()
    {
    ...
    }

/*******************************************************************************
*
* operator;new - run-time support for operator new with placement (C++)
*
* This function provides the default implementation of the global
* new operator, with support for the placement syntax.
* New-with-placement is used to initialize objects for which memory has
* already been allocated. <pMem> points to the previously allocated memory.
* memory.
*
* RETURNS:
* <pMem>
*
* INCLUDE FILES: `new'
*
*/

extern void * operator new
    (               
    size_t	n,	/* size of object to allocate (unused) */
    void *	pMem	/* pointer to allocated memory         */
    )               
    {
    ...
    }

/*******************************************************************************
*
* set_new_handler - set new_handler to user-defined function (C++)
*
* This function is used to define the function that will be called
* when operator new cannot allocate memory.
*
* The new_handler acts for all threads in the system;
* you cannot set a different handler for different tasks.
*
* RETURNS: A pointer to the previous value of new_handler.
*
* INCLUDE FILES: `new'
*
*/

extern void (*set_new_handler (void(* pNewNewHandler)())) ()
    {
    ...
    }

/*******************************************************************************
*
* set_terminate - set terminate to user-defined function (C++)
*
* This function is used to define the terminate_handler
* which will be called when an uncaught exception is raised.
*
* The terminate_handler acts for all threads in the system;
* you cannot set a different handler for different tasks.
*
* RETURNS: The previous terminate_handler.
*
* INCLUDE FILES: `exception'
*/

extern void (*set_terminate (void(* terminate_handler)())) ()
    {
    ...
    }

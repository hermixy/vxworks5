/* comSysLib.h - VxWorks VxDCOM cross-OS support */

/* Copyright (c) 1998 Wind River Systems, Inc. */

/*

modification history
--------------------
01x,13jul01,dbs  move some macros out to comMisc.h
01w,27jun01,dbs  add realloc function
01v,21jun01,nel  Add GUID and time prototypes.
01u,21jun01,dbs  fix up new name and C compilation
01t,19aug99,aim  removed TASK_SPAWN
01s,19aug99,aim  removed TASK_SLEEP and TASK_LOCK macros
01r,19aug99,aim  change assert to VXDCOM_ASSERT
01q,13aug99,aim  added ARG_UNUSED macro
01p,16jul99,aim  remove #undef Free
01o,17jun99,aim  fix assert definition
01n,17jun99,aim  added DECLARE_IUNKNOWN_METHODS
01m,17jun99,dbs  make sure Allocator is included for target build
01l,10jun99,dbs  fix NEW macro for Solaris
01k,10jun99,dbs  move all vxdcom-provate calls into here
01j,09jun99,aim  fix assert again
01i,09jun99,aim  fix DELZERO macro
01h,09jun99,aim  added DELZERO
01g,03jun99,dbs  fix W32 Sleep
01f,03jun99,dbs  fix UNLOCK for W32
01e,02jun99,aim  changes for solaris build
01d,02jun99,dbs  move all OS-specific into here
01c,27may99,dbs  change name, add more target-specific stuff
01b,22apr99,dbs  add task-priority to START_TASK macro
01a,20apr99,dbs  created during Grand Renaming
*/

/*

DESCRIPTION:

This file defines OS-specific features to allow VxCOM / VxDCOM to be
built on multiple 'target's. One of the macros VXDCOM_PLATFORM_XXXX
will be set on the make command line, with 'XXXX' being one of the
following values:-

  VXWORKS -- all VxWorks targets including simulators
  SOLARIS -- Sun/Solaris test-build environment
  LINUX   -- Linux test-build environment
  
  WIN32   -- (OBSOLETE) Win32 test-build environment
  
*/


#ifndef __INCcomSysLib_h
#define __INCcomSysLib_h

#ifdef __cplusplus
extern "C" {
#endif
    
/* VxCOM private functions for OS-agnosticism... */

unsigned long	comSysLocalGet (void);
void		comSysLocalSet (unsigned long);
void*   	comSysAlloc (unsigned long);
void*   	comSysRealloc (void*, unsigned long);
void    	comSysFree (void*);
int     	comSysAddressGet (unsigned char*);
void		comSysGuidCreate (void * result);

unsigned long 	comSysTimeGet (void);

/* Generic internal mem-allocation functions/macros... */

#define COM_MEM_ALLOC(nb)  comSysAlloc (nb)
#define COM_MEM_FREE(pv)   comSysFree (pv)

/* Platform-specific specialisations... */

#ifndef TRUE
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#define ARG_UNUSED(A) {if (&A) /* no-op */ ;}

#ifdef __cplusplus
}
#endif

#endif /* __INCcomSysLib_h */

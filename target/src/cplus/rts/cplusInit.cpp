/* cplusInit.C - initialize run-time support for C++  */

/* Copyright 1993 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02h,19mar02,sn   SPR 71699 - decouple from cplusUsr.o; general clean up
02g,06nov01,sn   include <exception> rather than prototyping set_terminate
                 (fixed Diab build failure)
02f,11oct00,sn   break out INCLUDE_CPLUS_LIBGCC as a seperate (but required!) 
                 component
02e,11oct98,ms   reworked 02d fix, since it was causing bootrom builds to fail
02d,08oct98,sn   moved defn of cplusXtorStrategy from cplusUsr.cpp to here
02c,17sep98,ms   removed force-links of loader support modules
02b,16jan98,sn   add force-link of 'new-style' new/delete
02a,15jan98,sn   add force-link of exception handling/RTTI
01f,20jun95,srh  remove force-link of iostreams.
01e,18jun95,srh  change cplusCfront to cplusCore.
01d,18jun93,jdi  more doc cleanup.
01c,03jun93,srh  doc cleanup
01b,23apr93,srh  implemented new force-link/initialization scheme
01a,23apr93,srh  written.
*/

/*
DESCRIPTION
This module defines cplusLibInit, and implements a scheme that forces
most of the WindC++ run-time support to be linked with VxWorks.
Initialization is actually performed by cplusLibMinInit(), which is
defined elsewhere.

NOMANUAL
*/

/* Includes */

#include "vxWorks.h"
#include "cplusLib.h"
#include <exception>

/* Defines */

/* Globals */

/* Locals */

extern char __cplusCore_o;
extern char __cplusStr_o;
extern char __cplusXtors_o;

char * __cplusObjFiles [] =
    {
    & __cplusCore_o,
    & __cplusStr_o,
    & __cplusXtors_o
    };

/* Forward declarations */

/*******************************************************************************
*
* cplusLibInit - initialize the C++ library (C++)
*
* This routine initializes the C++ library and forces all C++ run-time support
* to be linked with the bootable VxWorks image.  If INCLUDE_CPLUS is defined
* in configAll.h, cplusLibInit() is called automatically from the root task,
* usrRoot(), in usrConfig.c.
*
* RETURNS: OK or ERROR.
*/

extern void cplusTerminate ();

extern "C" STATUS cplusLibInit (void)
    {
/* We do this here to avoid pulling in exception handling code with
 * INCLUDE_CPLUS_MIN
 */
    set_terminate (cplusTerminate);
    return cplusLibMinInit ();
    }


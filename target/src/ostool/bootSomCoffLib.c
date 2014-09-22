/* bootSomCoffLib.c - HP-PA SOM COFF object module boot loader */

/* Copyright 1984-1993 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01a,08nov93,gae  created from bootAoutLib.c
*/

/*
DESCRIPTION
This library provides an object module boot loading facility for 
the HP-PA SOM COFF object module format. 

SEE ALSO: bootLoadLib
.pG "Basic OS"
*/

/* includes */

#include "vxWorks.h"
#include "som_coff.h"
#include "bootLoadLib.h"


/*******************************************************************************
*
* bootSomCoffModule - load an HP-PA SOM COFF object module into memory
*
* This routine loads an object module in HP-PA SOM format from the specified
* file, and places the code, data, and BSS at the locations specified within
* the file.  The entry point of the module is returned in <pEntry>.  This 
* routine is generally used for bootstrap loading.
*
* RETURNS:
* ERROR not implemented
*
* SEE ALSO: loadModuleAt()
*/

LOCAL STATUS bootSomCoffModule 
    (
    int fd,
    FUNCPTR *pEntry 			/* entry point of module */
    )
    {
    return (ERROR);
    }

/********************************************************************************
* bootSomCoffInit - initialize the system for load modules
*
* This routine initialized VxWorks to use an SOM COFF format for
* boot loading modules.
*
* RETURNS:
* OK
*
* SEE ALSO: loadModuleAt()
*/

STATUS bootSomCoffInit
    (
    )
    {
    /* XXX check for installed ? */
    bootLoadRoutine = bootSomCoffModule;
    return (OK);
    }

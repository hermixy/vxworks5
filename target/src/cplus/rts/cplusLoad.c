/* cplusLoad.c - load-time functions for C++  */

/* Copyright 1993 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01d,19mar02,sn   SPR 71699 - Pull in cplusUsr.o if cplusLoad.o is included
01c,22jan02,sn   Changed to C file
01b,03jun93,srh  doc cleanup
01a,21apr93,srh  split out from cplusLib.C
*/

/*
DESCRIPTION
This module supports the module loader. It contains no user-callable routines

NOMANUAL
*/

/* Includes */

#include "vxWorks.h"
#include "string.h"
#include "cplusLib.h"
#include "taskLib.h"

/* Defines */

/* Globals */

extern char __cplusUsr_o;
char __cplusLoad_o = 0;

char * __cplusLoadObjFiles [] =
    {
    & __cplusUsr_o
    };

/* Locals */

/* Forward declarations */

/*******************************************************************************
*
* findXtors - find initializers for module's _ctors and _dtors members
*
* This function is used by cplusLoadFixup. It called by symEach to
* scan the symbol table for ctors and dtors arrays, as generated
* by the munch utility.
*
* RETURNS: FALSE after values are found for both ctors and dtors,
*          otherwise TRUE.
*
* NOMANUAL
*/

LOCAL BOOL findXtors
    (
    char *name,
    int val,
    SYM_TYPE dummy,
    int module_arg,
    UINT16 group
    )
    {
    MODULE_ID module = (MODULE_ID) module_arg;
    
    /* First check to see if this symbol belongs to the module of interest */
    if (group == module->group)
	{
	/*
	 * This symbol belongs to the current module, so check to
	 * see if it is a ctors or dtors array, and if so, fill out the module
	 */
	if ((strcmp (name, "__ctors") == 0)
	    ||
	    (strcmp (name, "_ctors") == 0))
	    {
	    module->ctors = (VOIDFUNCPTR *) val;
	    }
	else if ((strcmp (name, "__dtors") == 0)
		 ||
		 (strcmp (name, "_dtors") == 0))
	    {
	    module->dtors = (VOIDFUNCPTR *) val;
	    }
	}

    /* Stop scanning iff we've found both ctors and dtors */
    if ((module->ctors == 0) || (module->dtors == 0))
	{
	return TRUE;
	}
    return FALSE;
    }
/*******************************************************************************
*
* cplusLoadFixup - post-load module manipulations for C++
*
* Search the given module for _ctors and _dtors. These symbols indicate
* lists pointers to cfront-composed functions that call constructors
* and destructors for a module's static objects. If matching symbols
* are found, set the module's ctors and dtors members to the symbols' values.
* Otherwise set to 0.
*
* RETURNS: OK
*
* NOMANUAL
*/

STATUS cplusLoadFixup
    (
    MODULE_ID		module,			/* just-loaded module */
    int			dummy,			/* which symbols were added */
    SYMTAB_ID		symTab			/* which symbol table to use */
    )

    {
    module->ctors = 0;
    module->dtors = 0;
    symEach (symTab, (FUNCPTR) findXtors, (int) module);

    if (module->ctors != 0 && cplusXtorStrategy == 1)
	{
	cplusCallCtors (module->ctors);
	}
    return OK;
    }
/*******************************************************************************
*
* cplusUnoadFixup - pre-unload module manipulations for C++
*
* Call static destructors if the current xtor strategy is automatic.
* Otherwise do nothing.
*
* RETURNS: OK
*
* NOMANUAL
*/

STATUS cplusUnloadFixup
    (
    MODULE_ID		module			/* module to be deleted */
    )

    {
    if (module && module->dtors && cplusXtorStrategy == 1)
	{
	cplusCallDtors (module->dtors);
	}
    return OK;
    }

/* taskHookShow.c - task hook show routines */

/* Copyright 1992-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01h,15oct01,jn   use symFindSymbol for symbol lookups (SPR #7453)
01g,17mar99,jdi  doc: updated w/ info about proj facility (SPR 25727).
01f,13feb93,kdl  changed cplusLib.h to private/cplusLibP.h (SPR #1917).
01e,02feb93,jdi  documentation tweaks.
01d,21jan93,jdi  documentation cleanup for 5.1.
01c,02aug92,srh  fixed declaration of nameToPrint
01b,01aug92,srh  added C++ demangling idiom to taskHookShow
01a,25jun92,jcf  written by extracting from taskHookLib.c.
*/

/*
This library provides routines which summarize the installed kernel
hook routines.  There is one routine dedicated to the display of each 
type of kernel hook:  task operation, task switch, and task deletion.

The routine taskHookShowInit() links the task hook show
facility into the VxWorks system.  It is called automatically when
this show facility is configured into VxWorks using either of the
following methods:
.iP
If you use the configuration header files, define
INCLUDE_SHOW_ROUTINES in config.h.
.iP
If you use the Tornado project facility, select INCLUDE_TASK_HOOK_SHOW.

INCLUDE FILES: taskHookLib.h

SEE ALSO: taskHookLib,
.pG "Basic OS"
*/

#include "vxWorks.h"
#include "taskHookLib.h"
#include "symLib.h"
#include "sysSymTbl.h"
#include "stdio.h"
#include "private/cplusLibP.h"
#include "private/taskLibP.h"
#include "private/funcBindP.h"

/* defines */

#define TASK_DEMANGLE_PRINT_LEN 256  /* Num chars of demangled names to print */

/* forward declarations */

static void taskHookShow (FUNCPTR table[], int maxEntries);


/*******************************************************************************
*
* taskHookShowInit - initialize the task hook show facility
*
* This routine links the task hook show facility into the VxWorks system.
* It is called automatically when the task hook show facility is
* configured into VxWorks using either of the following methods:
* .iP
* If you use the configuration header files, define
* INCLUDE_SHOW_ROUTINES in config.h.
* .iP
* If you use the Tornado project facility, select INCLUDE_TASK_HOOK_SHOW.
*
* RETURNS: N/A
*/

void taskHookShowInit (void)
    {
    }
/*******************************************************************************
*
* taskCreateHookShow - show the list of task create routines
*
* This routine shows all the task create routines installed in the task
* create hook table, in the order in which they were installed.
*
* RETURNS: N/A
*
* SEE ALSO: taskCreateHookAdd()
*/

void taskCreateHookShow (void)
    {
    taskHookShow (taskCreateTable, VX_MAX_TASK_CREATE_RTNS);
    }
/*******************************************************************************
*
* taskSwitchHookShow - show the list of task switch routines
*
* This routine shows all the switch routines installed in the task
* switch hook table, in the order in which they were installed.
*
* RETURNS: N/A
*
* SEE ALSO: taskSwitchHookAdd()
*/

void taskSwitchHookShow (void)
    {
    taskHookShow (taskSwitchTable, VX_MAX_TASK_SWITCH_RTNS);
    }
/*******************************************************************************
*
* taskSwapHookShow - show the list of task switch routines
*
* This routine shows all the switch routines installed in the task
* switch hook table, in the order in which they were installed.
*
* RETURNS: N/A.
*
* NOMANUAL
*/

void taskSwapHookShow (void)
    {
    taskHookShow (taskSwapTable, VX_MAX_TASK_SWAP_RTNS);
    }
/*******************************************************************************
*
* taskDeleteHookShow - show the list of task delete routines
*
* This routine shows all the delete routines installed in the task delete
* hook table, in the order in which they were installed.  Note that the
* delete routines will be run in reverse of the order in which they were
* installed.
*
* RETURNS: N/A
*
* SEE ALSO: taskDeleteHookAdd()
*/

void taskDeleteHookShow (void)
    {
    taskHookShow (taskDeleteTable, VX_MAX_TASK_DELETE_RTNS);
    }
/*******************************************************************************
*
* taskHookShow - show the hooks in a hook table
*
* Shows the contents of a hook table symbolically.
*
* RETURNS: N/A.
*/

LOCAL void taskHookShow
    (
    FUNCPTR	table[],	/* table from which to delete */
    int		maxEntries	/* max entries in table */
    )
    {
    FAST      int ix;
    char *    name;	        /* pointer to symTbl copy of name */
    int       displacement;
    void *    symValue;         /* actual symbol value */
    SYMBOL_ID symId;            /* symbol identifier   */

    char     demangled[TASK_DEMANGLE_PRINT_LEN+1];
    char *   nameToPrint;

    /* 
     * Only check one symLib function pointer (for performance's sake). 
     * All symLib functions are provided by the same library, by convention.    
     */

    if ((_func_symFindSymbol !=(FUNCPTR) NULL) &&
	(sysSymTbl != NULL))
        {
	for (ix = 0; ix < maxEntries; ++ix)
	    {
	    if (table [ix] == NULL)
	        break;

	    if (((* _func_symFindSymbol) (sysSymTbl,  NULL, 
					 (void *)table[ix], 
					 SYM_MASK_NONE, SYM_MASK_NONE, 
					 &symId) == OK) && 
		((* _func_symNameGet) (symId, &name) == OK) && 
		((* _func_symValueGet) (symId, &symValue) == OK)) 
	        {
		nameToPrint = cplusDemangle (name, demangled, 
					     sizeof (demangled));
		printf ("%-15s", nameToPrint);
		if ((displacement = (int)table[ix] - (int)symValue) != 0)
		    printf ("+0x%-4x", displacement);
		else
		    printf ("%6s", "");         /* no displacement */
		}
	    printf ("\n");
	    }
	}
    else
        {
	for (ix = 0; ix < maxEntries; ++ix)
	    {
	    if (table [ix] == NULL)
	        break;

	    printf ("%#21x", table [ix]);
	    printf ("\n");
	    }
	}
    }

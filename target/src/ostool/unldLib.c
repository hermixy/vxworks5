/* unldLib.c - object module unloading library */

/* Copyright 1992-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01r,15may02,jn   improve explanation of shell commands
01q,18mar02,jn   remove dependence on usrLib (call to ld)
01p,30oct96,elp  Added syncUnldRtn function pointer (symtbls synchro).
01o,09aug96,dbt  check segment size in macro IN_TEXT_SEGMENT (SPR #3908).
		 Updated copyright.
01n,10oct95,jdi  doc: added .tG Shell to unld(); changed .pG Cross-Dev to .tG.
01m,11feb95,jdi  doc format repair.
01l,02nov93,caf  made unldTextInUse() static, like its prototype.
01l,20may94,dvs  fixed references to unallocated memory in reld() (SPR 3005)
01k,25aug93,srh  added call to C++ run-time for static dtors (spr 2341)
01j,22aug93,jmm  added call to bdall() from unld{ByModuleId} ()
01i,18aug93,jmm  removed checking of ether hook routines for unld
01h,13aug93,jmm  added code to test hook routines before unld() (spr 2054)
01g,25nov92,jdi  documentation cleanup.
01f,30oct92,jmm  fixed adding "/" separator in reld() (spr 1717)
01e,16sep92,kdl  added "/" separator when combining path & name in reld().
01d,02sep92,jmm  changed name of symFlags field in reld()
01c,29jul92,jmm  cleaned up warning messages
01b,21jul92,rdc  mods to support text segment write protection.
01a,26may92,jmm  written
*/

/*
DESCRIPTION
This library provides a facility for unloading object modules.  Once an
object module has been loaded into the system (using the facilities
provided by loadLib), it can be removed from the system by calling one
of the unld...() routines in this library.

Unloading of an object module does the following:
.IP (1) 4
It frees the space allocated for text, data,
and BSS segments, unless loadModuleAt() was called with specific
addresses, in which case the user is responsible for freeing the space.
.IP (2)
It removes all symbols associated with the object module from the
system symbol table.
.IP (3)
It removes the module descriptor from the module list.
.LP

Once the module is unloaded, any calls to routines in that module from
other modules will fail unpredictably.  The user is responsible for
ensuring that no modules are unloaded that are used by other modules.
unld() checks the hooks created by the following routines to
ensure none of the unloaded code is in use by a hook:

    taskCreateHookAdd()
    taskDeleteHookAdd()
    taskHookAdd()
    taskSwapHookAdd()
    taskSwitchHookAdd()

However, unld() \f2does not\fP check the hooks created by these routines:

    etherInputHookAdd()
    etherOutputHookAdd()
    excHookAdd()
    rebootHookAdd()
    moduleCreateHookAdd()

The routines unld() and reld() are 'shell commands'.  That is,
they are designed to be used only in the shell, and not in code
running on the target.  In future releases, calling unld() and 
reld() directly from code may not be supported.

INCLUDE FILES: unldLib.h, moduleLib.h

SEE ALSO: loadLib, moduleLib,
.tG "Cross-Development" 
*/

/* LINTLIBRARY */

#include "vxWorks.h"

#include "a_out.h"
#include "errno.h"
#include "ioLib.h"
#include "loadLib.h"
#include "moduleLib.h"
#include "stdio.h"
#include "stdlib.h"
#include "symLib.h"
#include "symbol.h"
#include "sysSymTbl.h"
#include "unldLib.h"
#include "private/vmLibP.h"
#include "string.h"
#include "usrLib.h"
#include "etherLib.h"
#include "private/cplusLibP.h"
#include "private/taskLibP.h"
#include "private/funcBindP.h"

#define IN_TEXT_SEGMENT(adrs, segment) (((void *) (adrs) >= (segment)->address) && \
					(((void *) (adrs) <= (void *) (((int) (segment)->address) + (segment)->size))) && \
					(segment->size != 0))

/* globals */

FUNCPTR	syncUnldRtn = (FUNCPTR)NULL;

/* locals */

/* forward static functions */
 
static BOOL unldSymbols (char *name, int val, SYM_TYPE type, int deleteGroup,
			 UINT16 group, SYMBOL *pSymbol);
static STATUS unldTextInUse (MODULE_ID moduleId);
 
/******************************************************************************
*
* unld - unload an object module by specifying a file name or module ID
* 
* This routine unloads the specified object module from the system.  The
* module can be specified by name or by module ID.  For a.out and ECOFF
* format modules, unloading does the following:
* .IP (1) 4
* It frees the space allocated for text, data,
* and BSS segments, unless loadModuleAt() was called with specific
* addresses, in which case the user is responsible for freeing the space.
* .IP (2)
* It removes all symbols associated with the object module from the
* system symbol table.
* .IP (3)
* It removes the module descriptor from the module list.
* .LP
*
* For other modules of other formats, unloading has similar effects.
*
* Before any modules are unloaded, all breakpoints in the system are deleted.
* If you need to keep breakpoints, set the options parameter to
* UNLD_KEEP_BREAKPOINTS.  No breakpoints can be set in code that is unloaded.
* 
* This routine is a 'shell command'.  That is, it is designed to be
* used only in the shell, and not in code running on the target.  In
* future releases, calling unld() directly from code may not be
* supported.
*
* RETURNS: OK or ERROR.
*
* SEE ALSO:
* .pG "Target Shell,"
* windsh,
* .tG "Shell" 
*/

STATUS unld
    (
    void *	nameOrId,	/* name or ID of the object module file */
    int 	options		
    )
    {
    MODULE_ID 	moduleId;

    /* Given the name, find the right module id, then call unldByModuleId(). */

    if (((moduleId = moduleIdFigure (nameOrId))) != NULL)
        return (unldByModuleId (moduleId, options));
    else
        return (ERROR);
    }

/******************************************************************************
*
* unldByModuleId - unload an object module by specifying a module ID
*
* This routine unloads an object module that has a module ID
* matching <moduleId>.
*
* See the manual entries for unld() or unldLib for more information on
* module unloading.
* 
* RETURNS: OK or ERROR.
* 
* SEE ALSO: unld()
*/

STATUS unldByModuleId
    (
    MODULE_ID 	moduleId,      	/* module ID to unload */
    int       	options		
    )
    {
    SEGMENT_ID 	segmentId;
    int 	pageSize;
    int 	segSize;
    MODULE	syncBuf;

    /* store unloaded module info to be able to synchronize host symtbl */

    if (syncUnldRtn != NULL)
	memcpy (&syncBuf, moduleId, sizeof (MODULE));

    /* Make sure the text segment isn't in use */

    if (unldTextInUse (moduleId) != OK)
        return (ERROR);
    
    /* Give C++ run-time a chance to call static destructors */

    cplusUnloadFixup (moduleId);

    /* If options isn't set to 1, delete all breakpoints in the system */

    if (_func_bdall != NULL && !(options & UNLD_KEEP_BREAKPOINTS))
        (*_func_bdall) (0);
    
    /* Free all segments */

    while ((segmentId = moduleSegGet (moduleId)))
	{

	/*
	 * If the segment is marked to dynamically free space, do so
	 */

	if (segmentId->flags & SEG_FREE_MEMORY)
	    {
	    /* make sure it's not write protected */

	    if (segmentId->flags & SEG_WRITE_PROTECTION)
		{
		pageSize = VM_PAGE_SIZE_GET ();

		/* round seg size to intergral num pages */

		segSize = segmentId->size / pageSize * pageSize + pageSize;

		if (VM_STATE_SET (NULL, segmentId->address, segSize, 
		    VM_STATE_MASK_WRITABLE, VM_STATE_WRITABLE) == ERROR)
		    return (ERROR);
		}

	    free (segmentId->address);
	    }

	free (segmentId);
	}

    /*
     * Remove all symbols from the symbol table that match the group
     * ID #.  If they're common symbols, delete their storage space.
     */

    symEach (sysSymTbl, (FUNCPTR) unldSymbols, (int) moduleId->group);

    /* Remove the module itself */

    if (moduleDelete (moduleId) != OK)
	{
	return (ERROR);
	}

    if ((~options & UNLD_SYNC) && (syncUnldRtn != NULL))
	(* syncUnldRtn) (&syncBuf);

    return (OK);
    }

/******************************************************************************
*
* unldByNameAndPath - unload an object module by specifying a name and path
*
* This routine unloads an object module specified by <name> and <path>.
*
* See the manual entries for unld() or unldLib for more information on
* module unloading.
* 
* RETURNS: OK or ERROR.
* 
* SEE ALSO: unld()
*/

STATUS unldByNameAndPath
    (
    char * name,		/* name of the object module to unload */
    char * path,		/* path to the object module to unload */
    int    options		/* options, currently unused */
    )
    {
    MODULE_ID moduleId;

    /*
     * Given the name and path, find the right module id, then call
     * unldByModuleId().
     */

    if ((moduleId = moduleFindByNameAndPath (name, path)) != NULL)
        return (unldByModuleId (moduleId, options));
    else
        return (ERROR);
    }

/******************************************************************************
*
* unldByGroup - unload an object module by specifying a group number
*
* This routine unloads an object module that has a group number
* matching <group>.
*
* See the manual entries for unld() or unldLib for more information on
* module unloading.
* 
* RETURNS: OK or ERROR.
* 
* SEE ALSO: unld()
*/

STATUS unldByGroup
    (
    UINT16	group,		/* group number to unload */
    int  	options		/* options, currently unused */
    )
    {
    MODULE_ID 	moduleId;

    /*
     * Given the name, find the right module id, then call unldByModuleId().
     */

    if ((moduleId = moduleFindByGroup (group)) != NULL)
        return (unldByModuleId (moduleId, options));
    else
        return (ERROR);
    }


/******************************************************************************
*
* unldSymbols - support routine for unldByModuleId
*
* This routine is used by unldByModuleId() as an argument to symEach().
* 
* NOMANUAL
*/


LOCAL BOOL unldSymbols
    (
    char *	 name,		/* not used */
    int	   	 val,		/* not used */
    SYM_TYPE 	 type,		/* not used */
    int	 	 deleteGroup,	/* group number to delete */
    UINT16       group,		/* group number of current symbol*/
    SYMBOL *	 pSymbol	/* a pointer to the symbol itself */
    )
    {
    if (group == deleteGroup)
	{
	if (symTblRemove (sysSymTbl, pSymbol) != OK)
	    {
	    printf ("got bad symbol, %#x\n", (UINT) pSymbol);
	    return (FALSE);
	    }

	/*
	 * if the type is N_COMM, then free the memory associated
	 * with the symbol.
	 */

	if ((pSymbol->type & N_COMM) == N_COMM)
	    {
	    free (pSymbol->value);
	    }

	/* free the space allocated for the symbol itself */

	symFree (sysSymTbl, pSymbol);
        
	}

    return (TRUE);
    }

/******************************************************************************
*
* reld - reload an object module
*
* This routine unloads a specified object module from the system, and
* then calls loadModule() to load a new copy of the same name.
* 
* If the file was originally loaded using a complete pathname, then
* reld() will use the complete name to locate the file.  If
* the file was originally loaded using a partial pathname, then the 
* current working directory must be changed to the working
* directory in use at the time of the original load.
* 
* Valid values for the options parameter are the same as those allowed 
* for the function unld().  
* 
* This routine is a 'shell command'.  That is, it is designed to be
* used only in the shell, and not in code running on the target.  In
* future releases, calling reld() directly from code may not be
* supported.
*
* RETURNS: A module ID (type MODULE_ID), or NULL.
* 
* SEE ALSO: unld()
*/

MODULE_ID reld
    (
    void *	nameOrId,	/* name or ID of the object module file */
    int 	options		/* options used for unloading           */
    )
    {
    MODULE_ID 	moduleId;
    MODULE_ID 	newModuleId;
    char	fileName [NAME_MAX + PATH_MAX];
    int         fd;
    int		flags;
    
    /*
     * Given the name, find the right module id, then call unldByModuleId()
     * and loadModule().
     */

    if (((moduleId = moduleIdFigure (nameOrId))) != NULL)
	{
	/* build the new file name before destroying module */

	strcpy (fileName, moduleId->path);
	if (moduleId->path [0] != EOS)
	    strcat (fileName, "/");
	strcat (fileName, moduleId->name);

	flags = moduleId->flags;
	
        unldByModuleId (moduleId, options);

	/* reload module */

        fd = open (fileName, O_RDONLY, 0);

        if (fd == ERROR)
            return (NULL);

	newModuleId = loadModule (fd, flags);

        close (fd);

	return (newModuleId);
	}
    else
        return (NULL);
    }

/******************************************************************************
*
* unldTextSegmentCheck - check that text segment is not in use by hook routine
* 
* unldTextSegmentCheck returns TRUE if the segment is not a text segment,
* or if the text segment is not used by any hook routines.
* 
* RETURNS: TRUE, or FALSE if the text segment is in use by a hook routine.
* 
* NOMANUAL
*/

BOOL unldTextSegmentCheck
    (
    SEGMENT_ID 	 segmentId,
    MODULE_ID	 moduleId, 
    int		 unused
    )
    {
    int          hookNumber;

    /* If it's not a text segment, just return TRUE */
    
    if (segmentId->type != SEGMENT_TEXT)
        return (TRUE);

    /* The exception hook is a simple variable */

    if (IN_TEXT_SEGMENT (excExcepHook, segmentId))
        return (FALSE);

    /* check the various task hooks */

    for (hookNumber = 0; hookNumber < VX_MAX_TASK_CREATE_RTNS; hookNumber++)
        if (IN_TEXT_SEGMENT (taskCreateTable [hookNumber], segmentId))
	    return (FALSE);

    for (hookNumber = 0; hookNumber < VX_MAX_TASK_DELETE_RTNS; hookNumber++)
        if (IN_TEXT_SEGMENT (taskDeleteTable [hookNumber], segmentId))
	    return (FALSE);
    
    for (hookNumber = 0; hookNumber < VX_MAX_TASK_SWAP_RTNS; hookNumber++)
        if (IN_TEXT_SEGMENT (taskSwapTable [hookNumber], segmentId))
	    return (FALSE);
    
    for (hookNumber = 0; hookNumber < VX_MAX_TASK_SWITCH_RTNS; hookNumber++)
        if (IN_TEXT_SEGMENT (taskSwitchTable [hookNumber], segmentId))
	    return (FALSE);

    /* Didn't find anything, return TRUE */

    return (TRUE);
    }
    
/******************************************************************************
*
* unldTextInUse - check to see if text segment is used by a hook
* 
* unldTextSegment checks the system hook routines to see if any of them
* use text that is about to be unloaded.  Modules that have hooks using
* their text cannot be unloaded.
* 
* unldTextSegmentUsed checks the hooks created by the following routines:
* 
* STATUS taskCreateHookAdd
* STATUS taskDeleteHookAdd
* LOCAL STATUS taskHookAdd
* STATUS taskSwapHookAdd
* STATUS taskSwitchHookAdd
* 
* unldTextSegmentUsed DOES NOT check the hooks created by these routines:
*
* STATUS etherInputHookAdd
* etherOutputHookAdd
* void excHookAdd
* STATUS rebootHookAdd
* STATUS moduleCreateHookAdd
* 
* RETURNS:  OK, or ERROR if the text segment is in use.
* 
* NOMANUAL
*/

static STATUS unldTextInUse
    (
    MODULE_ID 	moduleId      	/* module ID to check */
    )
    {
    if (moduleSegEach (moduleId, unldTextSegmentCheck, 0) != NULL)
	{
	errno = S_unldLib_TEXT_IN_USE;
        return (ERROR);
	}
    else
        return (OK);
    }

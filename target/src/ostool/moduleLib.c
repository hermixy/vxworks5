/* moduleLib.c - object module management library */
 
/* Copyright 1992-1996 Wind River Systems, Inc. */
#include "copyright_wrs.h"
 
/*
modification history
--------------------
01r,15mar99,c_c  Doc: updated moduleSegAdd () (SPR #6138).
01q,27sep96,pad  reinstated a line I deleted accidentally.
01p,11sep96,pad  strengthened group number handling (SPR #7133).
01o,10oct95,jdi  doc: added .tG Shell to SEE ALSO for moduleShow();
		 changed .pG Cross-Dev to .tG; corrections to moduleCreate().
01n,14mar93,jdi  fixed documentation for moduleDelete().
01m,15feb93,kdl  changed documentation for moduleFlagsGet().
01l,12feb93,jmm  changed documentation for moduleDelete ()
01k,27nov92,jdi  documentation cleanup.
01j,30oct92,jmm  moduleSegInfoGet() now zeros out info structure (spr 1702)
                 moduleFindByName () gets just the name part of moduleName
		     (spr 1718)
01i,14oct92,jdi  made moduleInit() and moduleTerminate() NOMANUAL.
01h,12oct92,jdi  fixed mangen problem in moduleCreateHookDelete().
01g,27aug92,jmm  changed moduleSegInfoGet() to use new MODULE_SEG_INFO struct
                 changed symFlags to flags
		 added moduleNameGet(), moduleFlagsGet()
		 set ctors and dtors to NULL in moduleInit()
01f,31jul92,jmm  cleaned up forward declarations
01e,30jul92,smb  changed format for printf to avoid zero padding.
01d,28jul92,jmm  made NODE_TO_ID macro portable to i960
01c,20jul92,jmm  removed checksum() routine (duplicate of routine in cksumLib.c)
                 removed bzero() of segment id from moduleSegGet()
		 added check for HIDDEN_MODULE flag to moduleDisplayGeneric()
01b,18jul92,smb  Changed errno.h to errnoLib.h.
01a,jmm,01may92  written 
*/
 
/*
DESCRIPTION
This library is a class manager, using the standard VxWorks class/object
facilities.  The library is used to keep track of which object modules
have been loaded into VxWorks, to maintain information about object
module segments associated with each module, and to track which
symbols belong to which module.  Tracking modules makes it possible to
list which modules are currently loaded, and to unload them when
they are no longer needed.

The module object contains the following information:

    - name
    - linked list of segments, including base addresses
      and sizes
    - symbol group number
    - format of the object module (a.out, COFF, ECOFF, etc.)
    - the <symFlag> passed to ld() when the module was
      loaded.  (For more information about <symFlag> and the
      loader, see the manual entry for loadLib.)

Multiple modules with the same name are allowed (the same module may
be loaded without first being unloaded) but "find" functions find the
most recently created module.

The symbol group number is a unique number for each module, used to
identify the module's symbols in the symbol table.  This number is
assigned by moduleLib when a module is created.

In general, users will not access these routines directly, with the
exception of moduleShow(), which displays information about currently
loaded modules.  Most calls to this library will be from routines in
loadLib and unldLib.

INCLUDE FILES: moduleLib.h

SEE ALSO: loadLib,
.tG "Cross-Development"
*/

#include "vxWorks.h"
#include "classLib.h"
#include "dllLib.h"
#include "errnoLib.h"
#include "moduleLib.h"
#include "objLib.h"
#include "pathLib.h"
#include "semLib.h"
#include "stdio.h"
#include "stdlib.h"
#include "stddef.h"
#include "string.h"
#include "vxLib.h"
#include "loadLib.h"

#define NODE_TO_ID(pNode) (pNode != NULL ? (MODULE_ID) ((char *) pNode - \
				         offsetof (MODULE, moduleNode)) : NULL)

typedef struct
    {
    DL_NODE node;
    FUNCPTR func;
    } MODCREATE_HOOK;

/* forward declarations */
 
LOCAL STATUS moduleDestroy (MODULE_ID moduleId, BOOL dealloc);
LOCAL STATUS moduleDisplayGeneric (MODULE_ID moduleId, UINT options);
LOCAL STATUS moduleSegInfoGet (MODULE_ID moduleId,
			       MODULE_SEG_INFO *pModSegInfo);
LOCAL BOOL   moduleCheckSegment (SEGMENT_ID segmentId);
LOCAL BOOL   moduleCheckOne (MODULE_ID moduleId, int options);
LOCAL STATUS moduleInsert (MODULE_ID newModule);


/* LOCALS */

static BOOL	 moduleLibInitialized = FALSE; /* prevent multiple inits */

LOCAL OBJ_CLASS	 moduleClass;

LOCAL DL_LIST 	 moduleCreateHookList;
LOCAL SEM_ID	 moduleCreateHookSem;
LOCAL BOOL 	 moduleCreateHookInitialized 	= FALSE;

LOCAL BOOL  	 moduleLibDebug 		= FALSE;

LOCAL DL_LIST	 moduleList;	/* list of loaded modules */
LOCAL SEM_ID	 moduleListSem;	/* semaphore to protect moduleList */

LOCAL SEM_ID	 moduleSegSem;	/* semaphore to protect all segment lists */

LOCAL FUNCPTR 	 moduleDisplayRtn;

/* GLOBALS */

CLASS_ID moduleClassId = &moduleClass;

/******************************************************************************
*
* moduleLibInit - initialize object module library
* 
* This routine initializes the object module tracking facility.  It
* should be called once from usrRoot().
* 
* RETURNS: OK or ERROR.
* 
* NOMANUAL
*/

STATUS moduleLibInit
    (
    void
    )
    {
    STATUS      returnValue;	/* function return value */

    /*
     * If moduleLibInitialized has been set to TRUE, the library's already
     * initialized.  Return OK.
     */

    if (moduleLibInitialized)
        return (OK);

    /* Initialize the module list and its protecting semaphore */

    moduleListSem = semMCreate (SEM_DELETE_SAFE);
    dllInit (&moduleList);

    /* Initialize the semaphore protecting all segment lists */

    moduleSegSem = semMCreate (SEM_DELETE_SAFE);

    /* Initialize the create hook list */

    moduleCreateHookSem = semMCreate (SEM_DELETE_SAFE);
    dllInit (&moduleCreateHookList);
    moduleCreateHookInitialized = TRUE;

    /*
     * Initialize the display routine function pointer.
     * Currently, this is hard-coded to moduleDisplayGeneric().
     * At some future point, when we need to display other object
     * module formats, this will need to be changed.
     */

    moduleDisplayRtn = moduleDisplayGeneric;

    /* Initialize the module class */

    returnValue = classInit (moduleClassId, sizeof (MODULE),
			     OFFSET (MODULE, objCore),
			     (FUNCPTR) moduleCreate, (FUNCPTR) moduleInit,
			     (FUNCPTR) moduleDestroy);

    /* Library has been initialized, return */

    moduleLibInitialized = TRUE;

    return (returnValue);
    }

/******************************************************************************
*
* moduleCreate - create and initialize a module
* 
* This routine creates an object module descriptor.
* 
* The arguments specify the name of the object module file, 
* the object module format, and an argument specifying which symbols
* to add to the symbol table.  See the loadModuleAt() description of <symFlag>
* for possibles <flags> values.
* 
* Space for the new module is dynamically allocated.
* 
* RETURNS: MODULE_ID, or NULL if there is an error.
*
* SEE ALSO: loadModuleAt()
*/

MODULE_ID moduleCreate
    (
    char *	name,	 /* module name */
    int 	format,	 /* object module format */
    int		flags	 /* <symFlag> as passed to loader (see loadModuleAt()) */
    )
    {
    MODULE_ID	moduleId;

    /* Allocate the object, return NULL if objAlloc() fails */

    moduleId = objAlloc (moduleClassId);

    if (moduleId == NULL)
	{
	printf ("errno %#x\n", errnoGet());
        return (NULL);
	}

    /* Call moduleInit() to do the real work */

    if (moduleInit (moduleId, name, format, flags) != OK)
	{
	objFree (moduleClassId, (char *) moduleId);
        return (NULL);
	}

    return (moduleId);
    }

/******************************************************************************
*
* moduleInit - initialize a module
* 
* This routine initializes an object module descriptor.  Instead of
* dynamically allocating the space for the module descriptor,
* moduleInit() takes a pointer to an existing MODULE structure to initialize.
* The other arguments are identical to the moduleCreate() arguments.
* 
* RETURNS: OK, or ERROR.
*
* NOMANUAL
*/

STATUS moduleInit
    (
    MODULE_ID		moduleId,	/* ptr to module to initialize */
    char *		name,		/* module name */
    int 		format,		/* object module format */
    int			flags		/* symFlags as passed to loader */
    )
    {
    static UINT16 	nextGroupNumber = 1;	/* 1 is VxWorks */
    MODCREATE_HOOK *	createHookNode;
    MODULE_ID		oldModule;		/* ID of previous same module */
    MODULE_ID		lastModule;		/* ID of last module in list */

    /* Split the name into it's name and path components */

    pathSplit (name, moduleId->path, moduleId->name);

    /* see if the module was loaded earlier */

    semTake (moduleListSem, WAIT_FOREVER);

    if ((oldModule = moduleFindByName (moduleId->name)) != NULL)
	{
	/* we found an old module, mark it as obsolete */

	oldModule->flags |= MODULE_REPLACED;
	}

    /*
     * Today, due to a poor/incomplete implementation of the module group
     * concept, only 65534 modules can be loaded in a VxWorks session's life.
     * The group number counter was simply incremented at each load operation.
     * See SPR #7133 for more precisions on the troubles this brought.
     *
     * Reset the next group number counter to the highest value actually
     * available. This is a first step to work around SPR #7133.
     */

    if ((lastModule = NODE_TO_ID (DLL_LAST (&moduleList))) != NULL)
	{
	nextGroupNumber = lastModule->group;
	nextGroupNumber++;
	}
	
    /* Set the module's group number */

    if (nextGroupNumber < MODULE_GROUP_MAX)
	{
        moduleId->group = nextGroupNumber++;

	/* Add the module to the end of the module list */

	dllAdd (&moduleList, &moduleId->moduleNode);
	}
    else
	{
	/*
	 * When all group slots are apparently used, we must walk through the
	 * module list and try to find a free group number due to previous
	 * unloadings. This is the second step to work around SPR #7133.
	 */

	if (moduleInsert (moduleId) != OK)
	    {
            printf ("No free group number. Abort load operation.\n");
	    errnoSet (S_moduleLib_MAX_MODULES_LOADED);
            return (ERROR);
	    }
	}

    semGive (moduleListSem);
    
    /* Set the module format and flags */

    moduleId->flags = flags;
    moduleId->format = format;

    moduleId->ctors = NULL;
    moduleId->dtors = NULL;

    /* Initialize the segment list */

    if (dllInit (&moduleId->segmentList) != OK)
	return (ERROR);
    
    /* Initialize the object stuff */

    objCoreInit (&moduleId->objCore, moduleClassId);

    /* Call any moduleCreateHook routines */

    semTake (moduleCreateHookSem, WAIT_FOREVER);

    for (createHookNode = (MODCREATE_HOOK *) DLL_FIRST (&moduleCreateHookList);
	 createHookNode != NULL;
	 createHookNode = (MODCREATE_HOOK *) DLL_NEXT ((DL_NODE *)
						       createHookNode))
	{
	(* createHookNode->func) (moduleId);
	}

    semGive (moduleCreateHookSem);
    
    /* If debugging is on, print out the name of the module. */

    if (moduleLibDebug)
        printf ("Module %s, group %d added\n", moduleId->name, moduleId->group);

    return (OK);
    }

/******************************************************************************
*
* moduleInsert - insert the module in list when possible
*
* This routine walks the module list in order to find a hole in the group
* numbering. As soon as a free group number is found, the new module is
* inserted in the list and is given the available group number.  
*
* This routine should be called only when all the group numbers have been
* allocated (i.e. when the group number counter reaches its maximum).
*
* WARNING: the module list semaphore must be taken by the caller.
*
* RETURNS: OK or ERROR if the module could not be inserted.
*/

LOCAL STATUS moduleInsert
    (
    MODULE_ID 	newModule		/* module to insert */
    )
    {
    MODULE_ID 	currentModule;		/* module pointer */
    MODULE_ID 	nextModule;		/* module pointer */

    /* Go through the module list and try to insert the new module */

    for (currentModule = NODE_TO_ID (DLL_FIRST (&moduleList));
	 currentModule != NULL;
	 currentModule = NODE_TO_ID (DLL_NEXT (&currentModule->moduleNode)))
	{
	nextModule = NODE_TO_ID (DLL_NEXT (&currentModule->moduleNode));

	/* Check if we reached the end of the list: this means no room left */

	if (nextModule == NULL)
	    break;

	/* Insert the new module when there is a whole in the group numbers */

	if (nextModule->group > (currentModule->group + 1))
	    {
	    dllInsert (&moduleList, &currentModule->moduleNode,
		       &newModule->moduleNode);
	    newModule->group = currentModule->group + 1;
	    break;
	    }
	}

    if (nextModule == NULL)
	return (ERROR);
    else
	return (OK);
    }

/******************************************************************************
*
* moduleTerminate - terminate a module
* 
* This routine terminates a static object module descriptor that was
* initialized with moduleInit().
* 
* RETURNS: OK or ERROR.
* 
* NOMANUAL
*/

STATUS moduleTerminate
    (
    MODULE_ID moduleId		/* module to terminate */
    )
    {
    return (moduleDestroy (moduleId, FALSE));
    }

/******************************************************************************
*
* moduleDelete - delete module ID information (use unld() to reclaim space)
* 
* This routine deletes a module descriptor, freeing any space that was
* allocated for the use of the module ID.
*
* This routine does not free space allocated for the object module itself --
* this is done by unld().
* 
* RETURNS: OK or ERROR.
*/

STATUS moduleDelete
    (
    MODULE_ID moduleId		/* module to delete */
    )
    {
    return (moduleDestroy (moduleId, TRUE));
    }

/******************************************************************************
*
* moduleDestroy - destroy module
*
* This is the underlying routine for moduleDelete() and moduleTerminate().
* 
* RETURNS: OK or ERROR.
* 
* NOMANUAL
*/

LOCAL STATUS moduleDestroy
    (
    MODULE_ID 	moduleId,	/* module to destroy */
    BOOL      	dealloc		/* deallocate memory associated with module */
    )
    {
    SEGMENT_ID	pSegment;	/* temp. storage for deleting segments */

    /* Make sure moduleId is a real object of class moduleClassId */

    if (OBJ_VERIFY (moduleId, moduleClassId) != OK)  /* validate module id */
	{
	return (ERROR);
	}

    objCoreTerminate (&moduleId->objCore);	/* INVALIDATE */

    /* Remove the module from the module list */

    semTake (moduleListSem, WAIT_FOREVER);
    dllRemove (&moduleList, &moduleId->moduleNode);
    semGive (moduleListSem);

    /* Remove any segment records associated with the module. */

    while ((pSegment = moduleSegGet (moduleId)) != NULL)
        free (pSegment);

    /* Deallocate the module id object itself */

    if (dealloc)
	if (objFree (moduleClassId, (char *) moduleId) != OK)
	    return (ERROR);

    return (OK);
    }

/******************************************************************************
*
* moduleIdFigure - translate a module name or ID to a module ID
*
* Some routines take either a module ID (an integer) or a module name.  This
* routine determines whether the parameter is a module name or a module ID and
* returns the module ID. 
*
* RETURNS: A module ID (type MODULE_ID), or NULL.
*
* NOMANUAL
*/

MODULE_ID moduleIdFigure
    (
    void * 	moduleNameOrId	/* module name or module ID */
    )
    {
    MODULE_ID 	moduleId = NULL;	/* default is NULL */

    /*
     * Make sure we're not getting a NULL pointer, OBJ_VERIFY doesn't check
     * for that.
     */

    if (moduleNameOrId == NULL)
        return (NULL);
    
    /* If moduleNameOrId is a moduleId, just return it. */
    
    if (OBJ_VERIFY (moduleNameOrId, moduleClassId) == OK)
	{
	return ((MODULE_ID) moduleNameOrId);
	}
    
    /*
     * moduleNameOrId isn't a legitmate object - see if it's the name
     * of a module.  
     */

    if ((moduleId = moduleFindByName (moduleNameOrId)) == NULL)
	{

	/*
	 * It's neither an object module nor a module name.  Set errno and
	 * return NULL.
	 */

	errnoSet (S_moduleLib_MODULE_NOT_FOUND);
	return (NULL);
	}
    else
        return (moduleId);
    }

/******************************************************************************
*
* moduleShow - show the current status for all the loaded modules
*
* This routine displays a list of the currently loaded modules and
* some information about where the modules are loaded.
* 
* The specific information displayed depends on the format of the object
* modules.  In the case of a.out and ECOFF object modules, moduleShow() displays
* the start of the text, data, and BSS segments.
* 
* If moduleShow() is called with no arguments, a summary list of all
* loaded modules is displayed.  It can also be called with an argument,
* <moduleNameOrId>, which can be either the name of a loaded module or a
* module ID.  If it is called with either of these, more information
* about the specified module will be displayed.
* 
* RETURNS: OK or ERROR.
*
* SEE ALSO:
* .pG "Target Shell,"
* windsh,
* .tG "Shell"
*/

STATUS moduleShow
    (
    char * 		moduleNameOrId,	/* name or ID of the module to show */
    int			options		/* display options */
    )
    {
    MODULE_ID		moduleId;
    static char *	moduleShowHdr [] = {"\n\
MODULE NAME     MODULE ID  GROUP #    TEXT START DATA START  BSS START\n\
--------------- ---------- ---------- ---------- ---------- ----------\n"};


    /*
     * If moduleNameOrId isn't NULL, translate it into a module ID, and
     * display information about that specific module.
     * If it is NULL, set moduleId to NULL, and display info about all
     * loaded modules.
     */

    if (moduleNameOrId != NULL)
	{
        if ((moduleId = moduleIdFigure (moduleNameOrId)) == NULL)
	    {
	    /* moduleIdFigure couldn't find a match */

	    printf ("Module not found\n");
	    return (ERROR);
	    }
	}
    else
        moduleId = NULL;

    /* print the module display header */

    printf ("%s", moduleShowHdr [MODULE_A_OUT]);

    /*
     * If moduleId is a specific module ID, print information about that
     * specific module.  Otherwise, print the summary.
     */

    if (moduleId != NULL)
	{

	/*
	 * We've got a specific module to show.  If no options are specified,
	 * then display all information, otherwise use what's specified.
	 */

        moduleDisplayGeneric (moduleId,
			      options == 0 ? MODDISPLAY_ALL : options);
	}
    else
        {
	/* We need to display all the modules. */

	semTake (moduleListSem, WAIT_FOREVER);
	dllEach (&moduleList, moduleDisplayGeneric,
		 options | MODDISPLAY_IS_DLL_NODE);
	semGive (moduleListSem);
	}

    /*
     * There's nothing that can fail - always return OK
     */

    return (OK);
    }

/******************************************************************************
*
* moduleDisplayGeneric - support routine to show stats for an object module
*
* This routine is normally called via a dllEach() call from moduleShow.
* It prints a single line of information about the specified module.
* Note that the parameter is _not_ of type MODULE_ID, but rather is a
* DL_NODE *.
* 
* RETURNS: TRUE, or FALSE if display should be aborted.
* 
* NOMANUAL
*/

LOCAL STATUS moduleDisplayGeneric
    (
    MODULE_ID	moduleId,	/* pointer to the module node to display */
    UINT	options		/* display options */
    )
    {
    static char	moduleShowFmt[] = "%15s %#10x %10d %#10x %#10x %#10x\n";
    MODULE_SEG_INFO segInfo;    /* segment information */

    /* If MODDISPLAY_IS_DLL_NODE is set, need to convert node to module id */

    if (options & MODDISPLAY_IS_DLL_NODE)
        moduleId = NODE_TO_ID (moduleId);

    /* If HIDDEN_MODULE is set, return OK without displaying anything */

    if (moduleId->flags & HIDDEN_MODULE)
        return (TRUE);

    /* Return FALSE if we can't get information about the module segments */

    if (moduleSegInfoGet (moduleId, &segInfo) != OK)
	{
	printErr ("Can\'t get information about module %#x\n", moduleId);
        return (FALSE);
	}

    /*
     * Print out the module summary line.
     * We only want to print the file name, not the entire path.
     */

    printf (moduleShowFmt, moduleId->name, moduleId,
	    moduleId->group,
	    segInfo.textAddr,
	    segInfo.dataAddr,
	    segInfo.bssAddr);

    /*
     * Print out all the optional information
     */

    if (options & MODDISPLAY_CODESIZE)
	{
	/* Print specific information about the size of module's segments */

	printf ("\nSize of text segment:   %8d\n", segInfo.textSize);
	printf ("Size of data segment:   %8d\n", segInfo.dataSize);
	printf ("Size of bss  segment:   %8d\n", segInfo.bssSize);

	printf ("Total size          :   %8d\n\n",
		segInfo.textSize + segInfo.dataSize
		+ segInfo.bssSize);
	}

    /*
     * Return TRUE to continue displaying modules
     */

    return (TRUE);
    }

/******************************************************************************
*
* moduleSegAdd - add a segment to a module
* 
* This routine adds segment information to an object module descriptor.  The
* specific information recorded depends on the format of the object module.
* 
* The <type> parameter is one of the following:
* .iP SEGMENT_TEXT 25
* The segment is for TEXT.
* .iP SEGMENT_DATA
* The segment is for DATA
* .iP SEGMENT_BSS
* The segment is for BSS
* 
* NOMANUAL
* 
* RETURNS: OK, or ERROR.
*/

STATUS moduleSegAdd
    (
    MODULE_ID	moduleId,	/* module to add segment to*/
    int		type,		/* segment type */
    void *	location,	/* segment address */
    int		length,		/* segment length */
    int		flags		/* segment flags */
    )
    {
    SEGMENT_ID	seg;

    /*
     * Validate module id
     */

    if (OBJ_VERIFY (moduleId, moduleClassId) != OK)
	{
	return (ERROR);
	}
	    
    /*
     * Allocate space for the segment record
     */

    seg = (SEGMENT_ID) malloc (sizeof (*seg));

    if (seg == NULL)
	{
        return (ERROR);
	}

    /*
     * Set the fields of the segment record
     */

    seg->address	= location;
    seg->size		= length;
    seg->type		= type;
    seg->flags		= flags;
    seg->checksum	= checksum (location, length);

    /*
     * Add the segment to the module's segment list
     */

    semTake (moduleSegSem, WAIT_FOREVER);
    dllAdd (&moduleId->segmentList, (DL_NODE *) seg);
    semGive (moduleSegSem);

    return (OK);
    }

/******************************************************************************
*
* moduleSegGet - get (delete and return) the first segment from a module
* 
* This routine returns information about the first segment of a module
* descriptor, and then deletes the segment from the module.
* 
* RETURNS: A pointer to the segment ID, or NULL if the segment list is empty.
*
* SEE ALSO: moduleSegFirst()
*/

SEGMENT_ID moduleSegGet
    (
    MODULE_ID 	moduleId		/* module to get segment from */
    )
    {
    SEGMENT_ID	segmentId;

    /*
     * Validate module id
     */

    if (OBJ_VERIFY (moduleId, moduleClassId) != OK)
	{
	return (NULL);
	}
	    
    semTake (moduleSegSem, WAIT_FOREVER);

    segmentId = (SEGMENT_ID) dllGet (&moduleId->segmentList);

    semGive (moduleSegSem);

    return (segmentId);
    }

/******************************************************************************
*
* moduleSegFirst - find the first segment in a module
* 
* This routine returns information about the first segment of a module
* descriptor.
* 
* RETURNS: A pointer to the segment ID, or NULL if the segment list is empty.
*
* SEE ALSO: moduleSegGet()
*/

SEGMENT_ID moduleSegFirst
    (
    MODULE_ID 	moduleId		/* module to get segment from */
    )
    {
    SEGMENT_ID	newSegId;

    /*
     * Validate module id
     */

    if (OBJ_VERIFY (moduleId, moduleClassId) != OK)
	{
	return (NULL);
	}

    semTake (moduleSegSem, WAIT_FOREVER);
    newSegId = (SEGMENT_ID) DLL_FIRST (&moduleId->segmentList);
    semGive (moduleSegSem);

    return (newSegId);
    }

/******************************************************************************
*
* moduleSegNext - find the next segment in a module
* 
* This routine returns the segment in the list immediately following
* <segmentId>.
* 
* RETURNS: A pointer to the segment ID, or NULL if there is no next segment.
*/

SEGMENT_ID moduleSegNext
    (
    SEGMENT_ID segmentId	/* segment whose successor is to be found */
    )
    {
    SEGMENT_ID	newSegId;

    semTake (moduleSegSem, WAIT_FOREVER);
    newSegId = (SEGMENT_ID) DLL_NEXT (segmentId);
    semGive (moduleSegSem);

    return (newSegId);
    }

/******************************************************************************
*
* moduleSegEach - call a routine to examine each segment in a module
* 
* This routine calls a user-supplied routine to examine each segment
* associated with a given module.  The routine should be declared as
* follows:
* 
* .CS
*     BOOL routine
*         (
*         SEGMENT_ID 	 segmentId,   /@ The segment to examine @/
*         MODULE_ID	 moduleId,    /@ The associated module @/
*         int		 userArg      /@ An arbitrary user-supplied argument @/
*         )
* .CE
* 
* The user routine should return TRUE if moduleSegEach() is to continue
* calling it for the other segments, or FALSE if moduleSegEach() should
* exit.
* 
* RETURNS: NULL if all segments were examined, or the segment ID that
* caused the support routine to return FALSE.
* 
* NOMANUAL
*/

SEGMENT_ID moduleSegEach
    (
    MODULE_ID	moduleId,	/* The module to examine */
    FUNCPTR 	routine,	/* The routine to call */
    int		userArg		/* arbitrary user-supplied argument */
    )
    {
    SEGMENT_ID	segmentId;

    semTake (moduleSegSem, WAIT_FOREVER);
    for (segmentId = moduleSegFirst (moduleId);
	 segmentId != NULL;
	 segmentId = moduleSegNext (segmentId))
	{
	if ((*routine) (segmentId, moduleId, userArg) == FALSE)
	    {
	    semGive (moduleSegSem);
	    return (segmentId);
	    }
	}
    semGive (moduleSegSem);
    return (NULL);
    }
    
/******************************************************************************
*
* moduleCreateHookAdd - add a routine to be called when a module is added
* 
* This routine adds a specified routine to a list of routines to be
* called when a module is created.  The specified routine should be
* declared as follows:
* .CS
*     void moduleCreateHook
*         (
*         MODULE_ID  moduleId  /@ the module ID @/
*         )
* .CE
* 
* This routine is called after all fields of the module ID have been filled in.
* 
* NOTE:
* Modules do not have information about their object segments when they are
* created.  This information is not available until after the entire load
* process has finished.
* 
* RETURNS: OK or ERROR.
* 
* SEE ALSO: moduleCreateHookDelete()
*/

STATUS moduleCreateHookAdd
    (
    FUNCPTR     moduleCreateHookRtn  /* routine called when module is added */
    )
    {
    MODCREATE_HOOK * pHook;      	 /* pointer to hook node */

    if (!moduleCreateHookInitialized)
	{
	dllInit (&moduleCreateHookList);
	moduleCreateHookInitialized = TRUE;
	}

    /* allocate and initialize hook node */

    if ((pHook = (MODCREATE_HOOK *) malloc (sizeof (MODCREATE_HOOK)))
	== NULL)
	{
	return (ERROR);
	}

    pHook->func = moduleCreateHookRtn;

    /* add it to end of hook list */

    semTake (moduleCreateHookSem, WAIT_FOREVER);
    dllAdd (&moduleCreateHookList, &pHook->node);
    semGive (moduleCreateHookSem);

    return (OK);
    }

/******************************************************************************
*
* moduleCreateHookDelete - delete a previously added module create hook routine
*
* This routine removes a specified routine from the list of
* routines to be called at each moduleCreate() call.
*
* RETURNS: OK, or ERROR if the routine is not in the table of module create
* hook routines.
*
* SEE ALSO: moduleCreateHookAdd()
*/

STATUS moduleCreateHookDelete
    (
    FUNCPTR     moduleCreateHookRtn  /* routine called when module is added */
    )
    {
    MODCREATE_HOOK * pNode;	         /* hook node */

    /*
     * Step through the list of create hooks until we find a match
     */

    for (pNode = (MODCREATE_HOOK *) DLL_FIRST (&moduleCreateHookList);
	 pNode != NULL;
	 pNode = (MODCREATE_HOOK *) DLL_NEXT ((DL_NODE *) pNode))
	{
	if (pNode->func == moduleCreateHookRtn)
	    {
	    
	    /*
	     * We've found the node, delete it and free the space
	     */

	    dllRemove (&moduleCreateHookList, (DL_NODE *) pNode);
	    free (pNode);
	    return (OK);
	    }
	}

    /*
     * We didn't find the node, return ERROR
     */

    errnoSet (S_moduleLib_HOOK_NOT_FOUND);
    return (ERROR);
    }

/******************************************************************************
*
* moduleFindByName - find a module by name
* 
* This routine searches for a module with a name matching <moduleName>.
* 
* RETURNS: MODULE_ID, or NULL if no match is found.
*/

MODULE_ID moduleFindByName
    (
    char *	 moduleName	       /* name of module to find */
    )
    {
    MODULE_ID	 moduleId;
    char         splitName [NAME_MAX]; /* name part of moduleName */
    char         splitPath [PATH_MAX]; /* path part of moduleName */

    /* get just the name component of moduleName */

    pathSplit (moduleName, splitPath, splitName);

    /*
     * Go through the module list, check each module against
     * the search parameter(s).
     */

    semTake (moduleListSem, WAIT_FOREVER);
    for (moduleId = NODE_TO_ID (DLL_LAST (&moduleList));
	 moduleId != NULL;
	 moduleId = NODE_TO_ID (DLL_PREVIOUS (&moduleId->moduleNode)))
	{
	
	/*
	 * Found the right one, give the list semaphore, return the module id
	 */

	if (strcmp (moduleId->name, splitName) == 0)
	    {
	    semGive (moduleListSem);
	    return (moduleId);
	    }
	}

    /*
     * Give back the module list semaphore, and return NULL (nothing found)
     */

    semGive (moduleListSem);
    return (NULL);
    }

/******************************************************************************
*
* moduleFindByNameAndPath - find a module by file name and path
* 
* This routine searches for a module with a name matching <moduleName>
* and path matching <pathName>.
* 
* RETURNS: MODULE_ID, or NULL if no match is found.
*/

MODULE_ID moduleFindByNameAndPath
    (
    char * 	moduleName,		/* file name to find */
    char * 	pathName		/* path name to find */
    )
    {
    MODULE_ID 	moduleId;

    /*
     * Go through the module list, check each module against
     * the search parameter(s).  Take the module list semaphore first.
     */

    semTake (moduleListSem, WAIT_FOREVER);
    
    for (moduleId = NODE_TO_ID (DLL_LAST (&moduleList));
	 moduleId != NULL;
	 moduleId = NODE_TO_ID (DLL_PREVIOUS (&moduleId->moduleNode)))
	{
	
	/*
	 * Found the right one, give the semaphore, return the module id
	 */

	if ((strcmp (moduleId->name, moduleName) == 0) &&
	    (strcmp (moduleId->path, pathName) == 0))
	    {
	    semGive (moduleListSem);
	    return (moduleId);
	    }
	}

    semGive (moduleListSem);
    return (NULL);
    }

/******************************************************************************
*
* moduleFindByGroup - find a module by group number
* 
* This routine searches for a module with a group number matching
* <groupNumber>.
* 
* RETURNS: MODULE_ID, or NULL if no match is found.
*/

MODULE_ID moduleFindByGroup
    (
    int 	groupNumber		/* group number to find */
    )
    {
    MODULE_ID 	moduleId;

    /*
     * Go through the module list, check each module against
     * the search parameter(s).  Take the list semaphore first
     */

    semTake (moduleListSem, WAIT_FOREVER);

    for (moduleId = NODE_TO_ID (DLL_LAST (&moduleList));
	 moduleId != NULL;
	 moduleId = NODE_TO_ID (DLL_PREVIOUS (&moduleId->moduleNode)))
	{
	
	/*
	 * Found the right one, give the semaphore, return the module id
	 */

	if (groupNumber == moduleId->group)
	    {
	    semGive (moduleListSem);
	    return (moduleId);
	    }
	}

    semGive (moduleListSem);
    return (NULL);
    }

/******************************************************************************
*
* moduleEach - call a routine to examine each loaded module
* 
* This routine calls a user-supplied routine to examine each module.
* The routine should be declared as follows:
* 
* .CS
*       BOOL routine
*           (
*           MODULE_ID	 moduleId,    /@ The associated module @/
* 	    int		 userArg      /@ An arbitrary user-supplied argument @/
*           )
* .CE
* 
* The user routine should return TRUE if moduleEach() is to continue
* calling it for the remaining modules, or FALSE if moduleEach() should
* exit.
* 
* RETURNS: NULL if all modules were examined, or the module ID that
* caused the support routine to return FALSE.
* 
* NOMANUAL
*/

MODULE_ID moduleEach
    (
    FUNCPTR 	routine,	/* The routine to call */
    int		userArg		/* arbitrary user-supplied argument */
    )
    {
    MODULE_ID	 moduleId;

    semTake (moduleListSem, WAIT_FOREVER);

    for (moduleId = NODE_TO_ID (DLL_LAST (&moduleList));
	 moduleId != NULL;
	 moduleId = NODE_TO_ID (DLL_PREVIOUS (&moduleId->moduleNode)))
	{

	/*
	 * If the user routine returns false, then return the current
	 * module ID
	 */

	if ((* routine) (moduleId, userArg) == FALSE)
	    {
	    semGive (moduleListSem);
	    return (moduleId);
	    }
	}

    /* Give back the module list semaphore */

    semGive (moduleListSem);
    return (NULL);
    }

/******************************************************************************
*
* moduleIdListGet - get a list of loaded modules
* 
* This routine provides the calling task with a list of all loaded
* object modules.  An unsorted list of module IDs for no more than
* <maxModules> modules is put into <idList>.
* 
* RETURNS: The number of modules put into the ID list, or ERROR.
*/

int moduleIdListGet
    (
    MODULE_ID *	idList,		/* array of module IDs to be filled in */
    int		maxModules	/* max modules <idList> can accommodate */
    )
    {
    MODULE_ID 	moduleId;	/* current module */
    int		count = 0;	/* count of modules put into array */

    semTake (moduleListSem, WAIT_FOREVER);
    
    for (moduleId = NODE_TO_ID (DLL_FIRST (&moduleList));
	 moduleId != NULL && count < maxModules;
	 moduleId = NODE_TO_ID (DLL_NEXT (&moduleId->moduleNode)))
	{
	idList [count++] = moduleId;
	}

    semGive (moduleListSem);
    
    return (count);
    }

/******************************************************************************
*
* moduleSegInfoGet - get information about the segments of a module
*
* This routine fills in a MODULE_SEG_INFO struct with information about the
* specified module's segments.
* 
* RETURNS: OK or ERROR
*/

LOCAL STATUS moduleSegInfoGet
    (
    MODULE_ID	  	moduleId,	/* module to query */
    MODULE_SEG_INFO *	pModSegInfo	/* ptr to module segment info struct */
    )
    {
    SEGMENT_ID		segId;		/* loop variable */

    bzero ((char *) pModSegInfo, sizeof (*pModSegInfo));
    for (segId = moduleSegFirst (moduleId);
	 segId != NULL;
	 segId = moduleSegNext (segId))
	{
	switch (segId->type)
	    {
	    case SEGMENT_TEXT:
		pModSegInfo->textAddr = segId->address;
		pModSegInfo->textSize = segId->size;
		break;
	    case SEGMENT_DATA:
		pModSegInfo->dataAddr = segId->address;
		pModSegInfo->dataSize = segId->size;
		break;
	    case SEGMENT_BSS:
		pModSegInfo->bssAddr = segId->address;
		pModSegInfo->bssSize = segId->size;
		break;
	    }
	} /* for loop end */
    
    return (OK);
    }

/******************************************************************************
*
* moduleInfoGet - get information about an object module
* 
* This routine fills in a MODULE_INFO structure with information about the
* specified module.
* 
* RETURNS: OK or ERROR.
*/

STATUS moduleInfoGet
    (
    MODULE_ID	  moduleId,	/* module to return information about */
    MODULE_INFO * pModuleInfo	/* pointer to module info struct */
    )
    {
    /* Validate module id */

    if (OBJ_VERIFY (moduleId, moduleClassId) != OK)
	{
	return (ERROR);
	}

    strcpy (pModuleInfo->name, moduleId->name);
    pModuleInfo->format = moduleId->format;
    pModuleInfo->group = moduleId->group;

    return (moduleSegInfoGet (moduleId, &pModuleInfo->segInfo));
    }

/******************************************************************************
*
* moduleCheckSegment - compare the current checksum to the recorded checksum
*/

LOCAL BOOL moduleCheckSegment
    (
    SEGMENT_ID	segmentId	/* the segment */
    )
    {
    u_short 		cksum;
    extern u_short 	checksum();

    /*
     * Compute the new checksum, and compare it to the
     * old one.
     */

    cksum = checksum (segmentId->address, segmentId->size);

    if (cksum != segmentId->checksum)
	{
	errnoSet (S_moduleLib_BAD_CHECKSUM);
	return (FALSE);
	}
    else
	return (TRUE);
    }

/******************************************************************************
*
* moduleCheckOne - verify checksums on a specific module
*/

LOCAL BOOL moduleCheckOne
    (
    MODULE_ID 	moduleId,	/* module to check */
    int		options		/* options */
    )
    {
    SEGMENT_ID	segmentId;
    STATUS	returnValue = TRUE;

    /* If no options are set, default to checking text segments */

    if (options == 0)
        options = MODCHECK_TEXT;

    for (segmentId = moduleSegFirst (moduleId);
	 segmentId != NULL;
	 segmentId = moduleSegNext (segmentId))
	{
	if (segmentId->type & options)
	    {
	    if (moduleCheckSegment (segmentId) == FALSE)
		{
		if (!(options & MODCHECK_NOPRINT))
		    {
		    printf ("Checksum error in segment type %d, ",
			    segmentId->type);
		    printf ("module %#x (%s)\n", (int) moduleId,
			    moduleId->name);
		    }
		returnValue = FALSE;
		}
	    }
	}

    return (returnValue);
    }
    
/******************************************************************************
*
* moduleCheck - verify checksums on all modules
*
* This routine verifies the checksums on the segments of all loaded
* modules.  If any of the checksums are incorrect, a message is printed to
* the console, and the routine returns ERROR.
*
* By default, only the text segment checksum is validated.
*
* Bits in the <options> parameter may be set to control specific checks:
* .iP MODCHECK_TEXT
* Validate the checksum for the TEXT segment (default).
* .iP MODCHECK_DATA
* Validate the checksum for the DATA segment.
* .iP MODCHECK_BSS
* Validate the checksum for the BSS segment.
* .iP MODCHECK_NOPRINT
* Do not print a message (moduleCheck() still returns ERROR on failure.)
* .LP
* See the definitions in moduleLib.h
*
* RETURNS: OK, or ERROR if the checksum is invalid.
*/

STATUS moduleCheck
    (
    int options			/* validation options */
    )
    {
    if (moduleEach (moduleCheckOne, options) == NULL)
        return (OK);
    else
        return (ERROR);
    }

/******************************************************************************
*
* moduleNameGet - get the name associated with a module ID
*
* This routine returns a pointer to the name associated with a module ID.
*
* RETURNS: A pointer to the module name, or NULL if the module ID is invalid.
*/

char * moduleNameGet
    (
    MODULE_ID moduleId
    )
    {
    if (OBJ_VERIFY (moduleId, moduleClassId) != OK)  /* validate module id */
	{
	return (NULL);
	}

    return (moduleId->name);
    }

/******************************************************************************
*
* moduleFlagsGet - get the flags associated with a module ID
*
* This routine returns the flags associated with a module ID.
*
* RETURNS: The flags associated with the module ID, or NULL if the module ID
* is invalid.
*/

int moduleFlagsGet
    (
    MODULE_ID moduleId
    )
    {
    if (OBJ_VERIFY (moduleId, moduleClassId) != OK)  /* validate module id */
	{
	return (NULL);
	}

    return (moduleId->flags);
    }

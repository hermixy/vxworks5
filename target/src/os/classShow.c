/* classShow.c - class object show routines */

/* Copyright 1992-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01c,12oct01,jn   use symFindSymbol for symbol lookups (SPR #7453)
01b,10dec93,smb  changed helpRtn to instRtn
01a,04jul92,jcf  created.
*/

/*
DESCRIPTION
This library provides routines to show class related information.

INCLUDE FILE: classLib.h

SEE ALSO: classLib
*/

#include "vxWorks.h"
#include "string.h"
#include "stdio.h"
#include "a_out.h"
#include "sysSymTbl.h"
#include "symLib.h"
#include "errno.h"
#include "private/funcBindP.h"
#include "private/classLibP.h"

/* forward declarations */

LOCAL void classShowSymbol (int value);


/******************************************************************************
*
* classShowInit - initialize class show routine
*
* This routine links the class show routine into the VxWorks system.
* These routines are included automatically by defining \%INCLUDE_SHOW_RTNS
* in configAll.h.
*
* RETURNS: N/A
*/

void classShowInit (void)
    {
    classShowConnect (classClassId, (FUNCPTR)classShow);
    }

/*******************************************************************************
*
* classShow - show the information for the specified class
*
* This routine shows all information associated with the specified class.
*
* RETURNS: OK, or ERROR if invalid class id.
*
* SEE ALSO: symLib, symEach()
*/

STATUS classShow
    (
    CLASS_ID	classId, 	/* object class to summarize */
    int         level           /* 0 = summary, 1 = details */
    )
    {
    if (OBJ_VERIFY (classId, classClassId) != OK)
	return (ERROR);				/* invalid class object */

    /* summarize information */

    printf ("\n");

    printf ("%-20s: 0x%-10x", "Memory Partition Id", classId->objPartId);
    classShowSymbol ((int)classId->objPartId);

    printf ("%-20s: %-10d\n", "Object Size", classId->objSize);
    printf ("%-20s: %-10d\n", "Objects Allocated", classId->objAllocCnt);
    printf ("%-20s: %-10d\n", "Objects Deallocated", classId->objFreeCnt);
    printf ("%-20s: %-10d\n", "Objects Initialized", classId->objInitCnt);
    printf ("%-20s: %-10d\n", "Objects Terminated", classId->objTerminateCnt);

    printf ("%-20s: 0x%-10x", "Create Routine", classId->createRtn);
    classShowSymbol ((int)classId->createRtn);

    printf ("%-20s: 0x%-10x", "Init Routine", classId->initRtn);
    classShowSymbol ((int)classId->initRtn);

    printf ("%-20s: 0x%-10x", "Destroy Routine", classId->destroyRtn);
    classShowSymbol ((int)classId->destroyRtn);

    printf ("%-20s: 0x%-10x", "Show Routine", classId->showRtn);
    classShowSymbol ((int)classId->showRtn);

    printf ("%-20s: 0x%-10x", "Inst Routine", classId->instRtn);
    classShowSymbol ((int)classId->instRtn);

    return (OK);
    }

/*******************************************************************************
*
* classShowSymbol - print value symbolically if possible
*/

LOCAL void classShowSymbol
    (
    int	value		/* value to find in symbol table */
    )
    {
    char *    name = NULL;              /* actual symbol name (symTbl copy) */
    void *    symValue = 0;		/* actual symbol value */
    SYMBOL_ID symId;                    /* symbol identifier */ 

    if (value == 0)
	{
	printf ("No routine attached.\n");
	return;
	}

    /* 
     * Only check one symLib function pointer (for performance's sake). 
     * All symLib functions are provided by the same library, by convention.    
     */

    if ((_func_symFindSymbol != (FUNCPTR) NULL) && 
	(sysSymTbl != NULL))
        {
	if ((((* _func_symFindSymbol) (sysSymTbl, NULL, (char *)value,
				       N_EXT | N_TEXT, 
				       N_EXT | N_TEXT, &symId)) == OK) &&
	    ((* _func_symNameGet) (symId, &name) == OK) &&
	    ((* _func_symValueGet) (symId, &symValue) == OK) &&
	    (symValue == (void *)value)) 
	    {
	    /* address and type match exactly */

	    printf (" (%s)", name);
	    }
	}

    printf ("\n");
    }

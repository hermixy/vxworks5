/* symShow.c - symbol table show routines */

/* Copyright 1984-1993 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01i,17mar99,jdi  doc: updated w/ info about proj facility (SPR 25727).
01h,30sep93,dvs  changed default value of symLkupPgSz to 22.
01g,24aug93,dvs  modified symSysTblPrint() to display symbols in sets of 
		 symLkupPgSz, user can now quit from lkup() (SPR #921)
01f,13feb93,kdl  changed cplusLib.h to private/cplusLibP.h (SPR #1917).
01e,03feb93,jdi  changed INCLUDE_SHOW_RTNS to ...ROUTINES.
01d,01aug92,srh  added C++ demangling idiom to symSysTblPrint and symPrint
01c,20jul92,jmm  added group param to symSysTblPrint, lkup() now displays module
01b,12jul92,jcf  changed symShow (NULL) to summarize symbol table struct.
01a,04jul92,jcf  created.
*/

/*
DESCRIPTION
This library provides a routine for showing symbol table information.
The routine symShowInit() links the symbol table show facility into
the VxWorks system.  It is called automatically when this
facility is configured into VxWorks using either of the
following methods:
.iP
If you use the configuration header files, define
INCLUDE_SHOW_ROUTINES in config.h.
.iP
If you use the Tornado project facility, select INCLUDE_SYM_TABLE_SHOW.

INCLUDE FILE: symLib.h

SEE ALSO: symLib
*/

#include "vxWorks.h"
#include "string.h"
#include "stdio.h"
#include "a_out.h"
#include "sysSymTbl.h"
#include "symLib.h"
#include "errno.h"
#include "moduleLib.h"
#include "fioLib.h"
#include "types.h"
#include "private/cplusLibP.h"

/* locals */

LOCAL char *typeName [] =		/* system symbol table types */
    {
    "????",
    "abs",
    "text",
    "data",
    "bss",
    };
LOCAL int  symCount = 0;		/* number of symbols printed */

/* globals */

uint32_t symLkupPgSz = 22;		/* max symbols displayed at a time */

/* forward declarations */

LOCAL BOOL symPrint (char *name, int val, INT8 type, char *substr);
LOCAL BOOL symSysTblPrint (char *name, int val, INT8 type, char *substr,
			   UINT16 group);
LOCAL char *strMatch (FAST char *str1, FAST char *str2);


/******************************************************************************
*
* symShowInit - initialize symbol table show routine
*
* This routine links the symbol table show facility into the VxWorks system.
* It is called automatically when the symbol table show facility is
* configured into VxWorks using either of the following methods:
* .iP
* If you use the configuration header files, define
* INCLUDE_SHOW_ROUTINES in config.h.
* .iP
* If you use the Tornado project facility, select INCLUDE_SYM_TABLE_SHOW.
*
* RETURNS: N/A
*/

void symShowInit (void)
    {
    classShowConnect (symTblClassId, (FUNCPTR)symShow);
    }

/*******************************************************************************
*
* symShow - show the symbols of specified symbol table with matching substring
*
* This routine lists all symbols in the specified symbol table whose names
* contain the string <substr>.  If <substr> is is an empty * string (""), all
* symbols in the table will be listed.  If <substr> is NULL then the symbol
* table structure will be summarized.
*
* RETURNS: OK, or ERROR if invalid symbol table id.
*
* SEE ALSO: symLib, symEach()
*/

STATUS symShow
    (
    SYMTAB *	pSymTbl,	/* pointer to symbol table to summarize */
    char *	substr		/* substring to match */
    )
    {
    if (OBJ_VERIFY (pSymTbl, symTblClassId) != OK)
	return (ERROR);				/* invalid symbol table ID */

    if (substr == NULL)
	{
	printf ("%-20s: %-10d\n", "Number of Symbols", pSymTbl->nsymbols);
	printf ("%-20s: 0x%-10x\n", "Symbol Mutex Id", &pSymTbl->symMutex);
	printf ("%-20s: 0x%-10x\n", "Symbol Hash Id", pSymTbl->nameHashId);
	printf ("%-20s: 0x%-10x\n", "Symbol memPartId", pSymTbl->symPartId);
	printf ("%-20s: %-10s\n", "Name Clash Policy", 
		(pSymTbl->sameNameOk) ? "Allowed" : "Disallowed");
	}
    else
	{
	if (pSymTbl == sysSymTbl)
	    {
	    symEach (pSymTbl, (FUNCPTR) symSysTblPrint, (int) substr);
	    symCount = 0;
	    }
	else
	    symEach (pSymTbl, (FUNCPTR) symPrint, (int) substr);
	}

    return (OK);
    }

/*******************************************************************************
*
* symSysTblPrint - support routine for symShow()
*
* This routine is called by symEach() to deal with each symbol in the 
* system symbol table.  If the symbol's name contains <substr>, this routine
* prints the symbol.  Otherwise, it doesn't.  If <substr> is NULL, every
* symbol is printed.  The type is printed along with the symbol value.
*/

LOCAL BOOL symSysTblPrint
    (
    char *	name,
    int		val,
    INT8	type,
    char *	substr,
    UINT16      group
    )
    {
    char 	moduleName [NAME_MAX];
    MODULE_ID	moduleId;
    char	demangled [MAX_SYS_SYM_LEN + 1];
    char *	nameToPrint;
    char	quitChar;		/* q to quit displaying symbols */

    /*
     * If group is the system group default, don't print anything.  If
     * it's not the default, try to get a corresponding module name.
     * If you can't find a module, just print the group number.
     */

    if (group == symGroupDefault)
        moduleName [0] = EOS;
    else if ((moduleId = moduleFindByGroup (group)) != NULL)
        sprintf (moduleName, "(%s)", moduleId->name);
    else
        sprintf (moduleName, "(%d)", group);
    
    if (substr == NULL || strMatch (name, substr) != NULL)
	{
	nameToPrint = cplusDemangle (name, demangled, MAX_SYS_SYM_LEN + 1);
	printf ("%-25s 0x%08x %-8s %s", nameToPrint, val,
		typeName [(type >> 1) & 7], moduleName);

	if ((type & N_EXT) == 0)
	    printf (" (local)");

	printf ("\n");

	if (symLkupPgSz != 0)			/* do paging */
	    {
	    symCount++;
	    if ((symCount % symLkupPgSz) == 0)	/* prompt user to continue */
		{
		printf ("\nType <CR> to continue, Q<CR> to stop: ");
		fioRdString (STD_IN, &quitChar, 1);
		if ((quitChar == 'q') || (quitChar == 'Q'))
		    return (FALSE);
		}
	    }
	}


    return (TRUE);
    }

/*******************************************************************************
*
* symPrint - support routine for symShow()
*
* This routine is called by symEach() to deal with each symbol in the table.
* If the symbol's name contains <substr>, this routine prints the symbol.
* Otherwise, it doesn't.  If <substr> is NULL, every symbol is printed.
*/

LOCAL BOOL symPrint
    (
    char *	name,
    int		val,
    INT8	type,
    char *	substr
    )
    {
    char	demangled [MAX_SYS_SYM_LEN + 1];
    char *	nameToPrint;

    if (substr == NULL || strMatch (name, substr) != NULL)
        {
	nameToPrint = cplusDemangle (name, demangled, MAX_SYS_SYM_LEN + 1);
	printf ("%-25s 0x%08x\n", nameToPrint, val);
        }

    return (TRUE);
    }

/*******************************************************************************
*
* strMatch - find an occurrence of a string in another string
*
* This is a simple pattern matcher used by symPrint().  It looks for an
* occurence of <str2> in <str1> and returns a pointer to that occurrence in
* <str1>.  If it doesn't find one, it returns 0.
*/

LOCAL char *strMatch
    (
    FAST char *str1,		/* where to look for match */
    FAST char *str2		/* string to find a match for */
    )
    {
    FAST int str2Length = strlen (str2);
    FAST int ntries	= strlen (str1) - str2Length;

    for (; ntries >= 0; str1++, --ntries)
	{
	if (strncmp (str1, str2, str2Length) == 0)
	    return (str1);	/* we've found a match */
	}

    return (NULL);
    }

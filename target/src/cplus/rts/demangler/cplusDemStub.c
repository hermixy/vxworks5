/* cplusDemStub.c - C++ symbol demangler stub */

/* Copyright 1998 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01g,13mar02,sn   SPR 74275 - allow demangler to be decoupled from target shell
01f,21jan02,sn   changed to C file to avoid C++ being pulled in by timexShow
01e,12dec01,jab  added coldfire to PREPENDS_UNDERSCORE list
01d,04dec01,sn   moved PREPENDS_UNDERSCORE here
01c,30oct01,sn   got building with Diab
01b,12jun98,sn   moved function symbolStartOf from cplusDem.cpp to here.
01a,10apr98,sn   written.
*/

/*
DESCRIPTION
This module seperates the interface of cplusDemangle from its implementation
in cplusDem.cpp. 

Many "show" routines such as timexShowCalls attempt to present demangled 
symbol names to the user  by calling cplusDemangle. If
C++ is not configured into vxWorks the correct behaviour
is simply to return the mangled name. We have seperated out a "stub"
of cplusDemangle from its body (_cplusDemangle) to avoid dragging in the 
entire C++ runtime with any vxWorks image that contains show routines. 

NOMANUAL
*/

/* includes */

#include "vxWorks.h"
#include "cplusLib.h"
#include "stdio.h"
#include "stdlib.h"

/* defines */
/* PREPENDS_UNDERSCORE should be 1 if the compiler prepends an underscore 
 * to symbol names; otherwise 0.
 */
#if (CPU_FAMILY==SPARC || \
     CPU_FAMILY==I960 || \
     CPU_FAMILY==MC680X0 || \
     CPU_FAMILY==SIMSPARCSUNOS || \
     CPU_FAMILY==SIMNT || \
     CPU_FAMILY==COLDFIRE)
#define PREPENDS_UNDERSCORE 1
#else
#define PREPENDS_UNDERSCORE 0
#endif

#define MAX_OVERLOAD_SYMS 50

/* typedefs */

/* globals */

CPLUS_DEMANGLER_MODES cplusDemanglerMode = COMPLETE;
DEMANGLER_STYLE cplusDemanglerStyle;

char * (*cplusDemangleFunc) (char *, char *, int) = 0;

/* locals */

LOCAL char * overloadMatches [MAX_OVERLOAD_SYMS];
LOCAL int overloadMatchCount;
LOCAL BOOL findMatches
    (
    char *name,
    int,
    SYM_TYPE,
    int string
    );

LOCAL const char * startsWith
    (
    const char *	string,
    const char *	isubstr
    );

/* forward declarations */

/******************************************************************************
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
*
* EXAMPLES
* The following example shows how one function name would be printed
* under each demangling mode:
*
* .TS
* center,tab(|);
* lf3 lf3
* l l.
* Mode     | Printed symbol
* _
* off      | _member_\^_5classFPFl_PvPFPv_v
* terse    | _member
* complete | foo::_member(void* (*)(long),void (*)(void*))
* .TE
*
* RETURNS: N/A
*/
void cplusDemanglerSet
    (
    int	mode
    )
    {
    cplusDemanglerMode = (CPLUS_DEMANGLER_MODES) mode;
    }

/*******************************************************************************
*
* cplusDemanglerStyleSet - change C++ demangling style (C++)
*
* This command sets the C++ demangling mode to <style>.
* The default demangling style depends on the toolchain
* used to build the kernel. For example if the Diab
* toolchain is used to build the kernel then the default
* demangler style is DMGL_STYLE_DIAB.
*
* RETURNS: N/A
*/

void cplusDemanglerStyleSet
    (          
    DEMANGLER_STYLE style
    )          
    {
    cplusDemanglerStyle = style;
    }

/*******************************************************************************
*
* symbolStartOf - skip leading underscore
*
* RETURNS: pointer to the start of the symbol name after any compiler 
*          prepended leading underscore.
*        
*
* NOMANUAL
*/

LOCAL char *symbolStartOf (
			  char *str
			  )
{
    if (PREPENDS_UNDERSCORE && (str [0] == '_'))
        return str + 1;
    else
        return str;
}


/******************************************************************************
*
* cplusDemangle - demangle symbol
*
* This function takes a symbol (source), removes any compiler prepended 
* underscore, and then attempts to demangle it. If demangling succeeds
* the (first n chars of) the demangled string are placed in dest [] and
* a pointer to dest is returned. Otherwise a pointer to the start of
* the symbol proper (after any compiler prepended underscore) is returned.

* RETURNS:
* A pointer to a human readable version of source.
*
* NOMANUAL
*/

char * cplusDemangle
    (
    char	* source,		/* mangled name */
    char	* dest,			/* buffer for demangled copy */
    int		  n		/* maximum length of copy */
    )
    {
    source = symbolStartOf (source);
    if (cplusDemangleFunc == 0  )
	{
	return source;
	}
    else
	{
        return cplusDemangleFunc (source, dest, n);
	}
    }

/*******************************************************************************
*
* askUser - ask user to choose among overloaded name alternatives
*
* This routine is used by cplusMatchMangled when a name is overloaded.
*
* RETURNS:
* index into overloadMatches, or negative if no symbol is selected
*
* NOMANUAL
*/

LOCAL int askUser
    (
    char *name
    )
    {
    CPLUS_DEMANGLER_MODES	  savedMode;
    char			  demangled [MAX_SYS_SYM_LEN + 1];
    char			  userInput [10];
    const char 			* nameToPrint;
    int				  choice;
    int				  i;

    savedMode		= cplusDemanglerMode;
    cplusDemanglerMode	= COMPLETE;
    do
	{
	printf ("\"%s\" is overloaded - Please select:\n", name);
	for (i = 0; i < overloadMatchCount; i++)
	    {
	    nameToPrint = cplusDemangle (overloadMatches [i], demangled,
					 MAX_SYS_SYM_LEN + 1);
	    printf ("  %3d: %s\n", i+1, nameToPrint);
	    }
	printf ("Enter <number> to select, anything else to stop: ");
	fgets (userInput, 10, stdin);
	choice = atoi (userInput) - 1;
	}
    while (choice >= overloadMatchCount);
    cplusDemanglerMode = savedMode;
    return choice;
    }
/*******************************************************************************
*
* cplusMatchMangled - match string against mangled symbol table entries
*
* This function seeks a partial match between the given <string> and
* symbols in the given <symTab>. If <string> matches one symbol, that
* symbol's value and type are copied to <pValue> and <pType>. If <string>
* matches multiple symbols, the shell user is prompted to disambiguate
* her choice among the various alternatives.
*
* RETURNS: TRUE if a unique match is resolved, otherwise FALSE.
*
* NOMANUAL
*/

BOOL cplusMatchMangled
    (
    SYMTAB_ID		symTab,			/* symbol table to search */
    char		*string,		/* goal string */
    SYM_TYPE		*pType,			/* type of matched symbol */
    int			*pValue			/* value of matched symbol */
    )

    {
    int userChoice;
    
    overloadMatchCount = 0;
    symEach (symTab, (FUNCPTR) findMatches, (int) string);
    switch (overloadMatchCount)
	{
    case 0:
	return FALSE;

    case 1:
	return (symFindByName (symTab, overloadMatches[0],
			       (char **) pValue, pType)
		== OK);
    default:
	userChoice = askUser (string);
	if (userChoice >= 0)
	    {
	    return (symFindByName (symTab, overloadMatches [userChoice],
				   (char **)pValue, pType)
		    == OK);
	    }
	else
	    {
	    return FALSE;
	    }
	}
    }
/*******************************************************************************
*
* findMatches - find (possibly overloaded) symbols that match goal string
*
* This function is used by cplusMatchMangled. A match occurs when
* <goalString> is an initial substring of <name> and the occurrence
* of <goalString> is followed in <name> by "__", which is assumed
* to be followed by the function's encoded signature.
*
* RETURNS: TRUE
*
* NOMANUAL
*/
LOCAL BOOL findMatches
    (
    char *name,
    int dummy1,
    SYM_TYPE dummy2,
    int string
    )
    {
    const char *pMatch;
    const char * goalString = (const char *) string;

    if ((pMatch = startsWith (name + 1, goalString)) != 0
	&&
	(startsWith (pMatch + 1, "__") != 0))
	    {
	    if (overloadMatchCount < MAX_OVERLOAD_SYMS)
		{
		overloadMatches [overloadMatchCount++] = name;
		}
            }

    /* Check for symbol does not start with underscore */

    else if ((pMatch = startsWith (name, goalString)) != 0
	     && 
	     (startsWith (pMatch + 1, "__") != 0))
	     {
	     if (overloadMatchCount < MAX_OVERLOAD_SYMS)
		{
		overloadMatches [overloadMatchCount++] = name;
		}
	     }

    return TRUE;
    }
/*******************************************************************************
*
* startsWith - determine if <string> starts with <isubstr>
*
* RETURNS:
* Pointer to final non-EOS character of <isubstr> where it occurs in <string>,
* if <string> in fact starts with <isubstr>, otherwise 0.
*
* NOMANUAL
*/

LOCAL const char * startsWith
    (
    const char *	string,
    const char *	isubstr
    )
    {
    if (*isubstr == EOS || *string == EOS)
	{
	return 0;
	}
    for ( ; (*string == *isubstr); string++, isubstr++)
	{
	if (*isubstr == EOS)
	    {
	    return string - 1;
	    }
	}
    if (*isubstr == EOS)
	{
	return string - 1;
	}
    return 0;
    }



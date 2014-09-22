/* symLib.c - symbol table subroutine library */

/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01j,12oct01,jn   fix SPR 7453 - broken API leads to stack corruption - add
                 new API from AE (symXXXFind - from symLib.c@@/main/tor3_x/3) 
		 and new internal-only API (symXXXGet).  
01i,14mar99,jdi  doc: removed refs to config.h and/or configAll.h (SPR 25663).
04h,14feb97,dvs  added check if sysTblId is NULL in symFindByValueAndType() 
		 (SPR #6876)
04g,30oct96,elp  Added syncSymAddRtn, syncSymRemoveRtn function pointers
		 and symSAdd().
04f,19jun96,ism  added error condition if NULL is passed as symbol table to
                    symFindByNameAndType().
04e,22oct95,jdi  doc: added buffer size to allocate for the symFindByValue
		    routines (SPR 4386).
04d,13may95,p_m  added _func_symFindByValue initialization.
04c,11jan95,rhp  doc: mention system symbol table global, sysSymTbl (SPR#2575)
04b,16sep94,rhp  doc: fuller description of SYM_TYPE (SPR#3538).
04a,19may94,dvs  doc'ed problem of standalone image symbol removal (SPR #3199). 
03z,23feb93,jdi  doc: fuller description of <group> in symAdd().
03y,21jan93,jdi  documentation cleanup for 5.1.
03x,01oct92,jcf  biased symFindByValueAndType against xxx.o and gcc2_compiled.
03w,18jul92,smb  Changed errno.h to errnoLib.h.
03v,14jul92,jmm  added support to symKeyCmpName for matching pointers exactly
03u,04jul92,jcf  scalable/ANSI/cleanup effort.
03t,22jun92,jmm  removed symFooWithGroup, added parameter to routines instead
03s,26may92,rrr  the tree shuffle
03r,15may92,jmm  Changed "symFooGroup" to "symFooWithGroup"
03q,30apr92,jmm  Added support for group numbers
03p,02jan92,gae  used UINT's for value in symFindByValue{AndType}
                 for targets in negative address space (e.g. STAR MVP).
03o,13dec91,gae  ANSI cleanup.  Made symFindByCName() consistent with other
		   symFind* routines.
03n,26nov91,llk  added symName() and symNameValueCmp().
03m,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -changed TINY and UTINY to INT8 and UINT8
		  -changed VOID to void
		  -changed copyright notice
03l,20may91,jdi	 documentation tweak.
03k,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by dnw.
03j,24mar91,jdi  documentation cleanup.
03i,05oct90,dnw  made symFindByCName(), symFindByValueAndType(),
		   symFindSymbol(), symInit(), symTblTerminate() be NOMANUAL
		 made symFindByValueAndType bias against symbols ending in
		   "_text", "_data", "_bss"
03h,05oct90,dnw  added forward declarations.
03g,29sep90,jcf  documentation.
03f,02aug90,jcf  documentation.
03e,17jul90,dnw  changed to new objAlloc() call
03d,05jul90,jcf  documentation.
03c,20jun90,jcf  changed binary semaphore interface
		 changed ffs() changed ffsMsb()
03b,15apr90,jcf  included sysSymTbl.h.
03a,17nov89,jcf  rewritten to use hashLib (2).
		  changed interface/funtionality of symDelete/symInit/symCreate.
		  documentation.
02o,29nov89,llk  added symFindByCName for vxgdb.
02n,07jun89,gae  added symDelete.
02m,01sep88,gae  documentation.
02l,07jul88,jcf  changed malloc to match new declaration.
02k,30may88,dnw  changed to v4 names.
02j,04nov87,ecs  documentation.
02i,16aug87,gae  added parm. to symEach() and made it void.
02h,07jul87,ecs  added symFindType.
		 optimized symEach, symValFind, and symFind.
02g,02apr87,ecs  added include of strLib.c.
02f,23mar87,jlf  documentation
		 changed memAllocate's to malloc's.
02e,21dec86,dnw  changed to not get include files from default directories.
02d,04sep86,jlf  minor documentation
02c,01jul86,jlf  minor documentation
02b,05jun86,dnw  changed SYMTAB to have pointer to SYMBOL array instead of
		   having it imbedded in SYMTAB structure.
02a,11oct85,dnw  changed to dynamically allocate space for symbol strings.
		   Note change to symCreate call which now takes max number of
		   symbols instead of number of bytes in table.
		 De-linted.
01j,21jul85,jlf  documentation.
01i,11jun85,rdc  added variable length symbols.
01h,08sep84,jlf  added comments and copyright
01g,13aug84,ecs  changed status code EMPTY_SYMBOL_TABLE to SYMBOL_NOT_FOUND.
01f,08aug84,ecs  added calls to setStatus to symAdd, symFind, and symValFind.
01e,27jun84,ecs  changed symAdd & symValFind so that names in symbol tables
		   are now EOS terminated.
01d,26jun84,dnw  fixed bug in symValFind of potentially copying too many
		   bytes to user's name buffer.
01c,18jun84,jlf  added symEach and symValFind.
01b,03jun84,dnw  added symCreate.
		 changed symFind to search backwards in table to find most
		   recent of multiply defined symbols.
		 moved typedefs of SYMBOL and SYMTAB to symLib.h.
		 added LINTLIBRARY and various coercions for lint.
01a,03aug83,dnw  written
*/

/*
DESCRIPTION
This library provides facilities for managing symbol tables.  A symbol
table associates a name and type with a value.  A name is simply an
arbitrary, null-terminated string.  A symbol type is a small integer
(typedef SYM_TYPE), and its value is a pointer.  Though commonly used
as the basis for object loaders, symbol tables may be used whenever
efficient association of a value with a name is needed.

If you use the symLib subroutines to manage symbol tables local to
your own applications, the values for SYM_TYPE objects are completely
arbitrary; you can use whatever one-byte integers are appropriate for
your application.

If you use the symLib subroutines to manipulate the VxWorks system
symbol table (whose ID is recorded in the global \f3sysSymTbl\f1), the
values for SYM_TYPE are SYM_UNDF, SYM_LOCAL, SYM_GLOBAL, SYM_ABS,
SYM_TEXT, SYM_DATA, SYM_BSS, and SYM_COMM (defined in symbol.h).

Tables are created with symTblCreate(), which returns a symbol table ID.
This ID serves as a handle for symbol table operations, including the
adding to, removing from, and searching of tables.  All operations on a
symbol table are interlocked by means of a mutual-exclusion semaphore
in the symbol table structure.  Tables are deleted with symTblDelete().

Symbols are added to a symbol table with symAdd().  Each symbol in the
symbol table has a name, a value, and a type.  Symbols are removed from a
symbol table with symRemove().

Symbols can be accessed by either name or value.  The routine
symFindByName() searches the symbol table for a symbol with a
specified name.  The routine symByValueFind() finds a symbol with a
specified value or, if there is no symbol with the same value, the
symbol in the table with the next lower value than the specified
value.  The routines symFindByNameAndType() and
symByValueAndTypeFind() allow the symbol type to be used as an
additional criterion in the searches.

The routines symFindByValue() and symFindByValueAndType() are
obsolete.  They are replaced by the routines symByValueFind() and
symByValueAndTypeFind().

Symbols in the symbol table are hashed by name into a hash table for
fast look-up by name, e.g., by symFindByName().  The size of the hash
table is specified during the creation of a symbol table.  Look-ups by
value, e.g., symByValueFind(), must search the table linearly; these
look-ups can thus be much slower.

The routine symEach() allows each symbol in the symbol table to be
examined by a user-specified function.

Name clashes occur when a symbol added to a table is identical in name
and type to a previously added symbol.  Whether or not symbol tables
can accept name clashes is set by a parameter when the symbol table is
created with symTblCreate().  If name clashes are not allowed,
symAdd() will return an error if there is an attempt to add a symbol
with identical name and type.  If name clashes are allowed, adding
multiple symbols with the same name and type will be permitted.  In
such cases, symFindByName() will return the value most recently added,
although all versions of the symbol can be found by symEach().

The system symbol table (\f3sysSymTbl\f1) allows name clashes.

See the VxWorks Programmmer's Guide for more information about
configuration, intialization, and use of the system symbol table.

INTERNAL
Static symbol tables are initialized with symTblInit(), which operates
identically with symTblCreate() without allocating the symbol table.

Static symbol tables are terminated with symTblTerminate(), which operates
identically with symTblDelete() without deallocating the symbol table.

Symbols may be separately allocated, initialized, and added to the symbol
table.  symInit() initializes an existing uninitialized SYMBOL structure.
symAlloc() allocates a SYMBOL structure from the memory partition
specified to symTblCreate(), and then initializes it.  symTblAdd() adds an
existing initialized SYMBOL structure to a symbol table.  The usual
symAdd() function invokes both symAlloc() and symTblAdd().

Symbols are deallocated by symFree().  Symbols still resident in a symbol
table cannot be deallocated.

INCLUDE FILES: symLib.h

SEE ALSO: loadLib 
*/

/* LINTLIBRARY */

#include "vxWorks.h"
#include "errnoLib.h"
#include "objLib.h"
#include "classLib.h"
#include "symLib.h"
#include "semLib.h"
#include "memLib.h"
#include "memPartLib.h"
#include "hashLib.h"
#include "string.h"
#include "stdlib.h"
#include "sysSymTbl.h"
#include "private/funcBindP.h"

IMPORT int ffsMsb (int bitfield);

#define SYM_HFUNC_SEED	1370364821		/* magic seed */

typedef struct		/* RTN_DESC - routine descriptor */
    {
    FUNCPTR	routine;	/* user routine passed to symEach() */
    int		routineArg;	/* user routine arg passed to symEach() */
    } RTN_DESC;

/* local variables */

LOCAL OBJ_CLASS symTblClass;

/* global variables */

CLASS_ID symTblClassId = &symTblClass;
int mutexOptionsSymLib = SEM_Q_FIFO | SEM_DELETE_SAFE;
UINT16 symGroupDefault = 0;
FUNCPTR syncSymAddRtn = (FUNCPTR) NULL;
FUNCPTR syncSymRemoveRtn = (FUNCPTR) NULL;

/* forward static functions */
 
static BOOL symEachRtn (SYMBOL *pSymbol, RTN_DESC *pRtnDesc);
static int symHFuncName (int elements, SYMBOL *pSymbol, int seed);
static BOOL symKeyCmpName (SYMBOL *pMatchSymbol, SYMBOL *pSymbol,
			   int mask);
static BOOL symNameValueCmp (char *name, int val, SYM_TYPE type, int pSym);


/*******************************************************************************
*
* symLibInit - initialize the symbol table library
*
* This routine initializes the symbol table package.  If the configuration
* macro INCLUDE_SYM_TBL is defined, symLibInit() is called by the root task,
* usrRoot(), in usrConfig.c.
*
* RETURNS: OK, or ERROR if the library could not be initialized.
*/

STATUS symLibInit (void)
    {
    /* avoid direct coupling with symLib with these global variables */

    _func_symFindSymbol = (FUNCPTR) symFindSymbol;
    _func_symNameGet    = (FUNCPTR) symNameGet;
    _func_symValueGet   = (FUNCPTR) symValueGet;
    _func_symTypeGet    = (FUNCPTR) symTypeGet;
   
    /* 
     * XXX JLN The functions symFindByValueAndType and symFindByValue
     * are obsolete and should not be used.  They remain (as functions
     * and as function pointers) for backwards compatibility only.
     * The function pointers should be removed when all uses are
     * removed from vxWorks.  The functions themselves must remain
     * as long as required for backwards compatibility.
     */

    _func_symFindByValueAndType = (FUNCPTR) symFindByValueAndType; 
    _func_symFindByValue        = (FUNCPTR) symFindByValue;

    /* initialize the symbol table class structure */

    return (classInit (symTblClassId, sizeof (SYMTAB), OFFSET (SYMTAB, objCore),
		       (FUNCPTR) symTblCreate, (FUNCPTR) symTblInit,
		       (FUNCPTR) symTblDestroy));
    }

/*******************************************************************************
*
* symTblCreate - create a symbol table
*
* This routine creates and initializes a symbol table with a hash table of a
* specified size.  The size of the hash table is specified as a power of two.
* For example, if <hashSizeLog2> is 6, a 64-entry hash table is created.
*
* If <sameNameOk> is FALSE, attempting to add a symbol with
* the same name and type as an already-existing symbol results in an error.
*
* Memory for storing symbols as they are added to the symbol table will be
* allocated from the memory partition <symPartId>.  The ID of the system
* memory partition is stored in the global variable `memSysPartId', which
* is declared in memLib.h.
*
* RETURNS: Symbol table ID, or NULL if memory is insufficient.
*/

SYMTAB_ID symTblCreate
    (
    int     hashSizeLog2,       /* size of hash table as a power of 2 */
    BOOL    sameNameOk,         /* allow 2 symbols of same name & type */
    PART_ID symPartId           /* memory part ID for symbol allocation */
    )
    {
    SYMTAB_ID symTblId = (SYMTAB_ID) objAlloc (symTblClassId);

    if (symTblId != NULL)
	{
	symTblId->nameHashId = hashTblCreate (hashSizeLog2,
					      (FUNCPTR) symKeyCmpName,
					      (FUNCPTR) symHFuncName,
					      SYM_HFUNC_SEED);

	if (symTblId->nameHashId == NULL)	/* hashTblCreate failed? */
	    {
	    objFree (symTblClassId, (char *) symTblId);
	    return (NULL);
	    }

	if (symTblInit (symTblId, sameNameOk, symPartId,
			symTblId->nameHashId) != OK)
	    {
	    hashTblDelete (symTblId->nameHashId);
	    objFree (symTblClassId, (char *) symTblId);
	    return (NULL);
	    }
	}

    return (symTblId);				/* return the symbol table ID */
    }

/*******************************************************************************
*
* symTblInit - initialize a symbol table
*
* Initialize a symbol table.  Any symbols currently in the table will be lost.
* The specified hash table ID and memory partition ID must already be
* initialized.
*
* RETURNS: OK, or ERROR if initialization failed.
*
* NOMANUAL
*/

STATUS symTblInit
    (
    SYMTAB  *pSymTbl,           /* ptr to symbol table to initialize */
    BOOL    sameNameOk,         /* two symbols of same name and type allowed */
    PART_ID symPartId,          /* partition from which to allocate symbols */
    HASH_ID symHashTblId        /* ID of an initialized hash table */
    )
    {
    if ((OBJ_VERIFY (symHashTblId, hashClassId) != OK) ||
        (semMInit (&pSymTbl->symMutex, mutexOptionsSymLib) != OK))
	{
	return (ERROR);
	}

    pSymTbl->sameNameOk = sameNameOk;		/* name clash policy */
    pSymTbl->nsymbols   = 0;			/* initial number of syms */
    pSymTbl->nameHashId = symHashTblId;		/* fill in hash table ID */
    pSymTbl->symPartId  = symPartId;		/* fill in mem partition ID */

    /* initialize core */

    objCoreInit (&pSymTbl->objCore, symTblClassId);

    return (OK);
    }

/*******************************************************************************
*
* symTblDelete - delete a symbol table
*
* This routine deletes a specified symbol table.  It deallocates all
* associated memory, including the hash table, and marks the table as
* invalid.
*
* Deletion of a table that still contains symbols results in ERROR.
* Successful deletion includes the deletion of the internal hash table and
* the deallocation of memory associated with the table.  The table is marked
* invalid to prohibit any future references.
*
* RETURNS: OK, or ERROR if the symbol table ID is invalid.
*/

STATUS symTblDelete
    (
    SYMTAB_ID symTblId          /* ID of symbol table to delete */
    )
    {
    return (symTblDestroy (symTblId, TRUE));
    }

/*******************************************************************************
*
* symTblTerminate - terminate a symbol table
*
* The specified symbol table is terminated if no symbols are resident.
* Otherwise, ERROR is returned.
*
* RETURNS: OK, or ERROR if invalid symbol table ID, or symbols still in table.
*
* NOMANUAL
*/

STATUS symTblTerminate
    (
    SYMTAB_ID symTblId          /* ID of symbol table to delete */
    )
    {
    return (symTblDestroy (symTblId, FALSE));
    }

/*******************************************************************************
*
* symTblDestroy - destroy a symbol table and optionally deallocate memory
*
* The specified symbol table is terminated if no symbols are resident.
* If dealloc is TRUE, memory associated with the symbol table, including the
* hash table, is deallocated.
*
* RETURNS: OK, or ERROR if invalid symbol table ID, or symbols still in table.
*
* NOMANUAL
*/

STATUS symTblDestroy
    (
    SYMTAB_ID symTblId,         /* ID of symbol table to delete */
    BOOL      dealloc           /* deallocate associated memory */
    )
    {
    if (OBJ_VERIFY (symTblId, symTblClassId) != OK)
	return (ERROR);				/* invalid symbol table ID */

    semTake (&symTblId->symMutex, WAIT_FOREVER);

    if (symTblId->nsymbols > 0)			/* non-empty table? */
	{
	semGive (&symTblId->symMutex);		/* release mutual exclusion */

	errno = S_symLib_TABLE_NOT_EMPTY;
	return (ERROR);
	}

    semTerminate (&symTblId->symMutex);		 /* terminate mutex */

    objCoreTerminate (&symTblId->objCore);

    hashTblDestroy (symTblId->nameHashId, dealloc);

    if (dealloc)
	return (objFree (symTblClassId, (char *) symTblId));

    return (OK);
    }

/*******************************************************************************
*
* symAlloc - allocate and initialize a symbol, including group number
*
* Allocate and initialize a symbol.  The symbol is not added to any symbol
* table.  To add a symbol to a symbol table use symAdd() or symTblAdd().
* Space for the name is allocated along with the SYMBOL structure, thus the
* name parameter need not be static.
*
* RETURNS: Pointer to symbol, or NULL if out of memory.
*
* NOMANUAL
*/

SYMBOL *symAlloc
    (
    SYMTAB_ID   symTblId,       /* symbol table to allocate symbol for */
    char        *name,          /* pointer to symbol name string */
    char        *value,         /* symbol address */
    SYM_TYPE    type,           /* symbol type */
    UINT16	group		/* symbol group */
    )
    {
    SYMBOL *pSymbol;
    char   *symName;
    int	   length;

    if (OBJ_VERIFY (symTblId, symTblClassId) != OK)
	return (NULL);				/* invalid symbol table ID */

    if (name == NULL)
	return (NULL);				/* null name */

    length = strlen (name);			/* figure out name length */

    pSymbol = (SYMBOL *) memPartAlloc (symTblId->symPartId,
				       (unsigned)(sizeof(SYMBOL) + length + 1));

    if (pSymbol == NULL)			/* out of memory */
	return (NULL);

    /* copy name after symbol */

    symName	    = (char *) ((unsigned) pSymbol + sizeof (SYMBOL));
    symName[length] = EOS;			/* null terminate string */

    strncpy (symName, name, length);		/* copy name into place */

    symInit (pSymbol, symName, value, type, group); /* initialize symbol*/

    return (pSymbol);				/* return symbol ID */
    }

/*******************************************************************************
*
* symInit - initialize a symbol, including group number
*
* Initialize a symbol.  The symbol is not added to any symbol table.  To add
* a symbol to a symbol table use symAdd() or symTblAdd().
*
* RETURNS: OK, or ERROR if symbol table could not be initialized.
*
* NOMANUAL
*/

STATUS symInit
    (
    SYMBOL      *pSymbol,       /* pointer to symbol */
    char        *name,          /* pointer to symbol name string */
    char        *value,         /* symbol address */
    SYM_TYPE    type,           /* symbol type */
    UINT16	group		/* symbol group */
    )
    {
    /* fill in symbol */

    pSymbol->name  = name;			/* symbol name */
    pSymbol->value = (void *)value;		/* symbol value */
    pSymbol->type  = type;			/* symbol type */
    pSymbol->group = group;			/* symbol group */

    return (OK);
    }

/*******************************************************************************
*
* symFree - deallocate a symbol
*
* Deallocate the specified symbol.  This routine does not check if symbols
* are still resident in symbol tables, so all calls to symFree should be
* preceded by a call to symTblRemove.
*
* RETURNS: OK, or ERROR if invalid symbol table.
*
* NOMANUAL
*/

STATUS symFree
    (
    SYMTAB_ID symTblId, /* symbol table semaphore */
    SYMBOL *pSymbol     /* pointer to symbol to delete */
    )
    {
    if (OBJ_VERIFY (symTblId, symTblClassId) != OK)
	return (ERROR);				/* invalid symbol table ID */

    return (memPartFree (symTblId->symPartId, (char *) pSymbol));
    }

/*******************************************************************************
*
* symSAdd - create and add a symbol to a symbol table, including a group number
* 
* This routine behaves as symAdd() except it does not check for synchronization
* function pointers. Thus it can be used in loaders to prevent from trying to 
* independently synchronize each symbol of a module.
*
* RETURNS: OK, or ERROR if the symbol table is invalid
* or there is insufficient memory for the symbol to be allocated.
*
* NOMANUAL
*/

STATUS symSAdd
    (
    SYMTAB_ID symTblId,         /* symbol table to add symbol to */
    char      *name,            /* pointer to symbol name string */
    char      *value,           /* symbol address */
    SYM_TYPE  type,             /* symbol type */
    UINT16    group             /* symbol group */
    )
    {
    SYMBOL *pSymbol = symAlloc (symTblId, name, value, type, group);

    if (pSymbol == NULL)			/* out of memory? */
	return (ERROR);

    if (symTblAdd (symTblId, pSymbol) != OK)	/* try to add symbol */
	{
	symFree (symTblId, pSymbol);		/* deallocate symbol if fail */
	return (ERROR);
	}
    
    return (OK);
    }

/*******************************************************************************
*
* symAdd - create and add a symbol to a symbol table, including a group number
*
* This routine allocates a symbol <name> and adds it to a specified symbol
* table <symTblId> with the specified parameters <value>, <type>, and <group>.
* The <group> parameter specifies the group number assigned to a module when
* it is loaded; see the manual entry for moduleLib.
*
* RETURNS: OK, or ERROR if the symbol table is invalid
* or there is insufficient memory for the symbol to be allocated.
*
* SEE ALSO: moduleLib
*/

STATUS symAdd
    (
    SYMTAB_ID symTblId,         /* symbol table to add symbol to */
    char      *name,            /* pointer to symbol name string */
    char      *value,           /* symbol address */
    SYM_TYPE  type,             /* symbol type */
    UINT16    group             /* symbol group */
    )
    {
    SYMBOL *pSymbol = symAlloc (symTblId, name, value, type, group);

    if (pSymbol == NULL)			/* out of memory? */
	return (ERROR);

    if (symTblAdd (symTblId, pSymbol) != OK)	/* try to add symbol */
	{
	symFree (symTblId, pSymbol);		/* deallocate symbol if fail */
	return (ERROR);
	}
    
    /* synchronize host symbol table if necessary */

    if ((syncSymAddRtn != NULL)	&& (symTblId == sysSymTbl))
	(* syncSymAddRtn) (name, value, type, group);

    return (OK);
    }

/*******************************************************************************
*
* symTblAdd - add a symbol to a symbol table
*
* This routine adds a symbol to a symbol table.
*
* RETURNS: OK, or ERROR if invalid symbol table, or symbol couldn't be added.
*
* INTERNAL
* This is a lousy name, it should probably be symAddStatic ().
*
* NOMANUAL
*/

STATUS symTblAdd
    (
    SYMTAB_ID symTblId,         /* symbol table to add symbol to */
    SYMBOL    *pSymbol          /* pointer to symbol to add */
    )
    {
    if (OBJ_VERIFY (symTblId, symTblClassId) != OK)
	return (ERROR);				/* invalid symbol table ID */

    semTake (&symTblId->symMutex, WAIT_FOREVER);

    if ((!symTblId->sameNameOk) &&
	(hashTblFind (symTblId->nameHashId, &pSymbol->nameHNode,
		      SYM_MASK_EXACT_TYPE) != NULL))
	{
	semGive (&symTblId->symMutex);		/* release exclusion to table */

	errno = S_symLib_NAME_CLASH;		/* name clashed */
	return (ERROR);
	}

    hashTblPut (symTblId->nameHashId, &pSymbol->nameHNode);

    symTblId->nsymbols ++;			/* increment symbol count */

    semGive (&symTblId->symMutex);		/* release exclusion to table */

    return (OK);
    }

/*******************************************************************************
*
* symRemove - remove a symbol from a symbol table
*
* This routine removes a symbol of matching name and type from a
* specified symbol table.  The symbol is deallocated if found.
* Note that VxWorks symbols in a standalone VxWorks image (where the 
* symbol table is linked in) cannot be removed.
*
* RETURNS: OK, or ERROR if the symbol is not found
* or could not be deallocated.
*/

STATUS symRemove
    (
    SYMTAB_ID symTblId,         /* symbol tbl to remove symbol from */
    char      *name,            /* name of symbol to remove */
    SYM_TYPE  type              /* type of symbol to remove */
    )
    {
    SYMBOL *pSymbol;

    if (symFindSymbol (symTblId, name, NULL, 
		       type, SYM_MASK_EXACT_TYPE, &pSymbol) != OK)
	return (ERROR);

    if (symTblRemove (symTblId, pSymbol) != OK)
	return (ERROR);

    /* synchronize host symbol table if necessary */

    if ((syncSymRemoveRtn != NULL) && (symTblId == sysSymTbl))
	(* syncSymRemoveRtn) (name, type);

    return (symFree (symTblId, pSymbol));
    }

/*******************************************************************************
*
* symTblRemove - remove and terminate a symbol from a symbol table
*
* This routine removes the specified symbol from the symbol table.  The
* symbol is not deallocated.
*
* RETURNS: OK, or ERROR if symbol table invalid, or symbol not found.
*
* NOMANUAL
*/

STATUS symTblRemove
    (
    SYMTAB_ID symTblId,         /* symbol table to remove symbol from */
    SYMBOL    *pSymbol          /* pointer to symbol to remove */
    )
    {
    HASH_NODE *pNode;

    if (OBJ_VERIFY (symTblId, symTblClassId) != OK)
	return (ERROR);				/* invalid symbol table ID */

    semTake (&symTblId->symMutex, WAIT_FOREVER);

    pNode = hashTblFind (symTblId->nameHashId, &pSymbol->nameHNode,
			 SYM_MASK_EXACT);

    if (pNode == NULL)
	{
	semGive (&symTblId->symMutex);		/* release exclusion to table */

	errnoSet (S_symLib_SYMBOL_NOT_FOUND);	/* symbol wasn't found */
	return (ERROR);
	}

    hashTblRemove (symTblId->nameHashId, pNode);

    symTblId->nsymbols--;			/* one less symbol */

    semGive (&symTblId->symMutex);		/* release exclusion to table */

    return (OK);
    }

/*******************************************************************************
*
* symFindSymbol - find symbol in a symbol table of equivalent name and type
*
* This routine is a generic routine to retrieve a symbol in the
* specified symbol table.  It can be used to perform searches by name,
* name and type, value, or value and type.
* 
* If the 'name' parameter is non-NULL, a search by name will be
* performed and the value parameter will be ignored.  If the name
* parameter is NULL, a search by value will be performed.  In a search
* by value, if no matching entry is found, the symbol of the matching
* type with the next lower value is selected.  (If the type is NULL, as
* in a simple search by value, this will simply be whatever symbol
* in the symbol table has the next lowest address.)
*
* In both the search by name and search by value cases, potential
* matches will have their types compared to <type>, subject to the
* <mask>, and only symbols which match the specified type will be
* returned.  To have the type matched exactly, set the <type>
* parameter to the desired type and set the <mask> parameter to
* SYM_MASK_EXACT_TYPE.  To search only by name or only by value and
* have the <type> parameter ignored, set the <mask> parameter to
* SYM_MATCH_ANY_TYPE.
*
* To search the global VxWorks symbol table, specify \f3sysSymTbl\f1
* as <symTblId>.
* 
* INTERNAL
* This routine contains a weird hack designed to deal with the additional
* symbols the loader puts in the symbol table.  The loader adds three symbols
* to the symbol table: <filename>_text, <filename>_data, <filename>_bss.
* These symbols may have the same address (i.e. value in the symbol table)
* as real routine or variable names.  When looking up a symbol for display
* it is desirable to find the real routine or variable names in preference
* to these made up names.  For example, loading "demo.o" will cause a
* symbol "demo.o_text" to be added to the symbol table with the same value
* as the real symbol "_demo".  In a disassembly or "i" printout, etc, we
* would rather see "_demo".  So the test inside the loop in this routine
* that normally terminates the search if we find an exact match, has been
* changed to keep searching if the match it finds ends in "_text", "_data",
* or "_bss".  
*
* If no other exact match is found, the special symbols will be returned
* anyway.  Thus this routine simply has a "bias" against such symbols, but
* will not return an erroneous result in any event.  This nonsense should be
* removed when the loader is changed to not add the special symbols.
*
* The GNU toolkit adds symbols of the form "gcc2_compiled." and "xxx.o".
* These symbols are also biased against.  
*
* RETURNS: OK, or ERROR if <symTblId> is invalid, <pSymbolId> is NULL,
*          symbol not found (for a search by name), or if <value> is less than
*          the lowest value in the table (for search by value).
*
* NOMANUAL */

STATUS symFindSymbol
    (
    SYMTAB_ID   symTblId,       /* symbol table ID */
    char *      name,           /* name to search for */
    void * 	value,		/* value of symbol to search for */
    SYM_TYPE    type,           /* symbol type */
    SYM_TYPE    mask,           /* type bits that matter */
    SYMBOL_ID * pSymbolId       /* where to return id of matching symbol */
    )
    {
    HASH_NODE *         pNode;      /* node in symbol hash table */
    SYMBOL              keySymbol;  /* dummy symbol for search by name */
    int	                index;      /* counter for search by value */
    SYMBOL *	        pSymbol;    /* current symbol, search by value */
    SYMBOL *	        pBestSymbol = NULL; 
                                    /* symbol with lower value, matching type */
    char *		pUnder;     /* string for _text, etc., check */
    void *		bestValue = NULL; 
                                    /* current value of symbol with matching 
				       type */ 

    if ((symTblId == NULL) || (OBJ_VERIFY (symTblId, symTblClassId) != OK))
        {
	errnoSet (S_symLib_INVALID_SYMTAB_ID);	
	return (ERROR); 
	}

    if (pSymbolId == NULL) 
        {
	errnoSet (S_symLib_INVALID_SYM_ID_PTR);	
	return (ERROR); 
	}

    if (name != NULL) 
        {
	/* Search by name or by name and type: */
	  
	/* fill in keySymbol */

	keySymbol.name = name;			/* match this name */
	keySymbol.type = type;			/* match this type */

	semTake (&symTblId->symMutex, WAIT_FOREVER);

	pNode = hashTblFind (symTblId->nameHashId, &keySymbol.nameHNode, 
			     (int)mask);

	semGive (&symTblId->symMutex);		/* release exclusion to table */

	if (pNode == NULL)
	    {
	    errnoSet (S_symLib_SYMBOL_NOT_FOUND);    /* couldn't find symbol */
	    return (ERROR);
	    }

	*pSymbolId = (SYMBOL_ID) pNode;

	}
    else 
        {
	/* Search by value or by value and type: */
  
	semTake (&symTblId->symMutex, WAIT_FOREVER);

	for (index = 0; index < symTblId->nameHashId->elements; index++)
	    {
	    pSymbol = 
	        (SYMBOL *) SLL_FIRST(&symTblId->nameHashId->pHashTbl [index]);

	    while (pSymbol != NULL)			/* list empty */
	        {
		if (((pSymbol->type & mask) == (type & mask)) &&
		    (pSymbol->value == value) &&
		    (((pUnder = rindex (pSymbol->name, '_')) == NULL) ||
		     ((strcmp (pUnder, "_text") != 0) &&
		      (strcmp (pUnder, "_data") != 0) &&
		      (strcmp (pUnder, "_bss") != 0) &&
		      (strcmp (pUnder, "_compiled.") != 0))) &&
		    (((pUnder = rindex (pSymbol->name, '.')) == NULL) ||
		     ((strcmp (pUnder, ".o") != 0))))
		    {
		    /* We've found the entry.  Return it. */

		    *pSymbolId = pSymbol;

		    semGive (&symTblId->symMutex);

		    return (OK);
		    }

		else if (((pSymbol->type & mask) == (type & mask)) &&
			 ((pSymbol->value <= value) &&
			  (pSymbol->value > bestValue)))
		    {
		    /* 
		     * this symbol is of correct type and closer than last one 
		     */

		    bestValue   = pSymbol->value;
		    pBestSymbol = pSymbol;
		    }

		pSymbol = (SYMBOL *) SLL_NEXT (&pSymbol->nameHNode);
		}
	    }
	
	if (bestValue == NULL || pBestSymbol == NULL)	/* any closer symbol? */
	    {
	    semGive (&symTblId->symMutex);	/* release exclusion to table */

	    errnoSet (S_symLib_SYMBOL_NOT_FOUND);
	    return (ERROR);
	    }

	*pSymbolId = pBestSymbol;

	semGive (&symTblId->symMutex);		/* release exclusion to table */
	}

    return (OK);
    }

/*******************************************************************************
*
* symFindByName - look up a symbol by name
*
* This routine searches a symbol table for a symbol matching a specified
* name.  If the symbol is found, its value and type are copied to <pValue>
* and <pType>.  If multiple symbols have the same name but differ in type,
* the routine chooses the matching symbol most recently added to the symbol
* table.
*
* To search the global VxWorks symbol table, specify \f3sysSymTbl\f1
* as <symTblId>.
* 
* RETURNS: OK, or ERROR if the symbol table ID is invalid
* or the symbol cannot be found.
*/

STATUS symFindByName
    (
    SYMTAB_ID   symTblId,       /* ID of symbol table to look in */
    char        *name,          /* symbol name to look for */
    char        **pValue,       /* where to put symbol value */
    SYM_TYPE    *pType          /* where to put symbol type */
    )
    {
    return (symFindByNameAndType (symTblId, name, pValue, pType, 
				  SYM_MASK_ANY_TYPE, SYM_MASK_ANY_TYPE));
    }

/*******************************************************************************
*
* symByCNameFind - find a symbol in a symbol table, look for a '_'
*
* Find a symbol in the table, by name.  If the symbol isn't found,
* try looking for it with a '_' prepended to it.  If we find the
* symbol, we fill in the type and the value of the symbol.
*
* RETURNS: OK, or ERROR if <symTblId> is invalid or symbol is not found.
*
* INTERNAL
* The size of the symbol name is NOT limited.
*
* NOMANUAL
*/

STATUS symByCNameFind
    (
    SYMTAB_ID   symTblId,	/* ID of symbol table to look in */
    char *	name,		/* symbol name to look for   */
    char **	pValue,		/* where to put symbol value */
    SYM_TYPE *	pType		/* where to put symbol type  */
    )
    {
    char *	symBuf = NULL;
    STATUS	retVal;

    if (symFindByName (symTblId, name, pValue, pType) == ERROR)
	{
	/* prepend a '_' and try again */

	/* 
	 * XXX JLN - KHEAP_ALLOC isn't necessary for this function -
	 * it should be rewritten to search directly and not use KHEAP_ALLOC. 
	 */

        if ((symBuf = (char *) KHEAP_ALLOC (strlen (name) + 2)) == NULL)
            return ERROR;

        *symBuf = '_';
        strcpy (&symBuf[1], name);

        retVal = symFindByName (symTblId, symBuf, pValue, pType);

        KHEAP_FREE (symBuf);

        return (retVal);
	}

    return (OK);
    }

/*******************************************************************************
*
* symFindByCName - find a symbol in a symbol table, look for a '_'
*
* This routine is obsolete.  It is replaced by the routine
* symByCNameFind().  

* Find a symbol in the table, by name.  If the symbol isn't found, try
* looking for it with a '_' prepended to it.  If we find the symbol,
* we fill in the type and the value of the symbol.
*
* RETURNS: OK, or ERROR if <symTblId> is invalid or symbol is not found.
*
* INTERNAL
* The size of the symbol name is no longer limited by MAX_SYS_SYM_LEN.
*  
* NOMANUAL 
*/

STATUS symFindByCName
    (
    SYMTAB_ID   symTblId,	/* ID of symbol table to look in */
    char        *name,		/* symbol name to look for   */
    char        **pValue,       /* where to put symbol value */
    SYM_TYPE    *pType		/* where to put symbol type  */
    )
    {
    return symByCNameFind (symTblId, name, pValue, pType);
    }

/*******************************************************************************
*
* symFindByNameAndType - look up a symbol by name and type
*
* This routine searches a symbol table for a symbol matching both name and
* type (<name> and <sType>).  If the symbol is found, its value and type are
* copied to <pValue> and <pType>.  The <mask> parameter can be used to match
* sub-classes of type.
*
* To search the global VxWorks symbol table, specify \f3sysSymTbl\f1
* as <symTblId>.
* 
* RETURNS: OK, or ERROR if the symbol table ID is invalid
* or the symbol is not found.
*/

STATUS symFindByNameAndType
    (
    SYMTAB_ID   symTblId,       /* ID of symbol table to look in */
    char        *name,          /* symbol name to look for */
    char        **pValue,       /* where to put symbol value */
    SYM_TYPE    *pType,         /* where to put symbol type */
    SYM_TYPE    sType,          /* symbol type to look for */
    SYM_TYPE    mask            /* bits in <sType> to pay attention to */
    )
    {
    SYMBOL	*pSymbol = NULL;

    if (symFindSymbol (symTblId, name, NULL, sType, 
		       mask, &pSymbol) == ERROR)
        return (ERROR);

    if (pValue != NULL)
        *pValue = (char *) pSymbol->value;
    
    if (pType != NULL)
        *pType = pSymbol->type;

    return OK;
    }

/*******************************************************************************
*
* symByValueFind - look up a symbol by value
*
* This routine searches a symbol table for a symbol matching a
* specified value.  If there is no matching entry, it chooses the
* table entry with the next lower value.  A pointer to a copy of the
* symbol name string (with terminating EOS) is returned into
* <pName>. The actual value and the type are copied to <pValue> and
* <pType>.
*
* <pName> is a pointer to memory allocated by symByValueFind; 
* the memory must be freed by the caller after the use of <pName>.
*
* To search the global VxWorks symbol table, specify \f3sysSymTbl\f1
* as <symTblId>.
*
* RETURNS: OK or ERROR if <symTblId> is invalid, <pName> is NULL, or
* <value> is less than the lowest value in the table.
*/

STATUS symByValueFind
    (
    SYMTAB_ID	symTblId,		/* ID of symbol table to look in */
    UINT	value,			/* value of symbol to find */
    char **	pName,			/* where return symbol name string */
    int  *	pValue,			/* where to put symbol value */
    SYM_TYPE *	pType			/* where to put symbol type */
    )
    {
    return (symByValueAndTypeFind (symTblId, value, pName, pValue, pType,
				   SYM_MASK_ANY_TYPE, SYM_MASK_ANY_TYPE));
    }

/*******************************************************************************
*
* symByValueAndTypeFind - look up a symbol by value and type
*
* This routine searches a symbol table for a symbol matching both
* value and type (<value> and <sType>).  If there is no matching
* entry, it chooses the table entry with the next lower value (among
* entries with the same type). A pointer to the symbol name string
* (with terminating EOS) is returned into <pName>. The actual value
* and the type are copied to <pValue> and <pType>. The <mask>
* parameter can be used to match sub-classes of type.
* 
* <pName> is a pointer to memory allocated by symByValueAndTypeFind; 
* the memory must be freed by the caller after the use of <pName>.
*
* To search the global VxWorks symbol table, specify \f3sysSymTbl\f1
* as <symTblId>.
*
* RETURNS: OK or ERROR if <symTblId> is invalid, <pName> is NULL, or
* <value> is less than the lowest value in the table.
*/

STATUS symByValueAndTypeFind
    (
    SYMTAB_ID	    symTblId,	/* ID of symbol table to look in */
    UINT	    value,	/* value of symbol to find */
    char **	    pName,	/* where to return symbol name string */
    int *	    pValue,	/* where to put symbol value */
    SYM_TYPE *	    pType,	/* where to put symbol type */
    SYM_TYPE        sType,	/* symbol type to look for */
    SYM_TYPE        mask	/* bits in <sType> to pay attention to */
    )
    {
    SYMBOL *	pSymbol = NULL;
   
    if (pName == NULL)
        return ERROR;

    if (symFindSymbol(symTblId, NULL, (char *)value, sType, 
		      mask, &pSymbol) != OK)
        return ERROR;

    if (pValue != NULL) 
        *pValue = (int) pSymbol->value;

    if (pType != NULL)
        *pType = pSymbol->type;

    if (((*pName) = (char *) malloc (strlen (pSymbol->name) + 1)) == NULL)
        return ERROR;

    strcpy ((*pName), pSymbol->name);
    
    return (OK);
    }

/*******************************************************************************
*
* symFindByValue - look up a symbol by value
*
* This routine is obsolete.  It is replaced by symByValueFind().
*
* This routine searches a symbol table for a symbol matching a specified
* value.  If there is no matching entry, it chooses the table entry with the
* next lower value.  The symbol name (with terminating EOS), the actual
* value, and the type are copied to <name>, <pValue>, and <pType>.
*
* For the <name> buffer, allocate MAX_SYS_SYM_LEN + 1 bytes.  The value
* MAX_SYS_SYM_LEN is defined in sysSymTbl.h.  If the name of the symbol 
* is longer than MAX_SYS_SYM_LEN bytes, it will be truncated to fit into the
* buffer.  Whether or not the name was truncated, the string returned in the
* buffer will be null-terminated.
*
* To search the global VxWorks symbol table, specify \f3sysSymTbl\f1
* as <symTblId>.
* 
* RETURNS: OK, or ERROR if <symTblId> is invalid or <value> is less
* than the lowest value in the table.  
*/

STATUS symFindByValue
    (
    SYMTAB_ID	symTblId,	/* ID of symbol table to look in */
    UINT	value,		/* value of symbol to find */
    char *	name,		/* where to put symbol name string */
    int *	pValue,		/* where to put symbol value */
    SYM_TYPE *	pType		/* where to put symbol type */
    )
    {
    return (symFindByValueAndType (symTblId, value, name, pValue, pType,
				   SYM_MASK_ANY_TYPE, SYM_MASK_ANY_TYPE));
    }

/*******************************************************************************
*
* symFindByValueAndType - look up a symbol by value and type
*
* This routine is obsolete.  It is replaced by the routine
* symByValueAndTypeFind().
*  
* This routine searches a symbol table for a symbol matching both value and
* type (<value> and <sType>).  If there is no matching entry, it chooses the
* table entry with the next lower value.  The symbol name (with terminating
* EOS), the actual value, and the type are copied to <name>, <pValue>, and
* <pType>.  The <mask> parameter can be used to match sub-classes of type.
*
* For the <name> buffer, allocate MAX_SYS_SYM_LEN + 1 bytes.  The value
* MAX_SYS_SYM_LEN is defined in sysSymTbl.h.  If the name of the symbol 
* is longer than MAX_SYS_SYM_LEN bytes, it will be truncated to fit into the
* buffer.  Whether or not the name was truncated, the string returned in the
* buffer will be null-terminated.
*
* To search the global VxWorks symbol table, specify \f3sysSymTbl\f1
* as <symTblId>.
* 
* INTERNAL
* This routine contains a weird hack designed to deal with the additional
* symbols the loader puts in the symbol table.  The loader adds three symbols
* to the symbol table: <filename>_text, <filename>_data, <filename>_bss.
* These symbols may have the same address (i.e. value in the symbol table)
* as real routine or variable names.  When looking up a symbol for display
* it is desirable to find the real routine or variable names in preference
* to these made up names.  For example, loading "demo.o" will cause a
* symbol "demo.o_text" to be added to the symbol table with the same value
* as the real symbol "_demo".  In a disassembly or "i" printout, etc, we
* would rather see "_demo".  So the test inside the loop in this routine
* that normally terminates the search if we find an exact match, has been
* changed to keep searching if the match it finds ends in "_text", "_data",
* or "_bss".  
*
* If no other exact match is found, the special symbols will be returned
* anyway.  Thus this routine simply has a "bias" against such symbols, but
* will not return an erroneous result in any event.  This nonsense should be
* removed when the loader is changed to not add the special symbols.
*
* The GNU toolkit adds symbols of the form "gcc2_compiled." and "xxx.o".
* These symbols are also biased against.  
*
* RETURNS: OK, or ERROR if <symTblId> is invalid or <value> is less
* than the lowest value in the table.  */

STATUS symFindByValueAndType
    (
    SYMTAB_ID     symTblId,     /* ID of symbol table to look in */
    UINT          value,        /* value of symbol to find */
    char *        name,         /* where to put symbol name string */
    int *         pValue,       /* where to put symbol value */
    SYM_TYPE *    pType,        /* where to put symbol type */
    SYM_TYPE      sType,        /* symbol type to look for */
    SYM_TYPE      mask          /* bits in <sType> to pay attention to */
    )
    {
    SYMBOL * pSymbol = NULL;
      
    if (symFindSymbol (symTblId, NULL, (char *) value, sType, 
		       mask, &pSymbol) != OK)
        {

	/* 
	 * remap the errno for backward compatibility - can't change 
	 * what errno's are returned for a particular failure mode
	 */

	if (errno == S_symLib_INVALID_SYMTAB_ID)   
	    errnoSet (S_objLib_OBJ_ID_ERROR);

        return ERROR;
	}

    /* 
     * XXX - JLN - This API is broken and should not be used anymore.
     * The next line is the best of several bad options for providing
     * some version of this routine for backward compatibility.  We
     * truncate the number of characters we return to match the size
     * that the documentation tells the caller to allocate.  
     */

    strncpy (name, pSymbol->name, MAX_SYS_SYM_LEN + 1);

    /* Null-terminate the string in case the name was truncated. */

    if (name[MAX_SYS_SYM_LEN] != EOS)
        name[MAX_SYS_SYM_LEN] = EOS;

    if (pValue != NULL)
        *pValue = (int) pSymbol->value; 
      
    if (pType != NULL)
        *pType = pSymbol->type;

    return (OK);
    }


/*******************************************************************************
*
* symEach - call a routine to examine each entry in a symbol table
*
* This routine calls a user-supplied routine to examine each entry in the
* symbol table; it calls the specified routine once for each entry.  The
* routine should be declared as follows:
* .CS
*     BOOL routine
*         (
*         char	    *name,  /@ entry name                  @/
*         int	    val,    /@ value associated with entry @/
*         SYM_TYPE  type,   /@ entry type                  @/
*         int	    arg,    /@ arbitrary user-supplied arg @/
*         UINT16    group   /@ group number                @/
*         )
* .CE
* The user-supplied routine should return TRUE if symEach() is to continue
* calling it for each entry, or FALSE if it is done and symEach() can exit.
*
* RETURNS: A pointer to the last symbol reached,
* or NULL if all symbols are reached.
*
* INTERNAL
* In addition to the parameters given, it also passes a pointer to a symbol
* as the last arguement.
*
*/

SYMBOL *symEach
    (
    SYMTAB_ID   symTblId,       /* pointer to symbol table */
    FUNCPTR     routine,        /* func to call for each tbl entry */
    int         routineArg      /* arbitrary user-supplied arg */
    )
    {
    SYMBOL   *pSymbol;
    RTN_DESC rtnDesc;

    if (OBJ_VERIFY (symTblId, symTblClassId) != OK)
	return (NULL);				/* invalid symbol table ID */

    /* fill in a routine descriptor with the routine and argument to call */

    rtnDesc.routine    = routine;
    rtnDesc.routineArg = routineArg;

    semTake (&symTblId->symMutex, WAIT_FOREVER);

    pSymbol = (SYMBOL *) hashTblEach (symTblId->nameHashId, symEachRtn,
				      (int) &rtnDesc);

    semGive (&symTblId->symMutex);		/* release exclusion to table */

    return (pSymbol);				/* symbol we stopped on */
    }

/*******************************************************************************
*
* symEachRtn - call a user routine for a hashed symbol
*
* This routine supports hashTblEach(), by unpackaging the routine descriptor
* and calling the user routine specified to symEach() with the right calling
* sequence.
*
* RETURNS: Boolean result of user specified symEach routine.
*
* NOMANUAL
*/

LOCAL BOOL symEachRtn
    (
    SYMBOL      *pSymbol,       /* ptr to symbol */
    RTN_DESC    *pRtnDesc       /* ptr to a routine descriptor */
    )
    {
    return ((* pRtnDesc->routine) (pSymbol->name, (int) pSymbol->value,
				   pSymbol->type, pRtnDesc->routineArg,
				   pSymbol->group, pSymbol));
    }

/*******************************************************************************
*
* symHFuncName - symbol name hash function
*
* This routine checksums the name and applies a multiplicative hashing function
* provided by hashFuncMultiply().
*
* RETURNS: An integer between 0 and (elements - 1).
*/

LOCAL int symHFuncName
    (
    int         elements,       /* no. of elements in hash table */
    SYMBOL      *pSymbol,       /* pointer to symbol */
    int         seed            /* seed to be used as scalar */
    )
    {
    int	 hash;
    char *tkey;
    int	 key = 0;

    /* checksum the string and use a multiplicative hashing function */

    for (tkey = pSymbol->name; *tkey != '\0'; tkey++)
	key = key + (unsigned int) *tkey;

    hash = key * seed;				/* multiplicative hash func */

    hash = hash >> (33 - ffsMsb (elements));	/* take only the leading bits */

    return (hash & (elements - 1));		/* mask hash to (0,elements-1)*/
    }

/*******************************************************************************
*
* symKeyCmpName - compare two symbol's names for equivalence
*
* This routine returns TRUE if the match symbol's type masked by the specified
* symbol mask, is equivalent to the second symbol's type also masked by the
* symbol mask, and if the symbol's names agree.
*
* RETURNS: TRUE if symbols match, FALSE if they differ.
*/

LOCAL BOOL symKeyCmpName
    (
    SYMBOL      *pMatchSymbol,          /* pointer to match criteria symbol */
    SYMBOL      *pSymbol,               /* pointer to symbol */
    int		maskArg                 /* symbol type bits than matter (int) */
    )
    {
    SYM_TYPE    mask;                   /* symbol type bits than matter (char)*/

    /*
     * If maskArg is equal to SYM_MASK_EXACT, then check to see if the pointers
     * match exactly.
     */

    if (maskArg == SYM_MASK_EXACT)
        return (pMatchSymbol == pSymbol ? TRUE : FALSE);

    mask = (SYM_TYPE) maskArg;
    return (((pSymbol->type & mask) == (pMatchSymbol->type & mask)) &&
	    (strcmp (pMatchSymbol->name, pSymbol->name) == 0));
    }

/*******************************************************************************
*
* symName - get the name associated with a symbol value
*
* This routine returns a pointer to the name of the symbol of specified value.
*
* RETURNS: A pointer to the symbol name, or
* NULL if symbol cannot be found or <symTbl> doesn't exist.
*
* NOMANUAL
*/

char *symName
    (
    SYMTAB_ID   symTbl,			/* symbol table */
    char        *value			/* symbol value */
    )
    {
    SYMBOL sym;

    sym.value = (void *)value;                  /* initialize symbol */
    sym.name  = NULL;

    (void)symEach (symTbl, (FUNCPTR) symNameValueCmp, (int) (&sym));

    return (sym.name);
    }

/*******************************************************************************
*
* symNameValueCmp - compares a symbol value with a name
*
* This routine was written to be invoked by symEach().
* <pSym> is a pointer to a symbol which contains a value that this routine
* compares to the passed symbol value <val>.  If the two values match, then the
* name field in <pSym>'s name field is set to point to the symbol name in the
* symbol table.
*
* RETURNS: FALSE if <val> matches <pSym>->value and sets <pSym>->name,
* or TRUE otherwise.
*/

LOCAL BOOL symNameValueCmp
    (
    char *name,         /* symbol name */
    int val,            /* symbol value */
    SYM_TYPE type,      /* symbol type -- not used */
    int pSym            /* pointer to symbol trying to be matched */
    )
    {
    if (val == (int) (((SYMBOL *)pSym)->value))
        {
        ((SYMBOL *)pSym)->name = name;
        return (FALSE);
        }
    return (TRUE);
    }

/*******************************************************************************
*
* symNameGet - get name of a symbol
* 
* This routine is currently intended only for internal use. 
*
* It provides the name of the symbol specified by the SYMBOL_ID
* <symbolId>.  The SYMBOL_ID of a symbol may be obtained by using the
* routine symFindSymbol().  A pointer to the symbol table's copy of the
* symbol name is returned in <pName>.
*
* RETURNS: OK, or ERROR if either <pName> or <symbolId> is NULL.
*
* NOMANUAL 
*/

STATUS symNameGet
    (
    SYMBOL_ID  symbolId,
    char **    pName
    )
    {
    if ((symbolId == NULL) || (pName == NULL))
	return ERROR;

    *pName = symbolId->name;

    return OK;
    }

/*******************************************************************************
*
* symValueGet - get value of a symbol
* 
* This routine is currently intended only for internal use. 
*
* It provides the value of the symbol specified by the SYMBOL_ID
* <symbolId>.  The SYMBOL_ID of a symbol may be obtained by using the
* routine symFindSymbol().  The value of the symbol is copied to 
* the location specified by <pValue>.
*
* RETURNS: OK, or ERROR if either <pValue> or <symbolId> is NULL.
*
* NOMANUAL 
*/

STATUS symValueGet 
    (
    SYMBOL_ID  symbolId,
    void **    pValue
    )
    {
    if ((symbolId == NULL) || (pValue == NULL))
	return ERROR;

    *pValue = symbolId->value;

    return OK;
    }

/*******************************************************************************
*
* symTypeGet - get type of a symbol
* 
* This routine is currently intended only for internal use. 
*
* It provides the type of the symbol specified by the SYMBOL_ID
* <symbolId>.  The SYMBOL_ID of a symbol may be obtained by using the
* routine symFindSymbol().  The type of the symbol is copied to 
* the location specified by <pType>.
*
* RETURNS: OK, or ERROR if either <pType> or <symbolId> is NULL.
*
* NOMANUAL 
*/

STATUS symTypeGet 
    (
    SYMBOL_ID  symbolId,
    SYM_TYPE * pType
    )
    {
    if ((symbolId == NULL) || (pType == NULL))
	return ERROR;

    *pType = symbolId->type;

    return OK;
    }




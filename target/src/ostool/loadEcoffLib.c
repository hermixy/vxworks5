/* loadEcoffLib.c - UNIX extended coff object module loader */

/* Copyright 1984-1996 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01q,30nov98,dbt  no longer clear seg.flags<xxx> after loadSegmentAllocate()
                 call. (SPR #23553).
01p,05oct98,pcn  Initialize all the fields in the SEG_INFO structure.
01o,16sep98,pcn  Set to _ALLOC_ALIGN_SIZE the flags field in seg structure
                 (SPR #21836).
01n,17jul98,pcn  Fixed SPR #21836: alignment mismatch between sections and
                 target.
01m,31oct96,elp  Replaced symAdd() call by symSAdd() call (symtbls synchro).
01l,22feb94,caf  modified rdEcoffSymTab() to pass through the object file's
		 symbol table twice.  this way, undefined symbols can be
		 resolved w/ those appearing later in the table (SPR #3077).
01k,23jan94,jpb  fixed pointer skew problem (SPR #2896).
01j,31jul92,dnw  changed to call CACHE_TEXT_UPDATE.  doc tweak.
01i,29jul92,jmm  fixed setting of address for BSS segment
01h,23jul92,jmm  moved SEG_INFO to loadLib.h
                 memory allocation now done by loadSegmentsAllocate
01g,23jul92,jmm  removed include of loadCommonLib.h 
01f,23jul92,ajm  made boot loader independent
01e,18jul92,smb  Changed errno.h to errnoLib.h.
01d,16jul92,jmm  added module tracking code
                 moved addSegNames to loadLib.c
01c,07jul92,ajm  updated cache interface
01b,23jun92,ajm  reduced ansi warnings for 68k compiler
01a,14jun92,ajm  merged from 5.0.5 loadLib.c v01b
*/

/*
DESCRIPTION
This library provides an object module loading facility particuarly for the
MIPS compiler environment.  Any MIPS SYSV ECOFF
format files may be loaded into memory, relocated properly, their
external references resolved, and their external definitions added to
the system symbol table for use by other modules and from the shell.
Modules may be loaded from any I/O stream.

EXAMPLE
.CS
    fdX = open ("/devX/objFile", O_RDONLY);
    loadModule (fdX, ALL_SYMBOLS);
    close (fdX);
.CE
This code fragment would load the ecoff file "objFile" located on
device "/devX/" into memory which would be allocated from the system
memory pool.  All external and static definitions from the file would be
added to the system symbol table.

This could also have been accomplished from the shell, by typing:
.CS
    -> ld (1) </devX/objFile
.CE

INCLUDE FILE: loadEcoffLib.h

SEE ALSO: loadLib, usrLib, symLib, memLib,
.pG "Basic OS"
*/

/* LINTLIBRARY */

#include "vxWorks.h"
#include "stdio.h"
#include "loadEcoffLib.h"
#include "ecoffSyms.h"
#include "ecoff.h"
#include "ioLib.h"
#include "fioLib.h"
#include "bootLoadLib.h"
#include "loadLib.h"
#include "memLib.h"
#include "pathLib.h"
#include "string.h"
#include "symLib.h"
#include "sysSymTbl.h"
#include "cacheLib.h"
#include "errnoLib.h"
#include "stdlib.h"
#include "symbol.h"	/* for SYM_TYPE typedef */
#include "moduleLib.h"
#include "loadEcoffComm.h"

#define RTEXT   0
#define RDATA   1
#define RBSS    2

#define U_SYM_TYPE 	asym.sc		/* coff classes act as a.out types */
#define U_SYM_VALUE     asym.value

/* coff types and classes as macros */

#define COFF_EXT(symbol)  (((symbol)->asym.st == stProc) || \
			   ((symbol)->asym.st == stGlobal))
#define COFF_ABS(symbol)  (((symbol)->asym.sc == scNil) || \
			   ((symbol)->asym.sc == scAbs))
#define COFF_TEXT(symbol)  ((symbol)->asym.sc == scText)
#define COFF_DATA(symbol) (((symbol)->asym.sc == scData) || \
			   ((symbol)->asym.sc == scRData) || \
			   ((symbol)->asym.sc == scSData))
#define COFF_BSS(symbol)  (((symbol)->asym.sc == scBss) || \
			   ((symbol)->asym.sc == scSBss))
#define COFF_UNDF(symbol) (((symbol)->asym.sc == scUndefined) || \
			   ((symbol)->asym.sc == scSUndefined))
#define COFF_COMM(symbol) (((symbol)->asym.sc == scCommon) || \
			   ((symbol)->asym.sc == scSCommon))


/* misc defines */

#define COMMON_ALIGNMENT	sizeof(double)
#define OVERFLOW_COMPENSATION	0x10000
#define OVERFLOW        	0x00008000
#define LS16BITS        	0x0000ffff
#define MS16BITS        	0xffff0000
#define MS4BITS         	0xf0000000
#define MS6BITS         	0xfc000000
#define LS26BITS        	0x03ffffff

LOCAL char extMemErrMsg [] =
    "loadEcoffLib error: insufficient memory for externals (need %d bytes).\n";
LOCAL char cantConvSymbolType [] =
    "loadEcoffLib error: can't convert '%s' symbol type, error = %#x.\n";
LOCAL char cantAddSymErrMsg [] =
    "loadEcoffLib error: can't add '%s' to system symbol table, error = %#x.\n";
LOCAL char gpRelativeReloc [] =
    "loadEcoffLib error: can't relocate, recompile module with -G 0 flags.\n";

/* forward static functions */

static MODULE_ID ldEcoffMdlAtSym (int fd, int symFlag, char **ppText, 
    char **ppData, char **ppBss, SYMTAB_ID symTbl);

static STATUS rdEcoffSymTab (EXTR * pExtSyms, FAST int nEnts, char ***externals,
    int max_symbols, int symFlag, SEG_INFO *pSeg, char *symStrings,
    SYMTAB_ID symTbl, char *pNextCommon, int group);

static STATUS relSegmentR3k (RELOC *pRelCmds, SCNHDR *pScnHdr,
    char **pExternals, int section, SEG_INFO *pSeg);

static ULONG sizeEcoffCommons (EXTR *pNextSym, ULONG alignBss);

static STATUS ecoffSecHdrRead (int fd, SCNHDR *pScnHdr, FILHDR *pHdr, 
    BOOL swapTables);

static STATUS ecoffReadInSections (int fd, char **pSectPtr, SCNHDR *pScnHdr, 
    BOOL swapTables);

static STATUS ecoffLdSections (char **pSectPtr, SCNHDR *pScnHdr, 
    AOUTHDR *pOptHdr, char *pText, char *pData);

static STATUS ecoffReadRelocInfo (int fd, SCNHDR *pScnHdr, RELOC **pRelsPtr, 
    BOOL swapTables);

static STATUS ecoffReadSymHdr (int fd, HDRR *pSymHdr, FILHDR *pHdr, 
    BOOL swapTables);

static STATUS ecoffReadExtStrings (int fd, char **pStrBuf, HDRR *pSymHdr);

static STATUS ecoffReadExtSyms (int fd, EXTR **pExtSyms, HDRR *pSymHdr, 
    BOOL swapTables);

static int nameToRelocSection (char * pName);

static STATUS ecoffToVxSym (EXTR *symbol, SYM_TYPE *pVxSymType);

/*******************************************************************************
*
* loadEcoffInit - initialize the system for ecoff load modules
*
* This routine initialized VxWorks to use an extended coff format for
* loading modules.
*
* RETURNS:
* OK, or
* ERROR if XXX
*
* SEE ALSO: loadModuleAt()
*/

STATUS loadEcoffInit 
    (
    )
    {
    /* XXX check for installed ? */
    loadRoutine = (FUNCPTR) ldEcoffMdlAtSym;
    return (OK);
    }

/******************************************************************************
*
* ldEcoffMdlAtSym - load object module into memory with specified symbol table
*
* This routine is the underlying routine to loadModuleAtSym().  This interface
* allows specification of the the symbol table used to resolve undefined
* external references and to which to add new symbols.
*
* RETURNS:
* MODULE_ID, or
* NULL if can't read file or not enough memory or illegal file format
*
* NOMANUAL
*/  

LOCAL MODULE_ID ldEcoffMdlAtSym
    (
    FAST int fd,	/* fd from which to read module */
    int symFlag,	/* symbols to be added to table 
			 *   ([NO | GLOBAL | ALL]_SYMBOLS) */
    char **ppText,	/* load text segment at address pointed to by this
			 * pointer, return load address via this pointer */
    char **ppData,	/* load data segment at address pointed to by this
			 * pointer, return load address via this pointer */
    char **ppBss,	/* load bss segment at address pointed to by this
			 * pointer, return load address via this pointer */
    SYMTAB_ID symTbl 	/* symbol table to use */
    )
    {
    char *pText = (ppText == NULL) ? LD_NO_ADDRESS : *ppText;
    char *pData = (ppData == NULL) ? LD_NO_ADDRESS : *ppData;
    char *pBss  = (ppBss  == NULL) ? LD_NO_ADDRESS : *ppBss;
    char **externalsBuf   = NULL;	/* buffer for reading externals */
    EXTR *externalSyms    = NULL;	/* buffer for reading ecoff externals */
    EXTR *pNextSym    = NULL;		/* temp pointer to symbols */
    char *stringsBuf = NULL;		/* string table pointer */
    FILHDR hdr;				/* module's ECOFF header */
    AOUTHDR optHdr;             	/* module's ECOFF optional header */
    SCNHDR scnHdr[MAX_SCNS];		/* module's ECOFF section headers */
    char   *sectPtr[MAX_SCNS];      	/* ecoff sections */
    RELOC  *relsPtr[MAX_SCNS]; 		/* section relocation */
    HDRR symHdr;            		/* symbol table header */
    SEG_INFO seg;			/* file segment info */
    BOOL tablesAreLE;			/* boolean for table sex */
    ULONG alignBss;			/* next boundry for common entry */
    ULONG symbolValue;			/* size of a symbol */
    int status;				/* return value */
    int section;			/* ecoff section number */
    int ix;				/* temp counter */
    int nbytes = 0;			/* temp counter */
    char 	fileName[255];
    UINT16 	group;
    MODULE_ID	moduleId;

    /* Initialization */

    memset ((void *)&seg, 0, sizeof (seg));

    /* Set up the module */

    ioctl (fd, FIOGETNAME, (int) fileName);
    moduleId = loadModuleGet (fileName, MODULE_ECOFF, &symFlag);

    if (moduleId == NULL)
        return (NULL);

    group = moduleId->group;

    /* init section pointers to NULL */

    for (ix = 0; ix < MAX_SCNS; ix++)
	{
	sectPtr[ix] = NULL;
	relsPtr[ix] = NULL;
	bzero ((char *) &scnHdr[ix], (int) sizeof(SCNHDR));
	}

    /* read object module header */

    if (ecoffHdrRead (fd, &hdr, &tablesAreLE) != OK)
	{
	errnoSet(S_loadEcoffLib_HDR_READ);
	goto error;
	}

    /* read in optional header */

    if (ecoffOpthdrRead (fd, &optHdr, &hdr, tablesAreLE) != OK)
	{
	errnoSet(S_loadEcoffLib_OPTHDR_READ);
	goto error;
	}

    seg.addrText = pText;
    seg.addrData = pData;

    /*
     * If pBss is set to LD_NO_ADDRESS, change it to something else.
     * The coff loader allocates one large BSS segment later on, and
     * loadSegmentsAllocate doesn't work correctly for it.
     */

    seg.addrBss = (pBss == LD_NO_ADDRESS ? LD_NO_ADDRESS + 1 : pBss);
 
    seg.sizeText = optHdr.tsize;
    seg.sizeData = optHdr.dsize;
    seg.sizeBss = optHdr.bsize;

    /* 
     * SPR #21836: pSeg->flagsText, pSeg->flagsData, pSeg->flagsBss are used
     * to save the max value of each segments. These max values are computed
     * for each sections. These fields of pSeg are only used on output, then
     * a temporary use is allowed.
     */

    seg.flagsText = _ALLOC_ALIGN_SIZE;
    seg.flagsData = _ALLOC_ALIGN_SIZE;
    seg.flagsBss  = _ALLOC_ALIGN_SIZE;

    /* 
     * SPR #21836: loadSegmentsAllocate() allocate memory aligned on 
     * the max value of sections alignement saved in seg.flagsText,
     * seg.flagsData, seg.flagsBss.
     */

    if (loadSegmentsAllocate (&seg) != OK)
        {
	printErr ("Could not allocate segments\n");
	goto error;
	}
    else
	{
	pText = seg.addrText;
	pData = seg.addrData;
	pBss = LD_NO_ADDRESS;
	}

    /* read in section headers */

    if (ecoffSecHdrRead (fd, &scnHdr[0], &hdr, tablesAreLE) != OK)
	{
	errnoSet(S_loadEcoffLib_SCNHDR_READ);
	goto error;
	}

    /* read in all sections */

    if (ecoffReadInSections (fd, &sectPtr[0], &scnHdr[0], tablesAreLE) != OK)
	{
	errnoSet(S_loadEcoffLib_READ_SECTIONS);
	goto error;
	}

    /* copy sections to text, and data */

    if (ecoffLdSections (&sectPtr[0], &scnHdr[0], &optHdr, pText, pData) != OK)
	{
	errnoSet (S_loadEcoffLib_LOAD_SECTIONS);
	goto error;
	}

    /* 
     * free section memory that is no longer needed (sectPtr).
     * note: free should be as early as possible to allow memory reuse,
     * this is extremenly important in order to load large object modules.
     */

    for (ix = 0; ix < MAX_SCNS; ix++)
	{
	if (sectPtr[ix] != NULL)
	    {
	    free((char *) sectPtr[ix]);
	    sectPtr[ix] = NULL;
	    }
	}

    /* get section relocation info */

    if (ecoffReadRelocInfo (fd, &scnHdr[0], &relsPtr[0], tablesAreLE) != OK)
	{
	errnoSet (S_loadEcoffLib_RELOC_READ);
	goto error;
	}

    /* read in symbolic header */

    if (ecoffReadSymHdr (fd, &symHdr, &hdr, tablesAreLE) != OK)
	{
	errnoSet (S_loadEcoffLib_SYMHDR_READ);
	goto error;
	}

    /* read in local symbols XXX */

    /* read in local string table XXX */

    /* read external string table */

    if (ecoffReadExtStrings (fd, &stringsBuf, &symHdr) != OK)
	{
	errnoSet (S_loadEcoffLib_EXTSTR_READ);
	goto error;
	}

    /* read in external symbols */

    if (ecoffReadExtSyms (fd, &externalSyms, &symHdr, tablesAreLE) != OK)
	{
	errnoSet (S_loadEcoffLib_EXTSYM_READ);
	goto error;
	}

    /* calculate room for external commons in bss */

    alignBss = COMMON_ALIGNMENT;
    nbytes   = 0;

    for(pNextSym = (EXTR *) externalSyms;
        pNextSym < (EXTR *) ((long) externalSyms + 
			    (symHdr.iextMax * cbEXTR)); 
	pNextSym++)
	{
	if (COFF_EXT(pNextSym) && COFF_COMM(pNextSym))
	    {
            symbolValue = sizeEcoffCommons (pNextSym, alignBss);
            nbytes += symbolValue;
            alignBss += symbolValue;
	    }
	}

    /* add common size to bss */

    seg.sizeBss = nbytes + optHdr.bsize;

    if ((pBss == LD_NO_ADDRESS) || (seg.sizeBss > 0))
	{
	if ((pBss = (char *) malloc (seg.sizeBss)) == NULL) 
	    goto error;
        seg.addrBss = pBss;
	seg.flagsBss |= SEG_FREE_MEMORY;
	}

    /* allocate the externalsBuf */

    if (symHdr.iextMax != 0)
        {
        externalsBuf = (char **) malloc ((long) symHdr.iextMax *
					sizeof (char *));
	 if (externalsBuf == NULL)
	     {
	     printErr (extMemErrMsg, symHdr.iextMax * sizeof (char *));
	     goto error;
	     }
	}

    /* add segment names to symbol table before other symbols */

    if (!(symFlag & LOAD_NO_SYMBOLS))
        addSegNames (fd, pText, pData, pBss, symTbl, group);

    /* process symbol table */

    status = rdEcoffSymTab (externalSyms, symHdr.iextMax, &externalsBuf, 
			    symHdr.iextMax, symFlag, &seg, stringsBuf, symTbl, 
			 (char *) ((long) seg.addrBss + (seg.sizeBss - nbytes)),
			    group);

    if (stringsBuf != NULL)
        {
        free (stringsBuf);      /* finished with stringsBuf */
        stringsBuf = NULL;
        }

    if (externalSyms != NULL)
        {
        free (externalSyms);      /* finished with externalSyms */
        externalSyms = NULL;
        }

    /* 
     * relocate text and data segments;
     *   note: even if symbol table had missing symbols, continue with
     *   relocation of those symbols that were found.
     *   note: relocation is for changing the values of the relocated
     *   symbols.  bss is uninitialized data, so it is not relocated
     *   in the symbol table.
     *   caution: the order of relocation sections is not always the same  
     *   as the order of the sections 
     */

    if ((status == OK) || (errnoGet () == S_symLib_SYMBOL_NOT_FOUND))
        {
        for (ix = 0; ix < MAX_SCNS; ix++)
	    {
	    if (relsPtr[ix] != NULL)
		{
		if ((section = nameToRelocSection (scnHdr[ix].s_name)) == ERROR)
		    {
	    	    errnoSet (S_loadEcoffLib_GPREL_REFERENCE);
		    printErr (gpRelativeReloc);
		    goto error;
		    }
                if (relSegmentR3k (relsPtr[ix], &scnHdr[ix], externalsBuf, 
		    section, &seg) != OK)
		    {
	            goto error;
		    }
		}
	    }

        /* flush instruction cache before execution */

	if (optHdr.tsize != NULL)
	    cacheTextUpdate (pText, optHdr.tsize);
	}

    if (externalsBuf != NULL)
        {
        free ((char *) externalsBuf);   /* finished with externalsBuf */
        externalsBuf = NULL;
        }

    /* 
     * free memory that was used (free should be as early as possible to
     * allow memory reuse, which is very important for large object modules) 
     */

    for (ix = 0; ix < MAX_SCNS; ix++)
	if (relsPtr[ix] != NULL)
	    {
	    free((char *) relsPtr[ix]);
	    relsPtr[ix] = NULL;
	    }

    /* return load addresses, where called for */

    if (ppText != NULL)
        *ppText = pText;

    if (ppData != NULL)
        *ppData = pData;

    if (ppBss != NULL)
        *ppBss = pBss;

    /* clear out bss */

    bzero (pBss, (int) seg.sizeBss);

    /*
     * Add the segments to the module.
     * This has to happen after the relocation gets done.
     * If the relocation happens first, the checksums won't be
     * correct.
     */

    moduleSegAdd (moduleId, SEGMENT_TEXT, pText, seg.sizeText, seg.flagsText);
    moduleSegAdd (moduleId, SEGMENT_DATA, pData, seg.sizeData, seg.flagsData);
    moduleSegAdd (moduleId, SEGMENT_BSS, pBss, seg.sizeBss, seg.flagsBss);
		
    if (status == OK)
        return (moduleId);
    else
        return (NULL);

    /* error:
     * clean up dynamically allocated temporary buffers and return ERROR */

error:
    for (ix = 0; ix < MAX_SCNS; ix++)
	{
	if (sectPtr[ix] != NULL)
	    free((char *) sectPtr[ix]);
	if (relsPtr[ix] != NULL)
	    free((char *) relsPtr[ix]);
	}
    if (stringsBuf != NULL)
	free (stringsBuf);
    if (externalsBuf != NULL)
	free ((char *) externalsBuf);
    if (externalSyms != NULL)
	free ((char *) externalSyms);

    moduleDelete (moduleId);
    return (NULL);
    }


/*******************************************************************************
*
* rdEcoffSymTab -
*
* This is passed a pointer to an ecoff symbol table and processes
* each of the external symbols defined therein.  This processing performs
* two functions:
*  1) defined symbols are entered in the system symbol table as
*     specified by the "symFlag" argument:
*	ALL_SYMBOLS	= all defined symbols (LOCAL and GLOBAL) are added,
*	GLOBAL_SYMBOLS	= only external (GLOBAL) symbols are added,
*	NO_SYMBOLS	= no symbols are added;
*  2) any undefined externals are looked up in the system table, and
*     if found, entered in the specified 'externals' array.  This array
*     is indexed by the symbol number (position in the symbol table).
*     Note that only undefined externals are filled in in 'externals' and
*     that values for all other types of symbols are unchanged (i.e. left
*     as garbage!).  This is ok because 'externals' is only used to 
*     perform relocations relative to previously undefined externals.
*     Note that "common" symbols have type undefined external - the value
*     field of the symbol will be non-zero for type common, indicating
*     the size of the object.
*
* If more than the specified maximum number of symbols are found in the
* symbol table, the externals array is extended to accomodate the additional
* symbols.
* If an undefined external cannot be found in the system symbol table,
* an error message is printed, but ERROR is not returned until the entire
* symbol table has been read, which allows all undefined externals to be
* looked up.
* Stabs and absolute symbols are automatically discarded.
*
* RETURNS: OK or ERROR
*
* INTERNAL
* Local symbols are currently ignored.
*/

LOCAL STATUS rdEcoffSymTab 
    (
    EXTR * pExtSyms,		/* pointer to mips external symbols */
    FAST int nEnts,		/* # of entries in symbol table */
    char ***externals,		/* pointer to pointer to array to fill in 
				   values of undef externals */
    int max_symbols,		/* max symbols that can be put in 'externals' */
    int symFlag,		/* symbols to be added to table 
				 *   ([NO|GLOBAL|ALL]_SYMBOLS) */
    SEG_INFO *pSeg,		/* pointer to segment information */
    char *symStrings,		/* symbol string table (only meaningful in
				   BSD 4.2) */
    SYMTAB_ID symTbl,		/* symbol table to use */
    char *pNextCommon, 		/* next common address in bss */
    int group			/* symbol group */
    )
    {
    FAST char *name;		/* symbol name (plus EOS) */
    EXTR symbol;		/* symbol value */
    SYM_TYPE vxSymType;		/* ecoff symbol translation */
    int sym_num = 0;		/* symbol index */
    int status  = OK;		/* return status */
    char *adrs;			/* table resident symbol address */
    SYM_TYPE type;		/* ecoff symbol type */
    char *bias;			/* section relatilve address */
    ULONG bssAlignment;		/* alignment value of common type */

    /* pass 1: process defined symbols */

    for (sym_num = 0; sym_num < nEnts; sym_num ++)
	{
	/* read in next entry - symbol definition and name string */

	symbol = pExtSyms [sym_num];

	/* add requested symbols to symbol table */

        if ((!COFF_ABS(&symbol)) &&	/* throw away absolute symbols */
	    (!COFF_UNDF(&symbol)) && (!COFF_COMM(&symbol)) &&
	    (symFlag & LOAD_GLOBAL_SYMBOLS) && COFF_EXT(&symbol))
	    {
	    /*
	     * symbol name string is in symStrings array indexed 
	     * by symbol.iss
	     */

	    name = symStrings + symbol.asym.iss;

	    /*
	     *  I'm not sure the symbol type can tell us what section the 
	     *  symbol is in so we look at the address.  This should be 
	     *  investigated at a later date.
	     */

	    if (symbol.U_SYM_VALUE < (long) pSeg->sizeText)
		bias = (char *) pSeg->addrText;
	    else if (symbol.U_SYM_VALUE
		< ((long) pSeg->sizeText + (long) pSeg->sizeData))
		bias = (char *) (
		       (long) pSeg->addrData
		     - (long) pSeg->sizeText);
	    else
		bias = (char *) ((long) pSeg->addrBss - pSeg->sizeText - 
		    pSeg->sizeData);

	    if (ecoffToVxSym (&symbol, &vxSymType) == ERROR)
		printErr (cantConvSymbolType, name, errnoGet());

	    if (symSAdd (symTbl, name, symbol.U_SYM_VALUE + bias, 
		vxSymType, group) != OK)
		printErr (cantAddSymErrMsg, name, errnoGet());

	    /*
	     * add symbol address to externals table, but first
	     * check for enough room in 'externals' table
	     */
	    
	    if (sym_num == max_symbols)
		{
		/*
		 * no more room in symbol table -
		 * make externals table half again as big
		 */
		
		max_symbols += max_symbols / 2;
		
		*externals = (char **) realloc ((char *) *externals,
		    max_symbols * sizeof (char *));

		if (*externals == NULL)
		    return (ERROR);
		}

	    /* enter address in table used by relSegment */

	    (*externals) [sym_num] = symbol.U_SYM_VALUE + bias;
	    }
	}

    /* pass 2: process undefined symbols (including commons) */

    for (sym_num = 0; sym_num < nEnts; sym_num ++)
	{
	/* read in next entry - symbol definition and name string */

	symbol = pExtSyms [sym_num];

        if ((!(COFF_ABS(&symbol))) && 	/* throw away absolute symbols */
	    ((COFF_UNDF(&symbol)) || (COFF_COMM(&symbol))))
	    {
	    /*
	     * symbol name string is in symStrings array indexed 
	     * by symbol.iss
             */

	    name = symStrings + symbol.asym.iss;

	    /*
	     * undefined external symbol or "common" symbol -
	     *   common symbol type is denoted by undefined external with
	     *   the value set to non-zero.
	     *
	     * if symbol is a common, then allocate space and add to
	     * symbol table as BSS
	     */

	    /* common symbol - create new symbol */

	    if (COFF_COMM(&symbol)
		&& (symbol.U_SYM_VALUE != 0)) 	/* do we really need this ? */
		{
                /* 
		 *  common symbol - create new symbol 
		 *
		 *  Common symbols are now tacked to the end of the bss
		 *  section.  This portion of code reads the symbol value
		 *  which contains the symbol size and places it appropriately
		 *  in the bss section.  The function sizeEcoffCommons is used
		 *  with the address of the last common added to determine
		 *  proper alignment.
		 */

		adrs = pNextCommon;
		bssAlignment = sizeEcoffCommons (&symbol, (ULONG) adrs);
		bssAlignment -= (ULONG) symbol.U_SYM_VALUE;
		adrs += bssAlignment;
		pNextCommon += (symbol.U_SYM_VALUE + bssAlignment);

		if (symSAdd (symTbl, name, adrs, (N_BSS | N_EXT), group) != OK)
		    printErr (cantAddSymErrMsg, name, errnoGet());
		}


	    /* look up undefined external symbol in symbol table */

	    else if (symFindByNameAndType (symTbl, name, &adrs, &type,
					   N_EXT, N_EXT) != OK)
		{
		/* symbol not found in symbol table */

		printErr ("undefined symbol: %s\n", name);
		adrs = NULL;
		status = ERROR;
		}

	    /*
	     * add symbol address to externals table, but first
	     * check for enough room in 'externals' table
	     */

	    if (sym_num == max_symbols)
		{
		/*
		 * no more room in symbol table -
		 * make externals table half again as big
		 */

		max_symbols += max_symbols / 2;

		*externals = (char **) realloc ((char *) *externals, 
		    max_symbols * sizeof (char *));

		if (*externals == NULL)
		    return (ERROR);
		}

	    (*externals) [sym_num] = adrs;	/* enter address in table */
	    }
	}

    return (status);
    }

/*******************************************************************************
*
* relSegmentR3k - perform relocation commands for a MIPS segment
*
* This routine reads the specified relocation command segment and performs
* all the relocations specified therein.
* External relocations are looked up in the 'externals' table.
* All other relocations are relative to how much a particular segment has moved.
*
* That was the short explanation.  The following is more specific:
* Each relocation command is identified as being external or local; 
* external symbols require the use of the 'externals' table to perform
* the relocation while local symbols are relocated relative to a segment
* (text or data).
*
* There are the following types of external relocation entries:
*	-R_REFWORD for externally defined pointers,
* 	-R_HALFWORD ?? would be a 16 bit external pointer but thus far
* 	 	has not been seen from the MIPS C compiler,
*       -R_JMPADDR for calls to externalC functions,
* 	-R_REFHI/R_REFLO pairs for accessing and external data
* 		value, and
*	-unsupported GP (global pointer) relative relocation types which 
* 		cause an error to be returned.
*		
* There are local versions of these relocation entries also:
*	-R_REFWORD for static pointers and arrays,
*	-R_HALFWORD should be similar to R_REFWORD but have not been 
*		able to get the C compiler to generate it,
*	-R_JMPADDR for relative jumps to functions (references to functions
* 		located within the load module.
* 	-R_REFHI/R_REFLO combinations for accesses to objects  defined in 
*     		this file and with local visibility (i.e. static data 
* 		structure references).
*	-unsupported GP (global pointer) relative relocation types which 
* 		cause an error to be returned.
*
*
* RETURNS: OK or ERROR
*/

LOCAL STATUS relSegmentR3k 
    (
    RELOC *pRelCmds,		/* list of mips relocation commands */
    SCNHDR *pScnHdr,		/* section header for relocation */
    char **pExternals,		/* addresses of external symbols */
    int section,		/* section number TEXT, RDATA, BSS ... */
    SEG_INFO *pSeg 		/* section addresses and sizes */
    )
    {
    FAST long *pAddress;	/* relocation address */
    FAST long *pNextAddress;	/* next relocation address */
    long refhalfSum;		/* refhalf sum */
    long targetAddr;		/* relocation target address */
    long relocOffset; 		/* relocation offset */
    long relocConstant;		/* general reloc constant */
    long relSegment;		/* RTEXT, RDATA, ... */
    long segment;		/* segment we are relocating */
    unsigned short nCmds;	/* # reloc commands for seg */
    RELOC relocCmd;		/* relocation structure */
    RELOC nextRelocCmd;		/* next relocation structure */
    long refhiConstant = 0;	/* last REFHI constant */

    nCmds = pScnHdr->s_nreloc;

    while ( nCmds > 0 )
	{
	/* read relocation command */

	relocCmd = *pRelCmds++;
	nCmds -= 1;

	/* 
	 * Calculate actual address in memory that needs relocation
	 * and perform external or segment relative relocation 
	 */

        if ((section == R_SN_TEXT) || (section == R_SN_INIT))
	    {
	    pAddress = (long *) ((long) pSeg->addrText + relocCmd.r_vaddr);
	    segment = RTEXT;
	    }
	else
	    {
	    pAddress = (long *) ((long) pSeg->addrData + 
		(relocCmd.r_vaddr - pSeg->sizeText));
	    segment = RDATA;
	    }

	/* note: should check for valid pAddress here XXXX */

	if (relocCmd.r_extern)	/* globlal relocation */
	    {
	    switch (relocCmd.r_type)
	        {

	        case R_REFWORD:

		/*
		*  This relocation is performed by adding the 32 bit address
		*  of the symbol to the contents of pAddress.
		*  See IDT Assembly language programmers guide pg. 10-16
		*/

 		    *pAddress = (*pAddress + 
			(long) pExternals[relocCmd.r_symndx]);
		    break;

	        case R_REFHALF:	/* WARNING: UNTESTED */

		/* 
		*  This case is not well documented by MIPS, and has never
		*  been generated.  REFWORD and REFHALF relocation entries
		*  are generally used for pointers, and a case where you have
		*  a 16 bit pointer is very unlikely to occur on a 32 bit
		*  architecture.
		*/
		    refhalfSum = ((LS16BITS & *pAddress) + 
			(long) pExternals[relocCmd.r_symndx]);
		    if (refhalfSum >= OVERFLOW)
		        refhalfSum &= LS16BITS;
		    *pAddress = (MS16BITS & *pAddress) | 
			(LS16BITS & refhalfSum);
		    break;

	        case R_JMPADDR:

		/* 
		*  For this relocation type the loader determines the address
		*  for the jump, shifts the address right two bits, and adds 
		*  the lower 26 bits of the result to the low 26 bits of the 
		*  instruction at pAddress (after sign extension).  The
		*  results go back into the low 26 bits at pAddress.
		*  The initial check is to see if the jump is within range.  
		*  The current address is incremented by 4 because the jump
		*  actually takes place in the branch delay slot of the
		*  current address.
		*  See IDT Assembly language programmers guide pg. 10-16
		*/
		    if ((MS4BITS & ((long) pAddress + sizeof(INSTR))) != 
			(MS4BITS & (long) pExternals[relocCmd.r_symndx]))
			{
	    		errnoSet (S_loadEcoffLib_JMPADDR_ERROR);
		        return (ERROR);
			}
		    targetAddr = (LS26BITS & *pAddress) + (LS26BITS &
			((unsigned) pExternals[relocCmd.r_symndx] >> 2));
		    *pAddress = (MS6BITS & *pAddress) | (LS26BITS & targetAddr);
		    break;

	        case R_REFHI:

		/*
		*  A refhi relocation is done by reading the next relocation
		*  entry (always the coresponding reflo entry).  This least
		*  significant 16 bits are taken from the refhi instruction 
		*  and shifted left to form a 32 bit value.  This value is 
		*  added to the least significant 16 bits of the reflo 
		*  instruction (taking into account sign extention) to form 
		*  a 32 bit reference pointer.  The target address is added 
		*  to this constant and placed back in the least significant 
		*  16 bits of each instruction address.  The contents of the 
		*  lower 16 bits of pAddress (refhiConstant) are saved for any 
		*  other reflo entries that the refhi entry may correspond to.
		*  See IDT Assembly language programmers guide pg. 10-16
		*/
		    refhiConstant = (*pAddress << 16);
		    nextRelocCmd = *pRelCmds++;
	   	    nCmds -= 1;
	    	    if (nextRelocCmd.r_type != R_REFLO) 
			{
	    		errnoSet (S_loadEcoffLib_NO_REFLO_PAIR);
		        return (ERROR);
			}
        	    if (segment == RTEXT)
			{
	    	        pNextAddress = (long *) ((long) pSeg->addrText + 
			    nextRelocCmd.r_vaddr);
			}
		    else /* segment == RDATA */
			{
	    		pNextAddress = (long *) ((long) pSeg->addrData + 
			    (nextRelocCmd.r_vaddr - 
			    (long) pSeg->sizeText));
			}
                    /* following cast to short guarantees sign extension */
		    relocConstant = (*pAddress << 16) + 
			(int) ((short)(LS16BITS & *pNextAddress));
		    targetAddr = (long) pExternals[relocCmd.r_symndx] + 
			relocConstant;
		    if ((LS16BITS & targetAddr) >= OVERFLOW)
			targetAddr += OVERFLOW_COMPENSATION;
		    *pAddress = (MS16BITS & *pAddress) + 
			((unsigned) targetAddr >> 16);
		    *pNextAddress = (MS16BITS & *pNextAddress) + 
			(LS16BITS & targetAddr);
	            break;

	        case R_REFLO:

    		/*
    		*  Since a REFHI must occur with a REFLO pair (lui, addi 
		*  instructions) it was originally thought that a REFLO 
		*  entry would never be seen alone.  This turned out to be 
		*  an incorrect assumption.  If a R_REFLO is by itself, it 
		*  is assumed to belong to the last R_REFHI entry.
    		*/

                    /* following cast to short guarantees sign extension */
		    relocConstant = refhiConstant + 
                        (int) ((short)(LS16BITS & *pAddress));
		    targetAddr = (long) pExternals[relocCmd.r_symndx] + 
			relocConstant;
		    if ((LS16BITS & targetAddr) >= OVERFLOW)
			targetAddr += OVERFLOW_COMPENSATION;
		    *pAddress = (MS16BITS & *pAddress) + 
			(LS16BITS & targetAddr);
		    break;

	        case R_GPREL:
	        case R_LITERAL:

		    printErr (gpRelativeReloc);
	    	    errnoSet (S_loadEcoffLib_GPREL_REFERENCE);
		    return (ERROR); 	/* gp reg addressing not supported */
		    break;

	        case R_ABS:

		    break;		/* relocation already performed */

	        default:

		    printErr("Unrecognized ext relocation type %d\n",
		        relocCmd.r_type);
	    	    errnoSet (S_loadEcoffLib_UNRECOGNIZED_RELOCENTRY);
		    return (ERROR);
		    break;
	        }
	    }
	else	/* local relocation */
	    {
	    switch (relocCmd.r_symndx)
		{

		case R_SN_INIT:
		case R_SN_TEXT:

		    relSegment = RTEXT;
	    	    break;

		case R_SN_DATA:
		case R_SN_RDATA:	
		case R_SN_SDATA:	/* treat as data with no GP */

		    relSegment = RDATA;
	            break;

		case R_SN_BSS:
		case R_SN_SBSS:		/* treat as bss with no GP  */

		    relSegment = RBSS;
		    break;

		case R_SN_LIT4:	
		case R_SN_LIT8:	

		    printErr (gpRelativeReloc);
	    	    errnoSet (S_loadEcoffLib_GPREL_REFERENCE);
		    return (ERROR);
		    break;

		default:

		    printErr ("Unknown symbol number %d\n", relocCmd.r_symndx);
	    	    errnoSet (S_loadEcoffLib_UNRECOGNIZED_RELOCENTRY);
		    return (ERROR); 	
		    break;
		} 
	    switch (relocCmd.r_type)
		{

		case R_REFWORD:		

		/*
		*  This relocation is performed by adding the 32 bit address
		*  of the symbol to the contents of pAddress.  The address of
		*  the symbol is found by adding the relative segment address
		*  to the contents of the address we are relocating.
		*  See IDT Assembly language programmers guide pg. 10-16
		*/

		    if (relSegment == RTEXT)
			{
			relocOffset = *pAddress;
 			*pAddress = ((long) pSeg->addrText + relocOffset);
			}
		    else if (relSegment == RDATA)
			{
			relocOffset = *pAddress - pSeg->sizeText;
 			*pAddress = ((long) pSeg->addrData + relocOffset);
			}
		    else /* relSegment == RBSS */
			{
			relocOffset = *pAddress - pSeg->sizeText - 
			    pSeg->sizeData;
 			*pAddress = ((long) pSeg->addrBss + relocOffset);
			}
		    break;

		case R_REFHALF: 	/* WARNING: UNTESTED */

		/* 
		*  This case is not well documented by MIPS, and has never
		*  been generated.  REFWORD and REFHALF relocation entries
		*  are generally used for pointers, and a case where you have
		*  a 16 bit pointer is very unlikely to occur on a 32 bit
		*  architecture.
		*/
		    if (relSegment == RTEXT)
			{
			relocOffset = *pAddress;
 			*pAddress = ((long) pSeg->addrText + relocOffset);
			}
		    else if (relSegment == RDATA)
			{
			relocOffset = *pAddress - pSeg->sizeText;
 			*pAddress = ((long) pSeg->addrData + relocOffset);
			}
		    else /* relSegment == RBSS */
			{
			relocOffset = *pAddress - pSeg->sizeText - 
			    pSeg->sizeData;
 			*pAddress = ((long) pSeg->addrBss + relocOffset);
			}
		    if (LS16BITS & *pAddress >= OVERFLOW)
		        {
		        errnoSet (S_loadEcoffLib_REFHALF_OVFL);
		        return (ERROR);
		        }
		    break;

		case R_JMPADDR:

		/* 
		*  For this relocation type the loader determines the address
		*  for the jump, shifts the address right two bits, and adds 
		*  the lower 26 bits of the result to the low 26 bits of the 
		*  instruction at pAddress (after sign extension).  The
		*  results go back into the low 26 bits at pAddress.
		*  The initial check is to see if the jump is within range.  
		*  The current address is incremented by 4 because the jump
		*  actually takes place in the branch delay slot of the
		*  current address.  The address for the jump is dependant 
		*  on the contents of the address we are relocating and
		*  the address of the relative section.
		*  See IDT Assembly language programmers guide pg. 10-16
		*/

		    if (relSegment == RTEXT)
			{
		        targetAddr = (((LS26BITS & *pAddress) << 2)
			    + (MS4BITS & (long) pAddress )
		            + (long) pSeg->addrText);
			}
		    else if (relSegment == RDATA)
			{
			targetAddr = (((LS26BITS & *pAddress) << 2)
		            + (MS4BITS & (long) pAddress )
			    + (long) pSeg->addrData);
			}
		    else /* relSegment == RBSS */
			{
			targetAddr = (((LS26BITS & *pAddress) << 2)
		            + (MS4BITS & (long) pAddress )
			    + (long) pSeg->addrBss);
			}

                    if (MS4BITS & targetAddr !=
		        (MS4BITS & ((long) pAddress + sizeof(INSTR))))
		        {
		        errnoSet (S_loadEcoffLib_JMPADDR_ERROR);
		        return (ERROR);
		        }

		    targetAddr = (unsigned) targetAddr >> 2;
                    /* checking for overflows here is not valid */
		    targetAddr &= LS26BITS;
		    *pAddress =  (MS6BITS & *pAddress) | targetAddr;
		    break;

		case R_REFHI:

		/*
		*  A refhi relocation is done by reading the next relocation
		*  entry (always the coresponding reflo entry).  This least
		*  significant 16 bits are taken from the refhi instruction 
		*  and shifted left to form a 32 bit value.  This value is 
		*  added to the least significant 16 bits of the reflo 
		*  instruction (taking into account sign extention) to form 
		*  a 32 bit reference pointer.  The target address is added 
		*  to this constant and placed back in the least significant 
		*  16 bits of each instruction address.  The contents of the 
		*  lower 16 bits of pAddress (refhiConstant) are saved for any 
		*  other reflo entries that the refhi entry may correspond to.
		*  See IDT Assembly language programmers guide pg. 10-16
		*/

		    refhiConstant = (*pAddress << 16);
		    nextRelocCmd = *pRelCmds++;
		    nCmds -= 1;

	    	    if (nextRelocCmd.r_type != R_REFLO) 
			{
	    	        errnoSet (S_loadEcoffLib_NO_REFLO_PAIR);
		        return (ERROR);
			}

        	    if (segment == RTEXT)
			{
	    	        pNextAddress = (long *) ((long) pSeg->addrText + 
			    nextRelocCmd.r_vaddr);
			}
		    else /* segment == RDATA */
			{
	    		pNextAddress = (long *) ((long) pSeg->addrData + 
			    (nextRelocCmd.r_vaddr - 
			    (long) pSeg->sizeText));
			}

		    /* following cast to short guarantees sign extension */
		    relocConstant = ((LS16BITS & *pAddress) << 16)
		        + (int) ((short) (LS16BITS & *pNextAddress));

		    if (relSegment == RTEXT)
			{
			targetAddr = relocConstant + (long) pSeg->addrText;
			}
		    else if (relSegment == RDATA)
			{
			relocConstant = relocConstant - pSeg->sizeText;
		        targetAddr = relocConstant + (long) pSeg->addrData;
			}
		    else /* relSegment == RBSS */
			{
			relocConstant = relocConstant - pSeg->sizeText - 
			    pSeg->sizeData;
		        targetAddr = relocConstant + (long) pSeg->addrBss;
			}

                    if ((LS16BITS & targetAddr) >= OVERFLOW)
		        targetAddr += OVERFLOW_COMPENSATION;

		    *pAddress = ((MS16BITS & *pAddress) |
		 	((unsigned) (MS16BITS & targetAddr) >> 16));
		    *pNextAddress = (MS16BITS & *pNextAddress) | 
			(LS16BITS & targetAddr);
	            break;

		case R_REFLO:

		/*
		*  Since a REFHI must occur with a REFLO pair (lui, addi 
		*  instructions) it was originally thought that a REFLO 
		*  entry would never be seen alone.  This turned out to 
		*  be an incorrect assumption.  If a R_REFLO is by itself, 
		*  it is assumed to belong to the last R_REFHI entry.
		*/

                    /* following cast to short guarantees sign extension */
		    relocConstant = (LS16BITS & refhiConstant) 
			+ (int) ((short) (LS16BITS & *pAddress));
		    if (relSegment == RTEXT)
			{
			targetAddr = relocConstant + 
			    (long) pSeg->addrText;
			}
		    else if (relSegment == RDATA)
			{
			relocConstant = relocConstant - pSeg->sizeText;
		        targetAddr = relocConstant + (long) pSeg->addrData;
			}
		    else /* relSegment == RBSS */
			{
			relocConstant = relocConstant - pSeg->sizeText - 
			    pSeg->sizeData;
		        targetAddr = relocConstant + (long) pSeg->addrBss;
			}
		    if ((LS16BITS & targetAddr) >= OVERFLOW)
			targetAddr += OVERFLOW_COMPENSATION;
		    *pAddress = ((MS16BITS & *pAddress) | 
			(LS16BITS & targetAddr));
		    break;

		case R_GPREL:
		case R_LITERAL:

		    printErr (gpRelativeReloc);
	    	    errnoSet (S_loadEcoffLib_GPREL_REFERENCE);
		    return (ERROR);  /* gp relative addressing not supported */
		    break;

		case R_ABS:

		    break;	/* relocation already completed */

		default:

		    printErr ("Unknown relocation type %d\n", relocCmd.r_type);
	    	    errnoSet (S_loadEcoffLib_UNRECOGNIZED_RELOCENTRY);
		    return (ERROR); /* unrecognized relocation type */
		    break;
		}
	    }
	}
    return (OK);
    }

/*******************************************************************************
*
* softSeek - seek forward into a file
*
* This procedure seeks forward into a file without actually using seek
* calls.  It is usefull since seek does not work on all devices.
*
* RETURNS:
* OK, or
* ERROR if forward seek could not be accomplished
*
*/

LOCAL STATUS softSeek 
    (
    int fd,			/* fd to seek with */
    int startOfFileOffset 	/* byte index to seek to */
    )
    {
    int position;		/* present file position */
    int bytesToRead;		/* bytes needed to read */
    char tempBuffer[BUFSIZE];	/* temp buffer for bytes */

    position = ioctl (fd, FIOWHERE, (int) 0);

    if (position > startOfFileOffset)
	return(ERROR);

    /* calculate bytes to read */

    bytesToRead = startOfFileOffset - position;

    while (bytesToRead >= BUFSIZE)
	{
        if (fioRead (fd, tempBuffer, BUFSIZE) != BUFSIZE)
	    return (ERROR);
	bytesToRead -= BUFSIZE;
	}

    if (bytesToRead > 0)
	{
        if (fioRead (fd, tempBuffer, bytesToRead) != bytesToRead)
	    return (ERROR);
	}

    return(OK);
    }

/*******************************************************************************
*
* ecoffToVxSym - Translate an ecoff symbol type to a VxWorks symbol type
*
* This procedure translates an ecoff symbol type to a symbol type that the 
* VxWorks symbol table can understand.
*
* RETURNS:
* OK, or
* ERROR if the symbol could not be tranlated.
*
*/

LOCAL STATUS ecoffToVxSym
    (
    EXTR *symbol,		/* ecoff symbol to convert */
    SYM_TYPE *pVxSymType 	/* where to place conversion */
    )
    {
    switch(symbol->U_SYM_TYPE)
	{
	case scText:
	    *pVxSymType = (SYM_TYPE) N_TEXT;
	    break;
	case scData:
	case scRData:
        case scSData:
	    *pVxSymType = (SYM_TYPE) N_DATA;
	    break;
	case scBss:
        case scSBss:
	    *pVxSymType = (SYM_TYPE) N_BSS;
	    break;
	case scCommon:
	case scSCommon:
	    *pVxSymType = (SYM_TYPE) N_COMM;
	    break;
        case scUndefined:
	case scSUndefined:
        case scNil:
	case scAbs:
	    errnoSet (S_loadEcoffLib_UNEXPECTED_SYM_CLASS);
	    return (ERROR);
	    break;
	default:
	    errnoSet (S_loadEcoffLib_UNRECOGNIZED_SYM_CLASS);
	    return (ERROR);
	    break;
	}

    /* convert to global symbol ? */

    if (COFF_EXT(symbol))
	*pVxSymType |= (SYM_TYPE) N_EXT;

    return (OK);
    }

/*******************************************************************************
*
* nameToRelocSection - Convert COFF section name to integer representation
*
* This procedure converts a COFF section name to it's appropriate integer
* designator.  
*
* RETURNS:
* Integer value of section name
* or ERROR if section relocation is not supported.
*
*/

LOCAL int nameToRelocSection
    (
    char * pName         /* section name */
    )
    {
    if (strcmp (pName,_TEXT) == 0)
        return (R_SN_TEXT);
    else if (strcmp (pName,_RDATA) == 0)
        return (R_SN_RDATA);
    else if (strcmp (pName,_DATA) == 0)
        return (R_SN_DATA);
    else if (strcmp (pName,_SDATA) == 0)
        return (ERROR);			/* R_SN_SDATA: no gp rel sections */
    else if (strcmp (pName,_SBSS) == 0)
        return (ERROR);			/* R_SN_SBSS: no gp rel sections */
    else if (strcmp (pName,_BSS) == 0)
        return (ERROR);			/* R_SN_BSS: bss not relocatable */
    else if (strcmp (pName,_INIT) == 0)
	return (R_SN_INIT);
    else if (strcmp (pName,_LIT8) == 0)
        return (ERROR);			/* R_SN_LIT8: no gp rel sections */
    else if (strcmp (pName,_LIT4) == 0)
        return (ERROR);			/* R_SN_LIT8: no gp rel sections */
    else
        return (ERROR);     		/* return ERROR */
    }

/*******************************************************************************
*
* swapCoffscnHdr - swap endianess of COFF section header
* 
*/

LOCAL void swapCoffscnHdr
    (
    SCNHDR *pScnHdr 		/* module's ECOFF section header */
    )
    {
    SCNHDR tempScnHdr;
    bcopy ((char *) &pScnHdr->s_name[0], (char *) &tempScnHdr.s_name[0], 8);
    swabLong ((char *) &pScnHdr->s_paddr, (char *) &tempScnHdr.s_paddr);
    swabLong ((char *) &pScnHdr->s_vaddr, (char *) &tempScnHdr.s_vaddr);
    swabLong ((char *) &pScnHdr->s_size, (char *) &tempScnHdr.s_size);
    swabLong ((char *) &pScnHdr->s_scnptr, (char *) &tempScnHdr.s_scnptr);
    swabLong ((char *) &pScnHdr->s_relptr, (char *) &tempScnHdr.s_relptr);
    swabLong ((char *) &pScnHdr->s_lnnoptr, (char *) &tempScnHdr.s_lnnoptr);
    swab ((char *) &pScnHdr->s_nreloc, (char *) &tempScnHdr.s_nreloc, 
	sizeof(pScnHdr->s_nreloc));
    swab ((char *) &pScnHdr->s_nlnno, (char *) &tempScnHdr.s_nlnno, 
	sizeof(pScnHdr->s_nlnno));
    swabLong ((char *) &pScnHdr->s_flags, (char *) &tempScnHdr.s_flags);
    bcopy ((char *) &tempScnHdr, (char *) pScnHdr, sizeof(SCNHDR));
    }

/*******************************************************************************
*
* swapCoffsymHdr - swap endianess of COFF symbolic header
* 
*/

LOCAL void swapCoffsymHdr
    (
    HDRR *pSymHdr 		/* module's ECOFF symbolic header */
    )
    {
    HDRR tempSymHdr;
    swab ((char *) &pSymHdr->magic, (char *) &tempSymHdr.magic, 
	sizeof(pSymHdr->magic));
    swab ((char *) &pSymHdr->vstamp, (char *) &tempSymHdr.vstamp, 
	sizeof(pSymHdr->vstamp));
    swabLong ((char *) &pSymHdr->ilineMax, (char *) &tempSymHdr.ilineMax);
    swabLong ((char *) &pSymHdr->cbLine, (char *) &tempSymHdr.cbLine);
    swabLong ((char *) &pSymHdr->cbLineOffset, 
	(char *) &tempSymHdr.cbLineOffset);
    swabLong ((char *) &pSymHdr->idnMax, (char *) &tempSymHdr.idnMax);
    swabLong ((char *) &pSymHdr->cbDnOffset, (char *) &tempSymHdr.cbDnOffset);
    swabLong ((char *) &pSymHdr->ipdMax, (char *) &tempSymHdr.ipdMax);
    swabLong ((char *) &pSymHdr->cbPdOffset, (char *) &tempSymHdr.cbPdOffset);
    swabLong ((char *) &pSymHdr->isymMax, (char *) &tempSymHdr.isymMax);
    swabLong ((char *) &pSymHdr->cbSymOffset, (char *) &tempSymHdr.cbSymOffset);
    swabLong ((char *) &pSymHdr->ioptMax, (char *) &tempSymHdr.ioptMax);
    swabLong ((char *) &pSymHdr->cbOptOffset, (char *) &tempSymHdr.cbOptOffset);
    swabLong ((char *) &pSymHdr->iauxMax, (char *) &tempSymHdr.iauxMax);
    swabLong ((char *) &pSymHdr->cbAuxOffset, (char *) &tempSymHdr.cbAuxOffset);
    swabLong ((char *) &pSymHdr->issMax, (char *) &tempSymHdr.issMax);
    swabLong ((char *) &pSymHdr->cbSsOffset, (char *) &tempSymHdr.cbSsOffset);
    swabLong ((char *) &pSymHdr->issExtMax, (char *) &tempSymHdr.issExtMax);
    swabLong ((char *) &pSymHdr->cbSsExtOffset, 
	(char *) &tempSymHdr.cbSsExtOffset);
    swabLong ((char *) &pSymHdr->ifdMax, (char *) &tempSymHdr.ifdMax);
    swabLong ((char *) &pSymHdr->cbFdOffset, (char *) &tempSymHdr.cbFdOffset);
    swabLong ((char *) &pSymHdr->crfd, (char *) &tempSymHdr.crfd);
    swabLong ((char *) &pSymHdr->cbRfdOffset, (char *) &tempSymHdr.cbRfdOffset);
    swabLong ((char *) &pSymHdr->iextMax, (char *) &tempSymHdr.iextMax);
    swabLong ((char *) &pSymHdr->cbExtOffset, (char *) &tempSymHdr.cbExtOffset);
    bcopy ((char *) &tempSymHdr, (char *) pSymHdr, sizeof(HDRR));
    }

/*******************************************************************************
*
* swapCoffsymbols - swap endianess of COFF symbols
* 
*/

LOCAL void swapCoffsymbols
    (
    EXTR *pSym,		/* module's ECOFF symbol table */
    int symCount 	/* number of symbols in table */
    )
    {
    EXTR tempExtSym;
    SYMR_LE tempSym;
    EXTR_LE tempRsvd;
    for(;symCount > 0; pSym++, symCount--)
	{
	/* swap short bitfield */
	swab ((char *)pSym, (char *) &tempRsvd, sizeof(tempRsvd));
	tempExtSym.jmptbl = tempRsvd.jmptbl;
	tempExtSym.cobol_main = tempRsvd.cobol_main;
	tempExtSym.reserved = tempRsvd.reserved;

	swab ((char *) &pSym->ifd, (char *) &tempExtSym.ifd, sizeof(pSym->ifd));
	swabLong ((char *) &pSym->asym.iss, (char *) &tempExtSym.asym.iss);
	swabLong ((char *) &pSym->asym.value, (char *) &tempExtSym.asym.value);

/* 
*  The correct way to do the following swap would be to define a 
*  union so the address of the bitfield could be taken.  Doing this
*  would ripple changes through all of vxWorks so we are avioding
*  it at the moment.  If any fields get added to the structure
*  SYMR after value and before the bitfield we will surely notice
*  it here.
*/

	/* swap long bitfield */
	swabLong ((char *) ((long) &pSym->asym.value + 
	    sizeof(pSym->asym.value)),
	    (char *) ((long) &tempSym.value + 
	    sizeof(tempSym.value)));
	tempExtSym.asym.st = tempSym.st;
	tempExtSym.asym.sc = tempSym.sc;
	tempExtSym.asym.reserved = tempSym.reserved;
	tempExtSym.asym.index = tempSym.index;

        bcopy ((char *) &tempExtSym, (char *) pSym, sizeof(EXTR));
	}
    }

/*******************************************************************************
*
* swapCoffrelocTbl - swap endianess of COFF relocation entries
* 
*/

LOCAL void swapCoffrelocTbl
    (
    RELOC *pReloc,		/* module's ECOFF relocation table */
    int relocCount 		/* number of relocation entries in table */
    )
    {
    RELOC tempReloc;
    RELOC_LE tempBits;
    for (;relocCount > 0; pReloc++, relocCount--)
	{
	swabLong ((char *) &pReloc->r_vaddr, (char *) &tempReloc.r_vaddr);

/* 
*  The correct way to do the following swap would be to define a 
*  union so the address of the bitfield could be taken.  Doing this
*  would ripple changes through all of vxWorks so we are avioding
*  it at the moment.  If any fields get added to the structure
*  RELOC after r_vaddr and before the bitfield we will surely notice
*  it here.
*/

	/* swap long bitfield */
	swabLong ((char *) ((long) &pReloc->r_vaddr +
	    sizeof(pReloc->r_vaddr)),
	    (char *) ((long) &tempBits +
	    sizeof(tempBits.r_vaddr)));
	tempReloc.r_symndx = tempBits.r_symndx;
	tempReloc.r_reserved = tempBits.r_reserved;
	tempReloc.r_type = tempBits.r_type;
	tempReloc.r_extern = tempBits.r_extern;

        bcopy ((char *) &tempReloc, (char *) pReloc, 
	    sizeof(RELOC));
	}
    }

/*******************************************************************************
*
* ecoffSecHdrRead - read in COFF section header and swap if necessary
* 
*/

LOCAL STATUS ecoffSecHdrRead
    (
    int    fd,
    SCNHDR *pScnHdr,
    FILHDR *pHdr,
    BOOL   swapTables 
    )
    {
    int ix;

    /* check for correct section count */

    if (pHdr->f_nscns < NO_SCNS || pHdr->f_nscns > MAX_SCNS)
	{
	return (ERROR);
	}

    for (ix = 0; ix < pHdr->f_nscns; ix++)
	{
        if (fioRead (fd, (char *)pScnHdr, sizeof(SCNHDR)) != sizeof(SCNHDR))
	    {
	    return (ERROR);
	    }
        if (swapTables)
	    {
	    swapCoffscnHdr (pScnHdr);
	    }

	pScnHdr++;
	}

    return (OK);
    }

/*******************************************************************************
*
* ecoffReadInSections - read in COFF sections
* 
*/

LOCAL STATUS ecoffReadInSections
    (
    int    fd,
    char   **pSectPtr,
    SCNHDR *pScnHdr,
    BOOL   swapTables 
    )
    {
    FAST int ix;

    for (ix = 0; ix < MAX_SCNS; ix++)
	{
	if (pScnHdr->s_size != 0 && pScnHdr->s_scnptr != 0)
	    {
            if ((*pSectPtr = (char *) malloc ((UINT)pScnHdr->s_size)) == NULL)
		{
		return (ERROR);
		}

            if ((softSeek (fd, pScnHdr->s_scnptr) == ERROR) ||
                (fioRead (fd, *pSectPtr, (int) pScnHdr->s_size) != 
		 pScnHdr->s_size))
		{
		return (ERROR);
		}

	    }

	pScnHdr++;
	pSectPtr++;
	}

    return (OK);
    }

/*******************************************************************************
*
* ecoffLdSections - write out sections to memory
* 
*/

LOCAL STATUS ecoffLdSections
    (
    char    **pSectPtr,
    SCNHDR  *pScnHdr,
    AOUTHDR *pOptHdr,
    char    *pText,
    char    *pData 
    )
    {
    int     ix;
    int     textCount = 0;
    int     dataCount = 0;

    for (ix = 0; ix < MAX_SCNS; ix++)
	{
	if (strcmp (pScnHdr->s_name, _TEXT) == 0 ||
	    strcmp (pScnHdr->s_name, _INIT) == 0)
	    {
            bcopy (*pSectPtr, pText, pScnHdr->s_size);
	    pText = (char *) ((int) pText + pScnHdr->s_size);
	    textCount += pScnHdr->s_size;
	    }
	else if (strcmp (pScnHdr->s_name, _DATA) == 0 ||
		 strcmp (pScnHdr->s_name, _RDATA) == 0 ||
		 strcmp (pScnHdr->s_name, _SDATA) == 0 ||
		 strcmp (pScnHdr->s_name, _LIT4) == 0  ||
		 strcmp (pScnHdr->s_name, _LIT8) == 0)
	    {
            bcopy (*pSectPtr, pData, pScnHdr->s_size);
	    pData = (char *) ((int) pData + pScnHdr->s_size);
	    dataCount += pScnHdr->s_size;
	    }
	else
	    {
	    /* ignore all other sections */
	    }

	pScnHdr++;
	pSectPtr++;
	}

    /* did we copy it all ? */

    if ((textCount != pOptHdr->tsize) || (dataCount != pOptHdr->dsize))
	{
	return (ERROR);
	}

    return (OK);
    }

/*******************************************************************************
*
* ecoffReadRelocInfo - read in COFF relocation information and swap if necessary
* 
*/

LOCAL STATUS ecoffReadRelocInfo
    (
    int     fd,
    SCNHDR  *pScnHdrFirst,
    RELOC   *relsPtr[],
    BOOL    swapTables 
    )
    {
    int     relocSize;
    int     position;		/* present file position */
    SCNHDR  *pScnHdr;		/* try to use array instead pScnHdr[MAX_SCNS] */
    int     ix;			/* index thru reloc info sections */
    int     iy;			/* index thru section headers */

    for (ix = 0; ix < MAX_SCNS; ix++)
	{
	position = ioctl (fd, FIOWHERE, (int) 0);

        /*
	 * look through each pScnHdr for relocation offset equal to 
	 * current position.  should they be equal, then this relocation 
	 * data corresponds to that section header.  Use the section header 
	 * array index so that the resulting relocation information will
	 * be in the same order as the section headers.
	 */

	pScnHdr = pScnHdrFirst;
        for (iy = 0; iy < MAX_SCNS; iy++)
	    {
            if ((pScnHdr->s_nreloc > 0) && (pScnHdr->s_relptr == position))
	        {
	        relocSize = (int) pScnHdr->s_nreloc * RELSZ;

                /* use array instead of pRelPtr */
		/* then use header index into array */
	        if ((relsPtr[iy] = (RELOC *)malloc (relocSize)) == NULL)
		    {
		    return (ERROR);
		    }

	        if (fioRead (fd, (char *) relsPtr[iy], relocSize) != relocSize)
		    {
		    return (ERROR);
		    }

	        if (swapTables)
		    {
		    swapCoffrelocTbl(relsPtr[iy], pScnHdr->s_nreloc);
		    }

	        break; /* found the right header, no need to look at the rest */
	        }
	    else
	        pScnHdr++;
	    }
        }

    return (OK);
    }

/*******************************************************************************
*
* ecoffReadSymHdr - read in COFF symbol header and swap if necessary
* 
*/

LOCAL STATUS ecoffReadSymHdr
    (
    int    fd,
    HDRR   *pSymHdr,
    FILHDR *pHdr,
    BOOL   swapTables 
    )
    {

    if (pHdr->f_nsyms == 0)
	{
	return (ERROR);
	}
    else
	{

        if ((softSeek (fd, pHdr->f_symptr) == ERROR) || 
            (fioRead (fd, (char *) pSymHdr, pHdr->f_nsyms) != pHdr->f_nsyms))
	    {
	    return (ERROR);
	    }

	if (swapTables)
	    {
	    swapCoffsymHdr (pSymHdr);
	    }
	}

    return (OK);
    }

/*******************************************************************************
*
* ecoffReadExtStrings - read in COFF external strings
* 
*/

LOCAL STATUS ecoffReadExtStrings
    (
    int  fd,
    char **pStrBuf,
    HDRR *pSymHdr 
    )
    {

    if (pSymHdr->issExtMax > 0) 		/* is there a string table? */
        {
        if ((*pStrBuf = (char *) malloc ((UINT) pSymHdr->issExtMax)) == NULL)
	    {
	    return (ERROR);
	    }

	if ((softSeek (fd, pSymHdr->cbSsExtOffset) != OK) || 
	    (fioRead (fd, *pStrBuf, (int) pSymHdr->issExtMax) != 
	     pSymHdr->issExtMax))
	    {
	    return (ERROR);
	    }
        }

    return (OK);
    }

/*******************************************************************************
*
* ecoffReadExtSyms - read in COFF external symbols and swap if necessary
* 
*/

LOCAL STATUS ecoffReadExtSyms
    (
    int  fd,
    EXTR **pExtSyms,
    HDRR *pSymHdr,
    BOOL swapTables 
    )
    {

    int extSymSize = (int) (pSymHdr->iextMax * cbEXTR);

    if (extSymSize != 0)		/* is there a symbol table? */
	{
        if ((*pExtSyms = (EXTR *) malloc (extSymSize)) == NULL)
	    {
	    return (ERROR);
	    }

        if (softSeek (fd, pSymHdr->cbExtOffset) == ERROR) 
	    {
	    return (ERROR);
	    }

        if (fioRead (fd, (char *) *pExtSyms, extSymSize) != extSymSize)
	    {
	    return (ERROR);
	    }

	if (swapTables)
	    {
	    swapCoffsymbols (*pExtSyms, pSymHdr->iextMax);
	    }
	}

    return (OK);
    }

/*******************************************************************************
*
* sizeEcoffCommons - 
* 
*  INTERNAL
*  All common external symbols are being tacked on to the end
*  of the BSS section.  Each symbol is examined and placed at
*  the appropriate address.  The appropriate address is determined
*  by examining the address of the previous common symbol and the
*  size of the current common symbol.  (The size of the symbol is 
*  contained in the U_SYM_VALUE field of the symbol entry)  The 
*  address of a symbol must be correctly aliged for data access.  
*  For example, on the R3000 architecture word accesses must be word 
*  aligned.  The macro COMMON_ALIGNMENT is the base boundry size for the
*  architecture.
*/

LOCAL ULONG sizeEcoffCommons 
    (
    EXTR *pNextSym,
    ULONG alignBss 
    )
    {
    ULONG fixupBss;
    ULONG nbytes = 0;

    fixupBss = ((alignBss % (ULONG) pNextSym->U_SYM_VALUE) & 
        (COMMON_ALIGNMENT - 1));

    if (fixupBss != 0)
        {
        fixupBss = COMMON_ALIGNMENT - fixupBss;
        }

    nbytes = (ULONG) pNextSym->U_SYM_VALUE + fixupBss;	

    return (nbytes);
    }

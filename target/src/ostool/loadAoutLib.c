/* loadAoutLib.c - UNIX a.out object module loader */

/* Copyright 1984-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
06o,11may02,fmk  SPR 77007 - improve common symbol support
06n,28mar02,jn   rewind to the beginning of the file before reading the header
                 information (SPR 73145)
06m,30nov98,dbt  no longer clear seg.flags<xxx> after loadSegmentAllocate()
                 call. (SPR #23553).
06l,05oct98,pcn  Initialize all the fields in the SEG_INFO structure.
06k,16sep98,pcn  Set to _ALLOC_ALIGN_SIZE the flags field in seg structure
                 (SPR #21836).
06j,17jul98,pcn  Fixed SPR #21836: alignment mismatch between sections and
                 target.
06i,31oct96,elp  Replaced symAdd() call by symSAdd() call (symtbls synchro).
06h,07aug96,dbt  call loadModuleGet with MODULE_A_OUT format (SPR #3006).
06g,23feb95,caf  fixed #endif.
06f,12jun93,rrr  vxsim.
06e,30oct92,jwt  #if for nonLongErrMsg[] for all CPU_FAMILYs but SPARC.
06d,28oct92,yao  added support for non-contiguous text/data/bss segments in 
            jwt  relSegmentSparc(); cleaned up warnings and relSegmentSparc().
06c,16oct92,jmm  fixed spr 1664 - local symbols always added (in RdSymtab())
06b,31jul92,dnw  changed to call CACHE_TEXT_UPDATE.  doc tweak.
06a,29jul92,jcf  removed unnecessary forward declaration.
05z,29jul92,elh  Move boot routines to bootAoutLib.c
05y,22jul92,jmm  moduleSegAdd now called for all modules
                 loadSegmentsAllocate now called to allocate memory
05x,21jul92,rdc  mods to support text segment write protection.
		 changed the way aoutLdMdlAtSym allocates mem for segs
		 (didn't really work before.)
05w,14jul92,jmm  added support for unloading, including calls to moduleLib
                 permanent cache fix for MC68040
		 moved addSegNames to loadLib.c
05v,04jul92,smb  temporary cache fix for MC68040.
05u,01jul92,jmm  more cleanup of magic numbers
05t,23jun92,rrr  cleanup of some magic numbers
05s,18jun92,ajm, made object module independant with split from loadLib.c
            jwt  fixed masking bug in relSegmentSparc() as per SPARC 05i.
05r,26may92,rrr  the tree shuffle
05q,10dec91,gae  added includes for ANSI.
05p,19nov91,rrr  shut up some ansi warnings.
05o,05oct91,ajm  changed bad ifdef of CPU==MIPS to CPU_FAMILY in relSegmentR3k
05n,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -fixed #else and #endif
		  -changed TINY and UTINY to INT8 and UINT8
		  -changed READ, WRITE and UPDATE to O_RDONLY O_WRONLY and ...
		  -changed VOID to void
		  -changed copyright notice
05m,30aug91,ajm  added MIPS support from 68k 05h, and mips 4.02 05q.
05l,28aug91,shl  added cache coherency calls for MC68040 support.
05k,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by dnw.
05j,31mar91,del  made file null for I960.
05i,08feb91,jaa	 documentation cleanup.
05h,05oct90,dnw  made loadModuleAtSym() be NOMANUAL.
05g,30jul90,dnw  changed pathLastName() to pathLastNamePtr().
		 added forward declaration of void routines.
05f,11may90,yao  added missing modification history (05e) for the last checkin.
05e,09may90,yao  typecasted malloc to (char *).
05d,22aug89,jcf  updated to allow for separate symbol tables.
		  symbol table type now a SYM_TYPE.
05c,06jul89,ecs  updated to track 4.0.2.
05b,06jun89,ecs  fixed bug in relSegmentSparc.
05a,01may89,ecs  split relSegment into relSegmentSparc & relSegment68k.
04g,06jul89,llk  changed call to pathLastName().
04f,16jun89,llk  made loadModuleAt add absolute symbols to symbol table.
		 created SEG_INFO structure to reduce parameter passing.
04e,09jun89,llk  fixed bug so that symbols which reside in segments that have
		   been located at a specific address now have the correct
		   relocated address entered in the symbol table.
04d,01may89,gae  added empty N_ABS case to relSegment to stop useless warnings.
04c,02oct88,hin  used correct N_TYPE constant in 04b fix.
04b,29sep88,gae  fixed relative relocation bug with Greenhill's a.out,
		   sort of introduced in 03y.
04a,07sep88,gae  applied dnw's documentation.
03z,15aug88,jcf  lint.
03y,30jun88,dnw  fixed bug of not relocating relative to data or bss correctly.
	    llk
03x,05jun88,dnw  changed from ldLib to loadLib.
		 removed pathTail.
03w,30may88,dnw  changed to v4 names.
03v,28may88,dnw  removed ldLoadAt.
03u,19nov87,dnw  made ldLoadModule tolerant of 0 length segments, strings, etc.
03t,04nov87,rdc  made ldLoadModule not create segment names if NO_SYMBOLS.
03s,16oct87,gae  removed ldLoadName, or ldLoadFile, and made ldLoadModule
		   determine and add module name to symbol table.
		 added pathTail().
03r,29jul87,ecs  added ldLoadFile, ldLoadModule.
	    rdc  fixed rdSymtab to zero out space malloc'd for common symbols.
	    dnw  fixed to require exact match with system symbol table
		  instead of allowing matches with or without leading '_'.
		 fixed to not link COMM symbols to existing symbols.
03q,02jul87,ecs  changed rdSymtab to call symFindType, instead of symFind.
03p,02apr87,ecs  added include of strLib.h
03o,23mar87,jlf  documentation.
03n,20dec86,dnw  changed to not get include files from default directories.
03m,24nov86,llk  deleted SYSTEM conditional compiles.
03l,09oct86,dnw  fixed handling of "pc relative" relocation commands.
		 cleaned up rdSymtab.
		 changed to allocate externals table based on size of symbol
		   table in object module.
03k,04sep86,jlf  minor documentation.
03j,27jul86,llk  prints error messages to standard error
03i,24jun86,rdc  now throws away stabs and absolute symbols.  Now
		 dynamically allocates and frees necessary buffers.
		 now searches symbol table before allocating new symbol
		 for type N_COMM.
03h,04jun86,dnw  changed sstLib calls to symLib.
03g,03mar86,jlf  changed ioctrl calls to ioctl.
03f,22oct85,dnw  Fixed bug reading 4 bytes too many for BSD4.2 string table.
03e,11oct85,jlf  Made the string table buffer and the externals buffer
		   be allocated rather than on the stack.
		 Made new routine ldInit.
		 Got rid of MAX_STRINGS and MAX_SYMBOLS, and replaced them with
		   local variables which are initialize by ldInit.
		 De-linted.
03d,10oct85,jlf  upped MAX_STRINGS form 20000 to 25000.  Made all
		 references use the #define, instead of raw numbers.
03c,27aug85,rdc  changed MAX_SYM_LEN to MAX_SYS_SYM_LEN.
03b,12aug85,jlf  fixed to not try to relocate segments that have already
		 been relocated pc-relative in 4.2 version.  Also, made it
		 check relocation segment length, and print an error msg if
		 it's not a long.
03a,07aug85,jlf  combined 4.2 and v7/sys5 version.
02b,20jul85,jlf  documentation.
02a,24jun85,rdc  modified for 4.2 a.out format.
01e,19sep84,jlf  cleaned up comments a little
01d,09sep84,jlf  added comments, copyright, got rid of GLOBAL.
01c,10aug84,dnw  changed load to not add special module symbols {T,D,B,E}name
		   but also changed to add _etext, _edata, and _end as normal
		   symbols.
		 fixed rdSymtab to not set S_ldLib_UNDEFINED_SYMBOL and
		   instead just leave it S_symLib_SYMBOL_NOT_FOUND.
		 changed load to continue with relocation even if
		   undefined symbols were found.
		 changed call to 'fioOpen' to just 'open'.
		 replaced 'load' and 'loadat' with 'ldLoad' and 'ldLoadAt',
		   which now take an fd instead of filename.
01b,02jul84,ecs  changed format strings to be more unixlike.
01a,27apr84,dnw  written: some culled from old fioLib
*/

/*
DESCRIPTION
This library provides an object module loading facility.  Any UNIX BSD `a.out'
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
This code fragment would load the `a.out' file "objFile" located on
device "/devX/" into memory which would be allocated from the system
memory pool.  All external and static definitions from the file would be
added to the system symbol table.

This could also have been accomplished from the shell, by typing:
.CS
    -> ld (1) </devX/objFile
.CE

INCLUDE FILE: loadAoutLib.h

SEE ALSO: usrLib, symLib, memLib,
.pG "Basic OS"
*/

/* LINTLIBRARY */

#include "vxWorks.h"
#include "a_out.h"
#include "ioLib.h"
#include "loadLib.h"
#include "loadAoutLib.h"
#include "stdlib.h"
#include "pathLib.h"
#include "string.h"
#include "symLib.h"
#include "sysSymTbl.h"
#include "cacheLib.h"
#include "fioLib.h"
#include "logLib.h"
#include "memLib.h"
#include "errno.h"
#include "stdio.h"
#include "moduleLib.h"
#include "private/vmLibP.h"
#include "private/loadLibP.h"

#if	CPU_FAMILY==SIMSPARCSUNOS
#undef	CPU_FAMILY
#define	CPU_FAMILY	SPARC
#endif	/* CPU_FAMILY==SIMSPARCSUNOS */


/* The different systems use different names for the same info in some
 * structures.  Make them the same here.
 */

#define A_OUT_HDR exec
#define TEXTSIZE a_text
#define DATASIZE a_data
#define BSSSIZE a_bss
#define TRSIZE a_trsize
#define DRSIZE a_drsize

#define U_SYM_STRUCT nlist
#define U_SYM_TYPE n_type
#define U_SYM_VALUE n_value

#define RTEXT	0
#define RDATA	1
#define RBSS	2

LOCAL char stringMemErrMsg [] =
    "loadAoutLib error: insufficient memory for strings table (need %d bytes).\n";
LOCAL char extMemErrMsg [] =
    "loadAoutLib error: insufficient memory for externals table (need %d bytes).\n";
LOCAL char cantAddSymErrMsg [] =
    "loadAoutLib error: can't add '%s' to system symbol table - error = 0x%x.\n";
#if (CPU_FAMILY != SPARC)
LOCAL char nonLongErrMsg [] =
    "loadAoutLib error: attempt to relocate non-long address at 0x%x, code = %d.\n";
#endif
LOCAL char readStringsErrMsg [] =
    "loadAoutLib error: can't read string table - status = 0x%x\n";

/* forward static functions */

#if CPU_FAMILY == SPARC
static STATUS relSegmentSparc (int fd, int nbytes, char *segAddress, SEG_INFO
		*pSeg, char ** externals);
#else
static STATUS relSegment68k (int fd, int nbytes, char *segAddress, SEG_INFO
		*pSeg, char ** externals);
#endif

/* forward declarations */
 
LOCAL MODULE_ID aoutLdMdlAtSym (int fd, int symFlag, char **ppText,
				char **ppData, char **ppBss, SYMTAB_ID symTbl);
LOCAL STATUS rdSymtab (int fd, int nbytes, char ** *externals, int max_symbols,
		       int symFlag, SEG_INFO *pSeg, char *symStrings,
		       SYMTAB_ID symTbl, UINT16 group);

/* misc defines */

#define STORE_MASKED_VALUE(address, mask, value) \
    *(address) = ((*(address)) & ~(mask)) | ((value) & (mask))


/*******************************************************************************
*
* loadAoutInit - initialize the system for aout load modules
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

STATUS loadAoutInit 
    (
    void
    )
    {
    /* XXX check for installed ? */
    loadRoutine = (FUNCPTR) aoutLdMdlAtSym;
    return (OK);
    }

/******************************************************************************
*
* aoutLdMdlAtSym - load object module into memory with specified symbol table
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

LOCAL MODULE_ID aoutLdMdlAtSym
    (
    FAST int fd,        /* fd from which to read module */
    int symFlag,        /* symbols to be added to table */
                        /*   ([NO | GLOBAL | ALL]_SYMBOLS) */
    char **ppText,      /* load text segment at address pointed to by this */
                        /* pointer, return load address via this pointer */
    char **ppData,      /* load data segment at address pointed to by this */
                        /* pointer, return load address via this pointer */
    char **ppBss,       /* load bss segment at address pointed to by this */
                        /* pointer, return load address via this pointer */
    SYMTAB_ID symTbl    /* symbol table to use */
    )
    {
    struct A_OUT_HDR hdr;	/* module's a.out header stored here */
    SEG_INFO	seg;
    int		status;
    int		nbytes;
    FAST int	numExternals;
    UINT16 	group;
    MODULE_ID	moduleId;
    char *	pText		= (ppText == NULL) ? LD_NO_ADDRESS : *ppText;
    char *	pData 		= (ppData == NULL) ? LD_NO_ADDRESS : *ppData;
    char *	pBss  		= (ppBss  == NULL) ? LD_NO_ADDRESS : *ppBss;
    char **	externalsBuf   	= NULL;	/* buffer for reading externals */
    FAST char *	stringsBuf 	= NULL;
    char 	fileName[255];

    /* Initialization */

    memset ((void *)&seg, 0, sizeof (seg));

    /* Set up the module */

    ioctl (fd, FIOGETNAME, (int) fileName);
    moduleId = loadModuleGet (fileName, MODULE_A_OUT, &symFlag);

    if (moduleId == NULL)
        return (NULL);

    group = moduleId->group;

    /* 
     * Rewind to beginning of file before reading header 
     * XXX - JN - The return values of all these calls to ioctl should 
     * be checked.
     */

    ioctl(fd, FIOSEEK, 0);

    if (fioRead (fd, (char *) &hdr, sizeof (hdr)) != sizeof (hdr))
	goto error;

    seg.addrText = pText;
    seg.addrData = pData;
    seg.addrBss  = pBss;

    seg.sizeText = hdr.TEXTSIZE;
    seg.sizeData = hdr.DATASIZE;
    seg.sizeBss  = hdr.BSSSIZE;

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
	pBss = seg.addrBss;
	}

    /* read in all of text and initialized data */

    if ((fioRead (fd, pText, (int) hdr.TEXTSIZE) != hdr.TEXTSIZE) ||
        (fioRead (fd, pData, (int) hdr.DATASIZE) != hdr.DATASIZE))
	goto error;


    /* read in symbol strings by seeking to string stuff,
     * reading the first long which tells how big the
     * string table is, and then reading the string table */

    if ((ioctl (fd, FIOSEEK, N_STROFF(hdr)) == ERROR) ||
        (fioRead (fd, (char *) &nbytes, 4) != 4))
	goto error;

    if (nbytes != 0)
	{
	/* allocate and read in the string table */

	if ((stringsBuf = (char *) malloc ((unsigned) nbytes)) == NULL)
	    {
	    printErr (stringMemErrMsg, nbytes);
	    goto error;
	    }

	nbytes -= 4;	/* subtract 4 byte length we just read */

	if (fioRead (fd, &stringsBuf [4], nbytes) != nbytes)
	    {
	    printErr (readStringsErrMsg, errno);
	    goto error;
	    }
	}

    /* allocate the externalsBuf */

    numExternals = hdr.a_syms / sizeof (struct U_SYM_STRUCT);

    if (numExternals != 0)
	{
	externalsBuf = (char **) malloc ((unsigned) numExternals *
					 sizeof (char *));
	if (externalsBuf == NULL)
	    {
	    printErr (extMemErrMsg, numExternals * sizeof (char *));
	    goto error;
	    }
	}

    /* add segment names to symbol table before other symbols */

    if (!(symFlag & LOAD_NO_SYMBOLS))
	addSegNames (fd, pText, pData, pBss, symTbl, group);

    /* read in symbol table */

    if (ioctl (fd, FIOSEEK, N_SYMOFF(hdr)) == ERROR)
	goto error;

    status = rdSymtab (fd, (int) hdr.a_syms, &externalsBuf, numExternals,
		       symFlag, &seg, stringsBuf, symTbl, group);

    if (stringsBuf != NULL)
	{
	free (stringsBuf);	/* finished with stringsBuf */
	stringsBuf = NULL;
	}


    /* relocate text and data segments;
     *   note: even if symbol table had missing symbols, continue with
     *   relocation of those symbols that were found.
     *   note: relocation is for changing the values of the relocated
     *   symbols.  bss is uninitialized data, so it is not relocated
     *   in the symbol table.
     */

    if (ioctl (fd, FIOSEEK, N_TXTOFF(hdr) + hdr.a_text + hdr.a_data) == ERROR)
	goto error;

    if ((status == OK) || (errno == S_symLib_SYMBOL_NOT_FOUND))
	{
#if CPU_FAMILY == SPARC
	if ((hdr.a_machtype & 0xffff) == 0x0107)
	    {
	    if ((relSegmentSparc (fd, (int) hdr.TRSIZE, seg.addrText, &seg,
				  externalsBuf) != OK) ||
		(relSegmentSparc (fd, (int) hdr.DRSIZE, seg.addrData, &seg,
				  externalsBuf) != OK))
		 goto error;
	    }
	else
	    {
	    printErr ("Unsupported Object module format, a_machtype : %#x\n", 
		hdr.a_machtype);
	    goto error;
	    }
#else
	if ((hdr.a_machtype & 0xffff) == 0x0107)
	    {
	    if ((relSegment68k (fd, (int) hdr.TRSIZE, seg.addrText, &seg,
				externalsBuf) != OK) ||
		(relSegment68k (fd, (int) hdr.DRSIZE, seg.addrData, &seg,
				externalsBuf) != OK))
		 goto error;
	    }
	else
	    {
	    printErr ("Unsupported Object module format, a_machtype : %#x\n", 
		hdr.a_machtype);
	    goto error;
	    }
#endif
	}

    if (externalsBuf != NULL)
	{
	free ((char *) externalsBuf);	/* finished with externalsBuf */
	externalsBuf = NULL;
	}

    /* write protect the text if text segment protection is turned on */

    if (seg.flagsText & SEG_WRITE_PROTECTION) 
	{
	if (VM_STATE_SET (NULL, pText, seg.sizeProtectedText,
			  VM_STATE_MASK_WRITABLE,
			  VM_STATE_WRITABLE_NOT) == ERROR)
	    goto error;
	}

    /* return load addresses, where called for */

    if (ppText != NULL)
	*ppText = pText;

    if (ppData != NULL)
	*ppData = pData;

    if (ppBss != NULL)
	*ppBss = pBss;

    /* clear out bss */

    bzero (pBss, (int) hdr.BSSSIZE);

    /* flush stub to memory, flush any i-cache inconsistancies */

    cacheTextUpdate (pText, hdr.TEXTSIZE);

    /*
     * Add the segments to the module.
     * This has to happen after the relocation gets done.
     * If the relocation happens first, the checksums won't be
     * correct.
     */

    moduleSegAdd (moduleId, SEGMENT_TEXT, pText, hdr.TEXTSIZE, seg.flagsText);
    moduleSegAdd (moduleId, SEGMENT_DATA, pData, hdr.DATASIZE, seg.flagsData);
    moduleSegAdd (moduleId, SEGMENT_BSS, pBss, hdr.BSSSIZE, seg.flagsBss);
	
    if (status == OK)
        return (moduleId);
    else
        return (NULL);


    /* error:
     * clean up dynamically allocated temporary buffers and return ERROR */

error:
    if (stringsBuf != NULL)
	free (stringsBuf);
    if (externalsBuf != NULL)
	free ((char *) externalsBuf);

    /* flush stub to memory, flush any i-cache inconsistancies */

    cacheTextUpdate (pText, hdr.TEXTSIZE);

    /*
     * Delete the module associated with the unsuccessful load
     */

    moduleDelete (moduleId);

    return (NULL);
    }

/*******************************************************************************
*
* rdSymtab - read and process a symbol table
*
* This routine reads in a symbol table from an object file and processes
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
*/

LOCAL STATUS rdSymtab
    (
    FAST int fd,                /* fd of file positioned to symbol table */
    FAST int nbytes,            /* # of bytes of symbol table */
    char ***externals,          /* pointer to pointer to array to fill in
                                   values of undef externals */
    int max_symbols,            /* max symbols that can be put in 'externals' */
    int symFlag,                /* symbols to be added to table
                                 *   ([NO|GLOBAL|ALL]_SYMBOLS) */
    SEG_INFO *pSeg,             /* pointer to segment information */
    char *symStrings,           /* symbol string table (only meaningful in
                                   BSD 4.2) */
    SYMTAB_ID symTbl,           /* symbol table to use */
    UINT16 group		/* symbol group */
    )
    {
    struct U_SYM_STRUCT symbol;
    char *	adrs;
    SYM_TYPE	type;
    char *	bias;
    FAST char *	name;			/* symbol name (plus EOS) */
    int 	sym_num = 0;
    int 	status  = OK;

    for (; nbytes > 0; nbytes -= sizeof (symbol), ++sym_num)
	{
	/* read in next entry - symbol definition and name string */

	if (fioRead (fd, (char *) &symbol, sizeof (symbol)) != sizeof (symbol))
	    return (ERROR);

	/* we throw away stabs */

	if (symbol.n_type & N_STAB)
	    continue;

	/* symbol name string is in symStrings array indexed
	 * by symbol.n_un.n_name */

	name = symStrings + symbol.n_un.n_strx;

	/* add requested symbols to symbol table */

	if ((symbol.U_SYM_TYPE & N_TYPE) != N_UNDF)
	    {
	    if (((symFlag & LOAD_LOCAL_SYMBOLS) &&
		 !(symbol.U_SYM_TYPE & N_EXT)) ||
		((symFlag & LOAD_GLOBAL_SYMBOLS) && symbol.U_SYM_TYPE & N_EXT))
		{
		switch (symbol.U_SYM_TYPE & N_TYPE)
                    {
                    case N_ABS:
			bias = 0;
			break;
                    case N_DATA:
                        bias = pSeg->addrData - pSeg->sizeText;
                        break;
                    case N_BSS:
                        bias = pSeg->addrBss - pSeg->sizeText - pSeg->sizeData;
                        break;
                    default:
                        bias = pSeg->addrText;
                        break;
                    }

		if (symSAdd (symTbl, name, symbol.U_SYM_VALUE + bias,
			    symbol.U_SYM_TYPE, group) != OK)
		    printErr (cantAddSymErrMsg, name, errno);
		}
	    }
	else
	    {
	    /* 
             * undefined external symbol or "common" symbol - common
             * symbol type is denoted by undefined external with the
             * value set to non-zero. If symbol is a common, call
             * loadCommonManage() to process.  Alignment information
             * is not specified by a.out so '0' is passed as an
             * alignment parameter to loadCommonManage.
             */
	    
	    if (symbol.U_SYM_VALUE != 0)
		{
                if (loadCommonManage (symbol.U_SYM_VALUE, 0, name, 
                    symTbl, (void *) &adrs,  &type, 
                    symFlag, pSeg, group) != OK)
                   status = ERROR;
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


	    /* add symbol address to externals table, but first
	     * check for enough room in 'externals' table */

	    if (sym_num == max_symbols)
		{
		/* no more room in symbol table -
		 * make externals table half as big again */

		max_symbols += max_symbols / 2;

		*externals = (char **) realloc ((char *) *externals,
						(unsigned) max_symbols *
						sizeof (char *));

		if (*externals == NULL)
		    return (ERROR);
		}

	    (*externals) [sym_num] = adrs;	/* enter address in table */
	    }
	}

    return (status);
    }

#if CPU_FAMILY == SPARC

/*******************************************************************************
*
* relSegmentSparc - perform relocation commands for a SPARC segment
*
* This routine reads the specified relocation command segment and performs
* all the relocations specified therein.  External relocations are looked 
* up in the 'externals' table.  All other relocations are relative to how 
* much a particular segment has moved. 
*
* RETURNS: OK or ERROR
*/

LOCAL STATUS relSegmentSparc
    (
    int		fd,		/* fd of file positioned to reloc commands */
    int		nbytes,		/* # of bytes of reloc commands for this seg */
    FAST char *	segAddress,	/* addr of segment being relocated */
    SEG_INFO *	pSeg,		/* pointer to segment information */
    char **	externals	/* addresses of external symbols */
    )
    {
    ULONG *	pAddr;       	/* address in memory to receive relocated val */
    RINFO_SPARC rinfo;  	/* relocation command goes here */
    ULONG 	relVal;       	/* relocated value to be put into memory */

    for (; nbytes > 0; nbytes -= sizeof (rinfo))
        {
        /* read relocation command */

        if (fioRead (fd, (char *) &rinfo, sizeof (rinfo)) != sizeof (rinfo))
            return (ERROR);

        /* calculate actual address in memory that needs relocation,
         * and perform external or segment relative relocation */

        pAddr = (ULONG *) ((ULONG) segAddress + (ULONG) rinfo.r_address);

        if (rinfo.r_extern)
            relVal = (ULONG) externals[rinfo.r_index] + rinfo.r_addend;
        else
	    switch (rinfo.r_index & N_TYPE)
		{
		case N_TEXT:
		     relVal = (ULONG) pSeg->addrText + rinfo.r_addend;
		     break;

		case N_DATA:
		     relVal = ((ULONG) pSeg->addrData - pSeg->sizeText) + 
		              rinfo.r_addend;
		     break;

		case N_BSS:
		     relVal = ((ULONG) pSeg->addrBss - pSeg->sizeText - 
			       pSeg->sizeData) + rinfo.r_addend;
		     break;

		default:
		     logMsg ("Invalid relocation index %d\n", rinfo.r_addend,
			     0, 0, 0, 0, 0);
		     return (ERROR);
		     break;
		}

	switch (rinfo.r_type)
	    {
	    case RELOC_8:
		*(UINT8 *) pAddr = relVal;
		break;
	    case RELOC_16:
		*(USHORT *) pAddr = relVal;
		break;
	    case RELOC_32:
		*pAddr = relVal;
		break;
	    case RELOC_DISP8:
		*(UINT8 *) pAddr = relVal - (ULONG) segAddress;
		break;
	    case RELOC_DISP16:
		*(USHORT *) pAddr = relVal - (ULONG) segAddress;
		break;
	    case RELOC_DISP32:
		*pAddr = relVal - (ULONG) segAddress;
		break;
	    case RELOC_WDISP30:
		STORE_MASKED_VALUE (pAddr, 0x3fffffff, 
				    (((ULONG) relVal - 
				      (ULONG) segAddress) >> 2));
		break;
	    case RELOC_WDISP22:
		STORE_MASKED_VALUE (pAddr, 0x003fffff, 
				    (((ULONG) relVal - 
				      (ULONG) segAddress) >> 2));
		break;
	    case RELOC_HI22:
		STORE_MASKED_VALUE (pAddr, 0x003fffff, relVal >> 10);
		break;
	    case RELOC_22:
                STORE_MASKED_VALUE (pAddr, 0x003fffff, relVal);
		break;
	    case RELOC_13:
		STORE_MASKED_VALUE (pAddr, 0x00001fff, relVal);
		break;
	    case RELOC_LO10:
		STORE_MASKED_VALUE (pAddr, 0x000003ff, relVal);
		break;

	    case RELOC_SFA_BASE:
	    case RELOC_SFA_OFF13:
	    case RELOC_BASE10:
	    case RELOC_BASE13:
	    case RELOC_BASE22:
	    case RELOC_PC10:
	    case RELOC_PC22:
	    case RELOC_JMP_TBL:
	    case RELOC_SEGOFF16:
	    case RELOC_GLOB_DAT:
	    case RELOC_JMP_SLOT:
	    case RELOC_RELATIVE:
            default:
                logMsg ("Unknown relocation type %d\n", rinfo.r_type,
                        0, 0, 0, 0, 0);
		return (ERROR);
                break;
            }
        }

    return (OK);
    }

#else

/*******************************************************************************
*
* relSegment68k - perform relocation commands for a 680X0 segment
*
* This routine reads the specified relocation command segment and performs
* all the relocations specified therein.
* External relocations are looked up in the 'externals' table.
* All other relocations are relative to how much a particular segment has moved.
*
* RETURNS: OK or ERROR
*/

LOCAL STATUS relSegment68k
    (
    int		fd,		/* fd of file positioned to reloc commands */
    int		nbytes,		/* # of bytes of reloc commands for this seg */
    FAST char *	segAddress,	/* addr of segment being relocated */
    SEG_INFO *	pSeg,		/* pointer to segment information */
    char **	externals	/* addresses of external symbols */
    )
    {
    FAST char **pAddress;
    FAST char *textAddress = pSeg->addrText;

    RINFO_68K rinfo;		/* relocation command goes here */

    for (; nbytes > 0; nbytes -= sizeof (rinfo))
	{
	/* read relocation command */

	if (fioRead (fd, (char *) &rinfo, sizeof (rinfo)) !=
							sizeof (rinfo))
	    {
	    return (ERROR);
	    }


	/* calculate actual address in memory that needs relocation,
	 * and perform external or segment relative relocation */

	pAddress = (char **) (segAddress + rinfo.r_address);

	if (rinfo.r_extern)
	    {
	    if (rinfo.r_length != 2)
		printErr (nonLongErrMsg, pAddress, rinfo.r_length);
	    else
		{
		/* add external address to relocatable value */

		*pAddress += (int) externals [rinfo.r_symbolnum];

		/* relocations marked pc-relative were done assuming
		 * text starts at 0.  Adjust for actual start of text. */

		if (rinfo.r_pcrel)
		    *pAddress -= (int) textAddress;
		}
	    }
	else
	    {
	    /* pc-relative internal references are already ok.
	     * other internal references need to be relocated relative
	     * to start of the segment indicated by r_symbolnum. */

	    if (!rinfo.r_pcrel)
		{
		if (rinfo.r_length != 2)
		    printErr (nonLongErrMsg, pAddress, rinfo.r_length);
		else
		    {
		    /* some compiler's a.out may have the external bit set
		     * -- just ignore
		     */

		    switch (rinfo.r_symbolnum & N_TYPE)
			{
                        /* Text, data, and bss may not be contiguous
                         * in vxWorks (unlike UNIX).  Relocate by
                         * calculating the actual distances that a
                         * segment has moved.
                         */

			case N_ABS:
			    break;

			case N_TEXT:
			    *pAddress += (int) textAddress;
			    break;

			case N_DATA:
			    *pAddress += (int) pSeg->addrData - pSeg->sizeText;
			    break;

			case N_BSS:
			    *pAddress += (int) pSeg->addrBss - pSeg->sizeText
					 - pSeg->sizeData;
			    break;

			default:
			    printErr ("Unknown symbol number = %#x\n",
					rinfo.r_symbolnum);
			}
		    }
		}
	    }
	}

    return (OK);
    }

#endif

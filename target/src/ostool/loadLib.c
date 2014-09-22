/* loadLib.c - object module loader */

/* Copyright 1984-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
06t,08may02,fmk  SPR 77007 - improve common symbol support - port
                 loadCommonManage() and loadCommonMatch() from the host loader
06s,07mar02,jn   refix SPR # 30588 - return NULL when there are unresolved
                 symbols
06r,07feb02,jn   documentation - loadModule returns a module id, not NULL,
                 when there are unresolved symbols
06q,15oct01,pad  changed loadModuleAt() documentation about obsolete
		 <filename>_[text | data | bss] symbols.
06p,30nov98,dbt  clear seg.flags<xxx> in loadSegmentAllocate (SPR #23553).
		 If we are using text protection, do not use file alignment
		 if it is inferior to page size (SPR #23553).
06o,17jul98,pcn  Fixed SPR #21836: alignment mismatch between sections and
		 target.
06n,22nov96,elp  Synchronize only valid modules.
06m,06nov96,elp  replaced symAdd() calls by symSAdd() calls +
		 added symtbls synchro function pointer <syncLoadRtn>.
06l,04nov96,dgp  doc: fix for SPR #4108
06k,14oct95,jdi  doc: fixed unnecessary refs to UNIX.
06j,11feb95,jdi  doc tweaks.
06i,08dec94,rhp  docn: clarify RELOCATION section in loadModuleAT() (SPR#3769)
06h,19sep94,rhp  docn: show new load flags for loadModuleAt() (SPR#2369).
06g,16sep94,rhp  docn: deleted obsolete paragraph (SPR#2520)
06f,13feb93,kdl  changed cplusLib.h to private/cplusLibP.h (SPR #1917).
06e,27nov92,jdi  documentation cleanup.
06d,25sep92,jmm  added check of malloc's return value to loadSegmentsAllocate()
06c,10sep92,jmm  moved call to moduleLibInit() to usrConfig.c
06b,01aug92,srh  added include cplusLib.h.;
		 added call to cplusLoadFixup in loadModuleAtSym
06a,30jul92,jmm  documentation cleanup
05z,23jul92,jmm  loadSegmentsAllocate() created to alloc memory for all loaders
05v,18jul92,smb  Changed errno.h to errnoLib.h.
05u,16jul92,jmm  moved addSegNames here
		 added support for module tracking
05t,23jun92,ajm  fixed EXAMPLE that last change mucked
05s,18jun92,ajm  separated to be object module independent
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
This library provides a generic object module loading facility.  Any
supported format files may be loaded into memory, relocated properly, their
external references resolved, and their external definitions added to
the system symbol table for use by other modules and from the shell.
Modules may be loaded from any I/O stream which allows repositioning of the
pointer.  This includes netDrv, nfs, or local file devices.  It does not 
include sockets.

EXAMPLE
.CS
    fdX = open ("/devX/objFile", O_RDONLY);
    loadModule (fdX, LOAD_ALL_SYMBOLS);
    close (fdX);
.CE
This code fragment would load the object file "objFile" located on
device "/devX/" into memory which would be allocated from the system
memory pool.  All external and static definitions from the file would be
added to the system symbol table.

This could also have been accomplished from the shell, by typing:
.CS
    -> ld (1) </devX/objFile
.CE

INCLUDE FILE: loadLib.h

SEE ALSO: usrLib, symLib, memLib,
.pG "Basic OS"
*/

/* defines */

#undef INCLUDE_SDA		/* SDA is not supported for the moment */

/* 
 * SYM_BASIC_NOT_COMM_MASK and SYM_BASIC_MASK are temporary masks until
 * symbol values are harmonized between host and target sides
 */

#define SYM_BASIC_NOT_COMM_MASK   0x0d   /* basic mask but unset bit 
                                          * two for common symbol */
#define SYM_BASIC_MASK            0x0f   /* only basic types are of interest */

/* includes */

#include "vxWorks.h"
#include "sysSymTbl.h"
#include "ioLib.h"
#include "loadLib.h"
#include "fioLib.h"
#include "errnoLib.h"
#include "stdio.h"
#include "moduleLib.h"
#include "a_out.h"
#include "pathLib.h"
#include "private/vmLibP.h"
#include "stdlib.h"
#include "private/cplusLibP.h"
#include "string.h"
#include "memLib.h"
#include "private/loadLibP.h"

/* locals */

LOCAL char cantAddSymErrMsg [] =
    "loadAoutLib error: can't add '%s' to system symbol table - error = 0x%x.\n";

/* define generic load routine */

FUNCPTR loadRoutine = (FUNCPTR) NULL;
FUNCPTR syncLoadRtn = (FUNCPTR) NULL;

/*******************************************************************************
*
* loadModule - load an object module into memory
*
* This routine loads an object module from the specified file, and places
* the code, data, and BSS into memory allocated from the system memory
* pool.
*
* This call is equivalent to loadModuleAt() with NULL for the addresses of
* text, data, and BSS segments.  For more details, see the manual entry for
* loadModuleAt().
*
* RETURNS:
* MODULE_ID, or NULL if the routine cannot read the file, there is not
* enough memory, or the file format is illegal.
*
* SEE ALSO: loadModuleAt()
*/

MODULE_ID loadModule
    (
    int fd,		/* fd of file to load */
    int symFlag		/* symbols to add to table */
			/* (LOAD_[NO | LOCAL | GLOBAL | ALL]_SYMBOLS) */
    )
    {
    return (loadModuleAt (fd, symFlag, (char **)NULL, (char **)NULL,
			  (char **)NULL));
    }
/*******************************************************************************
*
* loadModuleAt - load an object module into memory
*
* This routine reads an object module from <fd>, and loads the code, data,
* and BSS segments at the specified load addresses in memory set aside by
* the user using malloc(), or in the system memory partition as described
* below.  The module is properly relocated according to the relocation
* commands in the file.  Unresolved externals will be linked to symbols
* found in the system symbol table.  Symbols in the module being loaded can
* optionally be added to the system symbol table.
*
* LINKING UNRESOLVED EXTERNALS 
* As the module is loaded, any unresolved external references are
* resolved by looking up the missing symbols in the the system symbol
* table.  If found, those references are correctly linked to the new
* module.  If unresolved external references cannot be found in the
* system symbol table, then an error message ("undefined symbol: ...")
* is printed for the symbol, but the loading/linking continues.  The
* partially resolved module is not removed, to enable the user to
* examine the module for debugging purposes.  Care should be taken
* when executing code from the resulting module.  Executing code which 
* contains references to unresolved symbols may have unexpected results 
* and may corrupt the system's memory.
* 
* Even though a module with unresolved symbols remains loaded after this
* routine returns, NULL will be returned to enable the caller to detect
* the failure programatically.  To unload the module, the caller may
* either call the unload routine with the module name, or look up the
* module using the module name and then unload the module using the 
* returned MODULE_ID.  See the library entries for moduleLib and unldLib
* for details.  The name of the module is the name of the file loaded with
* the path removed.
*
* ADDING SYMBOLS TO THE SYMBOL TABLE
* The symbols defined in the module to be loaded may be optionally added
* to the system symbol table, depending on the value of <symFlag>:
* .iP "LOAD_NO_SYMBOLS" 29
* add no symbols to the system symbol table
* .iP "LOAD_LOCAL_SYMBOLS"
* add only local symbols to the system symbol table
* .iP "LOAD_GLOBAL_SYMBOLS"
* add only external symbols to the system symbol table
* .iP "LOAD_ALL_SYMBOLS"
* add both local and external symbols to the system symbol table
* .iP "HIDDEN_MODULE"
* do not display the module via moduleShow().
* .LP
*
* Obsolete symbols:
*
* For backward compatibility with previous releases, the following symbols
* are also added to the symbol table to indicate the start of each segment:
* <filename>_text, <filename>_data, and <filename>_bss, where <filename> is
* the name associated with the fd. Note that these symbols are not available 
* when the ELF format is used. Also they will disappear with the next VxWorks
* release. The moduleLib API should be used instead to get segment information.
*
* RELOCATION
* The relocation commands in the object module are used to relocate
* the text, data, and BSS segments of the module.  The location of each
* segment can be specified explicitly, or left unspecified in which
* case memory will be allocated for the segment from the system memory
* partition.  This is determined by the parameters <ppText>, <ppData>, and
* <ppBss>, each of which can have the following values:
* .iP "NULL"
* no load address is specified, none will be returned;
* .iP "A pointer to LD_NO_ADDRESS"
* no load address is specified, the return address is referenced by the pointer;
* .iP "A pointer to an address"
* the load address is specified.
* .LP
*
* The <ppText>, <ppData>, and <ppBss> parameters specify where to load
* the text, data, and bss sections respectively.  Each of these
* parameters is a pointer to a  pointer; for example, **<ppText>
* gives the address where the text segment is to begin.
*
* For any of the three parameters, there are two ways to request that
* new memory be allocated, rather than specifying the section's
* starting address: you can either specify the parameter itself as
* NULL, or you can write the constant LD_NO_ADDRESS in place of an
* address.  In the second case, loadModuleAt() routine replaces the
* LD_NO_ADDRESS value with the address actually used for each section
* (that is, it records the address at *<ppText>, *<ppData>, or
* *<ppBss>).
*
* The double indirection not only permits reporting the addresses
* actually used, but also allows you to specify loading a segment
* at the beginning of memory, since the following cases can be
* distinguished:
*
* .IP (1) 4
* Allocate memory for a section (text in this example):  <ppText> == NULL
* .IP (2)
* Begin a section at address zero (the text section, below):  *<ppText> == 0
* .LP
* Note that loadModule() is equivalent to this routine if all three of the
* segment-address parameters are set to NULL.
*
* COMMON
* Some host compiler/linker combinations use another storage class internally
* called "common".  In the C language, uninitialized global variables are
* eventually put in the bss segment.  However, in partially linked object
* modules they are flagged internally as "common" and the static 
* linker (host) resolves these and places them in bss as a final step 
* in creating a fully linked object module.  However, the target loader
* is most often used to load partially linked object modules.
* When the target loader encounters a variable labeled "common",
* its behavior depends on the following flags :
* .iP "LOAD_COMMON_MATCH_NONE" 8
* Allocate memory for the variable with malloc() and enter the variable
* in the target symbol table (if specified) at that address. This is
* the default.
* .iP "LOAD_COMMON_MATCH_USER"
* Search for the symbol in the target symbol table, excluding the
* vxWorks image  symbols. If several symbols exist, then the order of matching
* is:  (1) bss, (2) data. If no symbol is found, act like the
* default.
* .iP "LOAD_COMMON_MATCH_ALL"
* Search for the symbol in the target symbol table, including the
* vxWorks image symbols. If several symbols exist, then the order of matching is:  
* (1) bss, (2) data. If no symbol is found, act like the default.
* .LP
* Note that most UNIX loaders have an option that forces resolution of the
* common storage while leaving the module relocatable (for example, with
* typical BSD UNIX loaders, use options "-rd").
*
* EXAMPLES
* Load a module into allocated memory, but do not return segment addresses:
* .CS
*     module_id = loadModuleAt (fd, LOAD_GLOBAL_SYMBOLS, NULL, NULL, NULL);
* .CE
* Load a module into allocated memory, and return segment addresses:
* .CS
*     pText = pData = pBss = LD_NO_ADDRESS;
*     module_id = loadModuleAt (fd, LOAD_GLOBAL_SYMBOLS, &pText, &pData, &pBss);
* .CE
* Load a module to off-board memory at a specified address:
* .CS
*     pText = 0x800000;			/@ address of text segment	  @/
*     pData = pBss = LD_NO_ADDRESS	/@ other segments follow by default @/
*     module_id = loadModuleAt (fd, LOAD_GLOBAL_SYMBOLS, &pText, &pData, &pBss);
* .CE
*
* RETURNS:
* MODULE_ID, or
* NULL if the file cannot be read, there is not enough memory,
* the file format is illegal, or there were unresolved symbols.
*
* SEE ALSO:
* .pG "Basic OS" 
*/

MODULE_ID loadModuleAt
    (
    FAST int fd,	/* fd from which to read module */
    int symFlag,	/* symbols to add to table */
			/* (LOAD_[NO | LOCAL | GLOBAL | ALL]_SYMBOLS) */
    char **ppText,      /* load text segment at addr. pointed to by this */
			/* ptr, return load addr. via this ptr */
    char **ppData,      /* load data segment at addr. pointed to by this */
			/* pointer, return load addr. via this ptr */
    char **ppBss	/* load BSS segment at addr. pointed to by this */
			/* pointer, return load addr. via this ptr */
    )
    {
    return (loadModuleAtSym (fd, symFlag, ppText, ppData, ppBss, sysSymTbl));
    }

/******************************************************************************
*
* loadModuleAtSym - load object module into memory with specified symbol table
*
* This routine is the underlying routine to loadModuleAt().  This interface
* allows specification of the the symbol table used to resolve undefined
* external references and to which to add new symbols.
*
* RETURNS:
* MODULE_ID, or
* NULL if can't read file or not enough memory or illegal file format
*
* NOMANUAL
*/

MODULE_ID loadModuleAtSym
    (
    FAST int fd,	/* fd from which to read module */
    int symFlag,	/* symbols to be added to table
			 *   ([NO | GLOBAL | ALL]_SYMBOLS) */
    char **ppText,      /* load text segment at address pointed to by this
			 * pointer, return load address via this pointer */
    char **ppData,      /* load data segment at address pointed to by this
			 * pointer, return load address via this pointer */
    char **ppBss,       /* load bss segment at address pointed to by this
			 * pointer, return load address via this pointer */
    SYMTAB_ID symTbl    /* symbol table to use */
    )
    {
    MODULE_ID module;

    if (loadRoutine == NULL)
	{
	errnoSet (S_loadLib_ROUTINE_NOT_INSTALLED);
	return (NULL);
	}
    else
	{
	module = (MODULE_ID) (*loadRoutine) (fd, symFlag, ppText, ppData, ppBss,
					    symTbl);
	if (module != NULL)
	    {
	    cplusLoadFixup (module, symFlag, symTbl);

	    /* synchronize host symbol table if necessary */
	
	    if (syncLoadRtn != NULL)
		(* syncLoadRtn) (module);
	    }

	return module;
	}
    }

/******************************************************************************
*
* loadModuleGet - create module information for a loaded object module
*
* RETURNS:  MODULE_ID, or NULL if module could not be created.
*
* NOMANUAL
*/

MODULE_ID loadModuleGet
    (
    char * 	fileName,	/* name of the module */
    int		format,		/* object module format */
    int	*	symFlag		/* pointer to symFlag */
    )
    {
    MODULE_ID	moduleId;

    /* Change old style flags to new style */

    switch (*symFlag)
	{
	case NO_SYMBOLS:
	    *symFlag = LOAD_NO_SYMBOLS;
	    break;
	case GLOBAL_SYMBOLS:
	    *symFlag = LOAD_GLOBAL_SYMBOLS;
	    break;
	case ALL_SYMBOLS:
	    *symFlag = LOAD_ALL_SYMBOLS;
	    break;
	}

    moduleId = moduleCreate (fileName, format, *symFlag);

    return (moduleId);
    }

/*******************************************************************************
*
* addSegNames - add segment names to symbol table
*
* This routine adds names of the form "objectFile.o_text," objectFile.o_data,"
* and "objectFile.o_bss" to the symbol table.
*
* NOMANUAL
*/

void addSegNames
    (
    int		fd,		/* file descriptor for the object file */
    char	*pText,		/* pointer to the text segment */
    char	*pData,		/* pointer to the data segment */
    char	*pBss,		/* pointer to the BSS segment */
    SYMTAB_ID   symTbl,		/* symbol table to use */
    UINT16	group		/* group number */
    )
    {
    char fullname [MAX_FILENAME_LENGTH];
    char symName[MAX_SYS_SYM_LEN + 1];
    char *pName;

    if (ioctl (fd, FIOGETNAME, (int) fullname) == OK)
	pName = pathLastNamePtr (fullname);	/* get last simple name */
    else
	pName = "???";

    /* XXX
     * Adding these symbols is now obsolete, and should probably
     * be removed in a future version of the OS.  For now, leave them
     * in for backwards compatability.
     */

    if (pText != NULL)
	{
	sprintf (symName, "%s_text", pName);
	if (symSAdd (symTbl, symName, pText, (N_EXT | N_TEXT), group) != OK)
	    printErr (cantAddSymErrMsg, symName, errnoGet());
	}

    if (pData != NULL)
	{
	sprintf (symName, "%s_data", pName);
	if (symSAdd (symTbl, symName, pData, (N_EXT | N_DATA), group) != OK)
	    printErr (cantAddSymErrMsg, symName, errnoGet());
	}

    if (pBss != NULL)
	{
	sprintf (symName,  "%s_bss",  pName);
	if (symSAdd (symTbl, symName, pBss, (N_EXT | N_BSS), group) != OK)
	    printErr (cantAddSymErrMsg, symName, errnoGet());
	}
    }

/******************************************************************************
*
* loadSegmentsAllocate - allocate text, data, and bss segments
*
* This routine allocates memory for the text, data, and bss segments of
* an object module.  Before calling this routine, set up the fields of the
* SEG_INFO structure to specify the desired location and size of each segment.
*
* RETURNS: OK, or ERROR if memory couldn't be allocated.
*
* NOMANUAL
*/

STATUS loadSegmentsAllocate
    (
    SEG_INFO *	pSeg		/* pointer to segment information */
    )
    {
    BOOL 	protectText = FALSE;
    int 	pageSize;
    int		alignment = 0;
    int		textAlignment = pSeg->flagsText; /* save text alignment */
    int		dataAlignment = pSeg->flagsData; /* save data alignment */
    int		bssAlignment = pSeg->flagsBss;	 /* save bss alignment */

    /* clear pSeg->flags<xxx> fields */

    pSeg->flagsText = 0;
    pSeg->flagsData = 0;
    pSeg->flagsBss = 0;

    /*
     * if text segment protection is selected, then allocate the text
     * segment seperately on a page boundry
     */

    if ((pSeg->sizeText != 0) && vmLibInfo.protectTextSegs
	&& (pSeg->addrText == LD_NO_ADDRESS))
	{
	if ((pageSize = VM_PAGE_SIZE_GET()) == ERROR)
	    return (ERROR);

	/* round text size to integral number of pages */

	pSeg->sizeProtectedText = pSeg->sizeText / pageSize * pageSize
				  + pageSize;

	/*
	 * get the section alignment (maximum between page size and section
	 * alignment specified in the file).
	 */

	alignment = max(pageSize, textAlignment);

	if ((pSeg->addrText = memalign (alignment,
					pSeg->sizeProtectedText)) == NULL)
	    return (ERROR);

	pSeg->flagsText |= SEG_WRITE_PROTECTION;
	pSeg->flagsText |= SEG_FREE_MEMORY;
	protectText = TRUE;
	}

    /* if all addresses are unspecified, allocate one large chunk of memory */

    if (pSeg->addrText == LD_NO_ADDRESS &&
	pSeg->addrData == LD_NO_ADDRESS &&
	pSeg->addrBss  == LD_NO_ADDRESS)
	{
	/* SPR #21836 */

	alignment = 0;

	if (textAlignment > dataAlignment)
	    alignment = max (textAlignment, bssAlignment);
	else
	    alignment = max (dataAlignment, bssAlignment);

	if (alignment != 0)
	    pSeg->addrText = memalign (alignment, pSeg->sizeText + 
				       pSeg->sizeData + pSeg->sizeBss);
	else
	    pSeg->addrText = malloc (pSeg->sizeText + pSeg->sizeData +
				     pSeg->sizeBss);

	/* check for malloc error */

	if (pSeg->addrText == NULL)
	    return (ERROR);

	pSeg->addrData = pSeg->addrText + pSeg->sizeText;
	pSeg->addrBss = pSeg->addrData + pSeg->sizeData;

	pSeg->flagsText |= SEG_FREE_MEMORY;
	}

    /* if addresses not specified, allocate memory. */

    if (pSeg->addrText == LD_NO_ADDRESS && pSeg->sizeText != 0)
	{
	/* SPR #21836 */

	if (textAlignment != 0)
	    {
	    if ((pSeg->addrText = (char *) memalign (textAlignment,
		pSeg->sizeText)) == NULL)
		return (ERROR);
	    }
	else
	    {
	    if ((pSeg->addrText = (char *) malloc (pSeg->sizeText)) == NULL)
		return (ERROR);
	    }

	pSeg->flagsText |= SEG_FREE_MEMORY;
	}

    if (pSeg->addrData == LD_NO_ADDRESS && pSeg->sizeData != 0)
	{
	/* SPR #21836 */

	if (dataAlignment != 0)
	    {
	    if ((pSeg->addrData = (char *) memalign (dataAlignment,
		pSeg->sizeData)) == NULL)
		return (ERROR);
	    }
	else
	    {
	    if ((pSeg->addrData = (char *) malloc (pSeg->sizeData)) == NULL)
		return (ERROR);
	    }

	pSeg->flagsData |= SEG_FREE_MEMORY;
	}

    if (pSeg->addrBss == LD_NO_ADDRESS && pSeg->sizeBss)
	{
	/* SPR #21836 */

	if (bssAlignment != 0)
	    {
	    if ((pSeg->addrBss = (char *) memalign (bssAlignment,
		pSeg->sizeBss)) == NULL)
		return (ERROR);
	    }
	else
	    {
	    if ((pSeg->addrBss = (char *) malloc (pSeg->sizeBss)) == NULL)
		return (ERROR);
	    }

	pSeg->flagsBss |= SEG_FREE_MEMORY;
	}

    return (OK);
    }

/*******************************************************************************
*
* loadCommonMatch - support routine for loadCommonManage
*
* This is the support routine for all loadCommonManage() function.
* It fills the pSymAddr<type> fields of a COMMON_INFO structure if a global
* symbol matches the name of the common symbol.
*
* For each symbol type (SYM_DATA, SYM_BSS), the most recent occurence
* of a matching symbol in the target symbol table is recorded.
*
* RETURN : OK always.
*
* NOMANUAL
*/

STATUS loadCommonMatch
    (
    COMMON_INFO * pCommInfo,            /* what to do with commons */
    SYMTAB_ID     symTblId              /* ID of symbol table to look in */
    )
    {
    int         nodeIdx;                /* index of node of interest */
    int         mask;
    HASH_ID     hashId = symTblId->nameHashId;    /* id of hash table */
    SYMBOL      matchSymbol;            /* matching symbol */
    SYMBOL *    pSymNode = NULL;        /* symbol in node's link list */
    SYM_TYPE    basicType;              /* symbol's basic type */

    /* initialize symbol's characteristics to match */

    matchSymbol.name = pCommInfo->symName;
    matchSymbol.type = SYM_MASK_NONE;

    /* get corresponding node index into hash table */

    nodeIdx = (* hashId->keyRtn)(hashId->elements, (HASH_NODE *) &matchSymbol,
                                 hashId->keyArg);

    /* get first symbol in the node's linked list */

    pSymNode = (SYMBOL *) SLL_FIRST (&hashId->pHashTbl [nodeIdx]);

    /* walk the node's linked list until we reach the end */

    while(pSymNode != NULL)
        {
        /*
         * If the core's symbols are excluded from the search and the global
         * symbol is a core's symbol (group 0), jump to the next symbol.
         * If the symbol's name doesn't match the common symbol's name, jump
         * to the next symbol.
         */

        mask = SYM_BASIC_MASK;

        if(!((pCommInfo->vxWorksSymMatched == FALSE) && (pSymNode->group == 0)) &&
           (strcmp (pCommInfo->symName, pSymNode->name) == 0))
            {
            /* extract symbol's basic type */

	    /* 
             * since SYM_BASIC_MASK does not properly mask out SYM_COMM
             * we have to apply another mask to mask out bit two. Since 
             * bit two can  be  SYM_ABS, we have to check for SYM_COMM first
             */ 

            if ((pSymNode->type & SYM_COMM)== SYM_COMM)
                mask = SYM_BASIC_NOT_COMM_MASK;

            basicType = pSymNode->type & mask;

            /*
             * Record the global symbol's address accordingly with its type,
             * then jump to the next symbol.
             * Note that we only record the last occurence of the symbol in
             * the symbol table.
             */
            
            if((basicType == (SYM_BSS | SYM_GLOBAL)) &&
               (pCommInfo->pSymAddrBss == NULL))
                {
                pCommInfo->pSymAddrBss = (void *) pSymNode->value;
                pCommInfo->bssSymType = pSymNode->type;
                }
            else if((basicType == (SYM_DATA | SYM_GLOBAL)) &&
                    (pCommInfo->pSymAddrData == NULL))
                {
                pCommInfo->pSymAddrData = (void *) pSymNode->value;
                pCommInfo->dataSymType = pSymNode->type;
                }
            }

        /* point to next symbol in the node's linked list */

        pSymNode = (SYMBOL *) SLL_NEXT ((HASH_NODE *)pSymNode);
        }

    return (OK);
    }

/*******************************************************************************
*
* loadCommonManage - process a common symbol
*
* This routine processes the common symbols found in the object
* module.  Common symbols are symbols declared global without being
* assigned a value.  Good programming avoids such declarations since
* it is almost impossible to be sure what initialized global symbol
* should be used to solve a common symbol relocation.  The processing
* of common symbols depends on the following control flags, which
* enforce one of three levels of strictness for common symbol
* evaluation: 
* LOAD_COMMON_MATCH_NONE
* This is the default option. Common symbols are kept isolated,
* visible from the object module only. This option prevents any
* matching with already-existing symbols (in other words, the
* relocations that refer to these symbols are kept local). Memory is
* allocated and symbols are added to the symbol table with type
* SYM_COMM unless LOAD_NO_SYMBOLS is set.
* LOAD_COMMON_MATCH_USER

* The loader seeks a matching symbol in the target symbol table, but
* only symbols brought by user's modules are considered. If no
* matching symbol exists, it acts like LOAD_COMMON_MATCH_NONE. If
* several matching symbols exist, the order of preference is: symbols
* in the bss segment, then symbols in the data segment. If several
* matching symbols exist within a segment type, memory is allocated
* and the symbol most recently added to the target symbol table is
* used as reference.

* LOAD_COMMON_MATCH_ALL
* The loader seeks for a matching symbol in the target symbol table.
* All symbols are considered. If no matching symbol exists, then it
* acts like LOAD_COMMON_MATCH_NONE.  If several matches are found, the
* order is the same as for LOAD_COMMON_MATCH_USER.
*
* RETURNS
* OK or ERROR if the memory allocation or the addition to the symbol table
* fails.
*
* SEE ALSO
* API Programmer's Guide: Object Module Loader
*
* INTERNAL
* Note that with LOAD_COMMON_MATCH_USER, and moreover
* LOAD_COMMON_MATCH_ALL, it is not possible to know whether the symbol
* used for the evaluation is the right symbol. For instance, consider
* a module A defining "int evtAction = 0;", a module B defining "int
* evtAction;".  The symbol "evtAction" is declared as common in B
* module.  If B module is loaded before A module, and if
* LOAD_COMMON_MATCH_ALL is set, the loader will use the vxWorks'
* global symbol evtAction to solve this common symbol...
* It is therefore up to the user to download in the right order his
* modules, or use the right flag, or even better, to not use common
* symbols at all.  Note also that no search is done for previous
* common symbol.
*
* NOMANUAL 
*
*/

STATUS loadCommonManage
    (
    int         comAreaSize,    /* size of area required for common sym */
    int         comAlignment,   /* alignment required for common sym */
    char *      symName,        /* symbol name */
    SYMTAB_ID   symTbl,         /* target symbol table */
    SYM_ADRS *pSymAddr,       /* where to return symbol's address */
    SYM_TYPE *  pSymType,       /* where to return symbol's type */
    int         loadFlag,       /* control of loader's behavior */
    SEG_INFO *  pSeg,           /* section addresses and sizes */
    int         group           /* module group */
    )
    {
    COMMON_INFO commInfo;       /* what to do with commons */

    /* Force the default choice if no flag set */

    if(!(loadFlag & LOAD_COMMON_MATCH_ALL) &&
       !(loadFlag & LOAD_COMMON_MATCH_USER) &&
       !(loadFlag & LOAD_COMMON_MATCH_NONE))
        loadFlag |= LOAD_COMMON_MATCH_NONE;

    /* Must we do more than the default ? */

    if(!(loadFlag & LOAD_COMMON_MATCH_NONE))
        {
        /* Initialization */

        memset (&commInfo, 0, sizeof (COMMON_INFO));
        commInfo.symName = symName;

        /* Do we involve the core's symbols ? */

        if(loadFlag & LOAD_COMMON_MATCH_USER)
            commInfo.vxWorksSymMatched = FALSE;    /* no */
        else if(loadFlag & LOAD_COMMON_MATCH_ALL)
            commInfo.vxWorksSymMatched = TRUE;    /* yes */

        /* Get the last occurences of matching global symbols in bss and data */

        loadCommonMatch (&commInfo, symTbl);

        /* Prefered order for matching symbols is : bss then data*/

        if(commInfo.pSymAddrBss != NULL)    /* matching sym in bss */
            {
            *pSymAddr = commInfo.pSymAddrBss;
            *pSymType = commInfo.bssSymType;
            }
        else if(commInfo.pSymAddrData != NULL)    /* matching sym in data */
            {
            *pSymAddr = commInfo.pSymAddrData;
            *pSymType = commInfo.dataSymType;
            }
        else *pSymAddr = NULL;          /* no matching symbol */

        /*
         * If we found a matching symbol, stop here, otherwise apply
         * LOAD_COMMON_MATCH_NONE management.
         */

        if(*pSymAddr != NULL)
            return (OK);
	}

    /* If additional info is available then we are dealing with a PowerPC */

#ifdef	INCLUDE_SDA
    if(pSeg->pAdnlInfo != NULL)
        *pSymType = SYM_SDA | SYM_BSS | SYM_COMM | SYM_GLOBAL;
    else
#endif	/* INCLUDE_SDA */
        *pSymType = SYM_BSS | SYM_COMM | SYM_GLOBAL;

    /*
     * Allocate memory for new symbol. This must be done even if flag
     * LOAD_NO_SYMBOLS is applied (SPR #9259).
     */
 

#ifdef	INCLUDE_SDA
    if (pSeg->pAdnlInfo != NULL)
        {
        if (comAlignment != 0)
            *pSymAddr = (void *) memPartAlignedAlloc
                                 (((SDA_INFO *)pSeg->pAdnlInfo)->sdaMemPartId,
                                  comAreaSize, comAlignment);
	else
            *pSymAddr = (void *) memPartAlloc
                                 (((SDA_INFO *)pSeg->pAdnlInfo)->sdaMemPartId,
                                  comAreaSize);
	}
    else
#endif	/* INCLUDE_SDA */
        {
        if (comAlignment != 0)
            *pSymAddr = (void *) memalign (comAlignment, comAreaSize);
	else
	    *pSymAddr = (void *) malloc (comAreaSize);
	}

    if (*pSymAddr == NULL)
        return (ERROR);
 
    memset ((SYM_ADRS)*pSymAddr, 0, comAreaSize);
 
    /* Add symbol to the target symbol table if required */
 
    if(!(loadFlag & LOAD_NO_SYMBOLS) && (loadFlag & LOAD_GLOBAL_SYMBOLS))
        {
        if(symSAdd (symTbl, symName, (char *)*pSymAddr, *pSymType, 
		 (UINT16) group) != OK)
            {
            printErr ("Can't add '%s' to symbol table.\n", symName);
            *pSymAddr = NULL;
            return (ERROR);
            }
        }

    return (OK);
    }

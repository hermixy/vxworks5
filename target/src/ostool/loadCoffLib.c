/* loadCoffLib.c - UNIX coff object module loader */

/* Copyright 1984-1998 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02o,22may01,h_k  fixed R_RELTHUMB23 for big-endian
02n,08feb99,pcn  Added pai changes: fix MAX_SYS_SYM_LEN (SPR #9028) and added
                 functionality to boot an image with multiple text and / or
                 multiple data sections (SPR #22452).
02m,30nov98,dbt  no longer clear seg.flags<xxx> after loadSegmentAllocate()
                 call. (SPR #23553).
02m,13nov98,cdp  added support for ARM generic Thumb library (ARM_THUMB);
                 merged ARM big-endian support - check object file endianness.
02l,05oct98,pcn  Set to _ALLOC_ALIGN_SIZE the flags field in seg structure
                 (SPR #21836).
02k,02sep98,cym  fix static constructor/multiple text segments (SPR #21809)
02h,10aug98,jgn  fix loadModuleAt problem with bss segment (SPR #21635)
02i,17jul98,pcn  Fixed SPR #21836: alignment mismatch between sections and
                 target.
02h,19jun98,c_c  Ignore STYP_REG relocations.
02g,03mar98,cdp  make bootCoffModule skip bytes between scnhdrs and raw data
		 (fixes loading of ARM stripped kernels).
02f,07nov97,yp   made changes to work with tool-chain fix related to 02d.
02e,24oct97,cdp  add support for THUMB23D relocs and trap THUMB9 and THUMB12.
02d,16oct97,cdp  temporary fix to THUMB23 relocs while waiting tool-chain fix.
02c,13aug97,cdp  added Thumb (ARM7TDMI_T) support;
		 tighten up RELARM26 overflow test;
		 initialise 'bias' in coffSymTab to prevent warning.
02b,04mar97,pad  Prevented relocations associated with STYP_INFO, STYP_NOLOAD
		 and STYP_PAD section to be done. Now refuses STYP_REG
		 sections. Silently ignore sections with NOLOAD bit set and
		 symbols from such sections. coffSecHdrRead() now sets the
		 errno.
02a,04feb97,cdp  Fix ARM non-common relocs, 16-bit transfers, overflow tests.
01z,08jan97,cdp  added ARM support.
01y,31oct96,elp  Replaced symAdd() call by symSAdd() call (symtbls synchro).
01x,02jul96,dbt	 removed all references to ANSI C library (SPR #4321).
		 Updated copyright.
01w,19jun95,jmb  fix memory leak -- BSS wasn't freed and
		 the open object file wasn't closed.
01v,06jan95,yp   we now recognize C_STAT T_NULL symbols for 29k
01u,10dec94,pad  now recompute bss sections base addresses for Am29K also and
		 store then in scnAddr array. 
01t,10dec94,kdl  Merge cleanup - restore fast_load handling (SPR 3852).
01s,03nov94,pad  Fixed bugs, fixed history, added comments, achieved
		 separation of i960 and Am29k architectures.
		 Multiple Text segments is not supported by Am29k architecture.
01r,09sep94,ism  integrated a fix from Japan to allow multiple Text segments
01q,14mar94,pme  fixed nested const/consth relocation carry propagation
	   +tpr  in coffRelSegmentAm29K() (SPR #3074).
		 Added comments about relocation.
		 Removed AM29030 tests around VM_STATE_SET().
01p,16dec93,tpr  fixed address computation bug. 
01o,29oct93,tpr  added carry out management into coffRelSegmentAm29K().
01n,09jul93,pme  added text protection for AM29030.
		 added Am29K family support.
                 Need to be reworked one day to really support .lit sections.
01m,19apr93,wmd+ added flag to check to choose between faster load without
    01nov94,kdl	 bal optimizations, or normal load with bal optimizations.
01l,11mar93,wmd  changed coffSymTab() so that it returns ERROR for undefined
		 symbols correctly (SPR # 2056). Modified to use buffered
		 I/O to speedup loader. Moved call to symFindByValue() in
		 coffRelSegment() to after the check for BAL_ENTRY. Fixed
		 SPR # 2031 - use new style loader flags. Modified coffSymTab()
		 not to load common symbols if NO_SYMBOLS option is passed.
01k,11sep92,ajm  changed OFFSET_MASK to OFFSET24_MASK due to b.out redefines,
		 got rid of b.out.h reference, fixed warnings
01j,28aug92,wmd  changed to allow loader to handle absolute symbols
		 which allows two pass compilation to occur,
		 courtesy of F.Opila at Intel.
01i,31jul92,jmm  changed doc for return value of ldCoffModAtSym
01h,28jul92,jmm  removed ifdefs for I960, now compiles for all arch's.
                 installed moduleLib and unloader support
01g,24jul92,jmm  changed SEG_COFF_INFO back to SEG_INFO
                 forced symFlags to 1 for ldCoffModAtSym()
		 forced return value of for ldCoffModAtSym()
01f,24jul92,rrr  change SEG_INFO to SEG_COFF_INFO to avoid conflict
                 also changed addSegNames to _addSegNames
01e,18jul92,smb  Changed errno.h to errnoLib.h.
01d,17jul92,rrr  ifdef the file to only compile on 960
01c,21may92,wmd  modified per Intel to support multiple loaders.
01b,26jan92,ajm  cleaned up from review
01a,21oct91,ajm  written: some culled from loadLib.c
*/

/*
DESCRIPTION
This library provides an object module loading facility for loading
COFF format object files.
The object files are loaded into memory, relocated properly, their
external references resolved, and their external definitions added to
the system symbol table for use by other modules and from the shell.
Modules may be loaded from any I/O stream.

EXAMPLE
.CS
    fdX = open ("/devX/objFile", READ);
    loadModule (fdX, ALL_SYMBOLS);
    close (fdX);
.CE
This code fragment would load the COFF file "objFile" located on
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

/* LINTLIBRARY */

#include "vxWorks.h"
#include "bootLoadLib.h"
#include "cacheLib.h"
#include "errnoLib.h"
#include "fioLib.h"
#include "ioLib.h"
#include "loadCoffLib.h"
#include "loadLib.h"
#include "memLib.h"
#include "pathLib.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "symLib.h"
#include "symbol.h"	/* for SYM_TYPE typedef */
#include "sysSymTbl.h"
#include "private/vmLibP.h"

/*  VxWorks Symbol Type definitions.
 *
 *  These are the symbol types for symbols stored in the vxWorks symbol table.
 *  These are currently defined in b_out.h as N_xxx (e.g. N_ABS).
 *  They are defined with new macro names so that they are
 *       independent of the object format.
 *  It would probably be better to define these in symLib.h.
 */

#define VX_ABS	2	/* Absolute symbol	*/
#define VX_TEXT	4	/* Text symbol		*/
#define VX_DATA	6	/* Data symbol		*/
#define VX_BSS	8	/* BSS symbol		*/
#define VX_EXT	1	/* External symbol (OR'd in with one of above)	*/

/* fast download */
#define BAL_LOAD 0x80	/* flag indicating download with branch-and-link
			   optimization - results in slower download */
LOCAL int fast_load = TRUE;

/*
 *  DEBUG Stuff
 *  #define DEBUG 
 */


#ifdef DEBUG
int debug = 2;                          /* 2 = printf relocation info */
#endif  /* DEBUG */

#ifdef DEBUG
#define DPRINTF if (debug >= 2) printf
#else
#define DPRINTF noop
void noop() { }
#endif

/* #define DEBUG_MALLOC
 */
#ifdef DEBUG_MALLOC
char *
debugMalloc(nbytes)
    int nbytes;
    {
    printf("Calling malloc(%d)\n", nbytes);
    memShow(0);
    return( (char *) malloc(nbytes));
    }

#define malloc debugMalloc
#endif

/*
 *  Abridged edition of a object file symbol entry SYMENT
 *
 *  This is used to minimize the amount of malloc'd memory.
 *  The abridged edition contains all elements of SYMENT required
 *  to perform relocation.
 */
#ifdef __GNUC__
#pragma align 1			/* tell gcc960 to not pad structures */
#endif

struct symentAbridged {
	long		n_value;	/* value of symbol		*/
	short		n_scnum;	/* section number		*/
	char		n_sclass;	/* storage class		*/
	char		n_numaux;       /* number of auxiliary entries  */
};

#define	SYMENT_A struct symentAbridged
#define	SYMESZ_A sizeof(SYMENT_A)	

#ifdef __GNUC__
#pragma align 0			/* turn off alignment requirement */
#endif

/* 
 *  COFF symbol types and classes as macros
 *
 *   'sym' in the macro defintion must be defined as:   SYMENT *sym;
 */
#define U_SYM_VALUE    n_value

#if (CPU_FAMILY == I960)
#define COFF_EXT(sym) \
    (((sym)->n_sclass == C_EXT) || ((sym)->n_sclass == C_LEAFEXT))
#else /* (CPU_FAMILY == I960) */
#if (CPU_FAMILY == ARM)
#define COFF_EXT(sym) \
    (((sym)->n_sclass == C_EXT) || \
     ((sym)->n_sclass == C_THUMBEXT) || \
     ((sym)->n_sclass == C_THUMBEXTFUNC))
#else /* (CPU_FAMILY == ARM) */
#define COFF_EXT(sym) \
    ((sym)->n_sclass == C_EXT)
#endif /* (CPU_FAMILY == ARM) */
#endif /* (CPU_FAMILY == I960) */

/*  COFF Undefined External Symbol */
#define COFF_UNDF(sym) \
    (((sym)->n_sclass == C_EXT) && ((sym)->n_scnum == N_UNDEF))

/*  COFF Common Symbol
 *    A common symbol type is denoted by undefined external
 *    with its value set to non-zero.
 */
#define COFF_COMM(sym) ((COFF_UNDF(sym)) && ((sym)->n_value != 0))

/* COFF Unassigned type
 *    An unassigned type symbol of class C_STAT generated by
 *    some assemblers
 */
#define COFF_UNASSIGNED(sym) (((sym)->n_sclass == C_STAT) && ((sym)->n_type == T_NULL))


/* misc defines */

#define INSERT_HWORD(WORD,HWORD)        \
    (((WORD) & 0xff00ff00) | (((HWORD) & 0xff00) << 8) | ((HWORD)& 0xff))
#define EXTRACT_HWORD(WORD) \
    (((WORD) & 0x00ff0000) >> 8) | ((WORD)& 0xff)
#define SIGN_EXTEND_HWORD(HWORD) \
    ((HWORD) & 0x8000 ? (HWORD)|0xffff0000 : (HWORD))

#define SWAB_SHORT(s) \
  ((((unsigned short) s & 0x00ff) << 8) | (((unsigned short) s & 0xff00) >> 8))

#define MAX_SCNS        11              /* max sections supported */
#define NO_SCNS         0

#if (CPU_FAMILY == I960)
#define MAX_ALIGNMENT	16              /* max boundary for architecture */
#else /* (CPU_FAMILY == I960) */
#define MAX_ALIGNMENT	4		/* max boundary for architecture */
#endif /* (CPU_FAMILY == I960) */

#if (CPU_FAMILY == AM29XXX) || (CPU_FAMILY == ARM)
LOCAL int textSize;			/* size of the "real" text not */
					/* including the .lit section size */
					/* needed by the 29K coff files */
#endif /* (CPU_FAMILY == AM29XXX) || (CPU_FAMILY == ARM) */
char *addrScn[MAX_SCNS];                /* where each section is located */
LOCAL int  max_scns = MAX_SCNS;		/* load with MAX */

LOCAL char stringMemErrMsg [] =
    "loadLib error: insufficient memory for strings table (need %d bytes).\n";
LOCAL char extMemErrMsg [] =
    "loadLib error: insufficient memory for externals table (need %d bytes).\n";
LOCAL char cantConvSymbolType [] =
    "loadLib error: can't convert '%s' symbol type - error = %#x.\n";
LOCAL char cantAddSymErrMsg [] =
    "loadLib error: can't add '%s' to system symbol table - error = %#x.\n";
#ifndef MULTIPLE_LOADERS
LOCAL char fileTypeUnsupported [] =
    "loadLib error: File type with magic number %#x is not supported.\n";
#endif
#if (CPU_FAMILY == I960)
LOCAL char symNotFoundErrMsg [] =
    "loadLib error: trying to relocate to non-existent symbol\n";
#endif /* (CPU_FAMILY == I960) */
#if (CPU_FAMILY == AM29XXX)
LOCAL char outOfRangeErrMsg [] =
    "loadLib error: relative address out of range\n";
LOCAL char hiHalfErrMsg [] =
    "loadLib error: IHIHALF missing in module\n";
#endif /* (CPU_FAMILY == AM29XXX) */
#if (CPU_FAMILY == ARM)
LOCAL int sprFixTextOff[100]; /* SPR 21809 */
LOCAL char outOfRangeErrMsg [] =
    "loadLib error: relative address out of range\n";
LOCAL char symNotFoundErrMsg [] =
    "loadLib error: trying to relocate to non-existent symbol\n";
LOCAL char incorrectEndiannessErrMsg [] =
    "loadLib error: incorrect endianness for target\n";
#endif  /* (CPU_FAMILY == ARM) */


/* forward declarations */

LOCAL void coffFree ();
LOCAL void coffSegSizes ();
LOCAL STATUS coffReadSym ();
#if (CPU_FAMILY == AM29XXX)
LOCAL STATUS coffRelSegmentAm29K ();
#endif /* (CPU_FAMILY == AM29XXX) */
#if (CPU_FAMILY == I960)
LOCAL STATUS coffRelSegmentI960 ();
#endif /* (CPU_FAMILY == I960) */
#if (CPU_FAMILY == ARM)
LOCAL STATUS coffRelSegmentArm ();
#endif  /* (CPU_FAMILY == ARM) */
LOCAL STATUS softSeek ();
LOCAL STATUS fileRead ();
LOCAL STATUS swabData ();
LOCAL void swabLong ();
LOCAL ULONG coffTotalCommons ();
LOCAL ULONG dataAlign ();
LOCAL STATUS coffHdrRead ();
LOCAL STATUS coffOpthdrRead ();
LOCAL STATUS coffSecHdrRead ();
LOCAL STATUS coffLoadSections ();
LOCAL STATUS coffReadRelocInfo ();
LOCAL STATUS coffReadExtSyms ();
LOCAL STATUS coffReadExtStrings ();
LOCAL STATUS coffSymTab (int fd, struct filehdr *pHdr, SYMENT *pSymbols, 
			 char ** *externals,
                         int symFlag, SEG_INFO *pSeg, char *symStrings,
                         SYMTAB_ID symTbl, char *pNextCommon,
                         struct scnhdr *pScnHdrArray, char **pScnAddr,
			 BOOL symsAbsolute, BOOL swapTables, UINT16 group);

/******************************************************************************
*
* loadCoffInit - initialize the system for coff load modules.
*
* This routine initializes VxWorks to use a COFF format for loading 
* modules.
*
* RETURNS: OK
*
* SEE ALSO: loadModuleAt()
*/

STATUS loadCoffInit
    (
    )
    {
    /* XXX check for installed? */
    loadRoutine = (FUNCPTR) ldCoffModAtSym;
    return (OK);
    }

/******************************************************************************
*
* ldCoffModAtSym - load object module into memory with specified symbol table
*
* This routine is the underlying routine to loadModuleAtSum().  This interface
* allows specification of the the symbol table used to resolve undefined
* external references and to which to add new symbols.
*
* RETURNS:
* MODULE_ID, or
* NULL if can't read file or not enough memory or illegal file format
*
* NOMANUAL
*/

MODULE_ID ldCoffModAtSym
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
    SYMTAB_ID symTbl	/* symbol table to use */
    )
    {
    char *pText = (ppText == NULL) ? LD_NO_ADDRESS : *ppText;
    char *pData = (ppData == NULL) ? LD_NO_ADDRESS : *ppData;
    char *pBss  = (ppBss  == NULL) ? LD_NO_ADDRESS : *ppBss;
    char **externalsBuf = NULL;         /* array of symbol absolute values */
    char *externalSyms = NULL;          /* buffer for object file symbols */
    char *stringsBuf = NULL;		/* string table pointer */
    FILHDR hdr;				/* module's COFF header */
    AOUTHDR optHdr;             	/* module's COFF optional header */
    SCNHDR scnHdr[MAX_SCNS];		/* module's COFF section headers */
    char *scnAddr[MAX_SCNS];            /* array of section loaded address */
    RELOC  *relsPtr[MAX_SCNS]; 		/* section relocation */
    SEG_INFO seg;			/* file segment info */
    BOOL tablesAreLE;			/* boolean for table sex */
    BOOL symsAbsolute = FALSE;          /* TRUE if already absolutely located*/
    int status;				/* return value */
    int ix;				/* temp counter */
    int nbytes;				/* temp counter */
    char *pCommons;                     /* start of common symbols in bss */
    char *addrBss;
    char        fileName[255];
    UINT16      group;
    MODULE_ID   moduleId;

    /* Initialization */

    memset ((void *)&seg, 0, sizeof (seg));

#if (CPU_FAMILY == I960)
    /* check for fast load */

    if ((symFlag & BAL_LOAD) || (symFlag == NO_SYMBOLS))
	{
	fast_load = FALSE;
	if (symFlag != NO_SYMBOLS)
	    symFlag &= ~BAL_LOAD;
	}
    else
        fast_load = TRUE;
#endif

    /* Set up the module */

    ioctl (fd, FIOGETNAME, (int) fileName);
    moduleId = loadModuleGet (fileName, MODULE_ECOFF, &symFlag);

    if (moduleId == NULL)
        return (NULL);

    group = moduleId->group;

    /* init section pointers to NULL */

    for (ix = 0; ix < MAX_SCNS; ix++)
	{
	scnAddr[ix] = NULL;
	relsPtr[ix] = NULL;
	bzero((char *) &scnHdr[ix], (int) sizeof(SCNHDR));
	}

    /* read object module header */

    if (coffHdrRead (fd, &hdr, &tablesAreLE) != OK)
	{
	/*  Preserve errno value from subroutine. */
	goto error;
	}

    /* read in optional header */

    if (hdr.f_opthdr)                   /* if there is an optional header */
        {
        if (coffOpthdrRead (fd, &optHdr, tablesAreLE) != OK)
            {
            errnoSet(S_loadLib_OPTHDR_READ);
            goto error;
            }
        }

    /* read in section headers */

    if (coffSecHdrRead (fd, &scnHdr[0], &hdr, tablesAreLE) != OK)
	{
	errnoSet(S_loadLib_SCNHDR_READ);
	goto error;
	}

    /* Determine segment sizes */

    /* 
     * XXX seg.flagsXxx mustn't be used between coffSegSizes() and 
     * loadSegmentsAllocate().
     */

    coffSegSizes(&hdr, scnHdr, &seg);

    /*  If object file is already absolutely located (by linker on host),
     *  then override parameters pText, pData and pBss.
     */
    if (   (hdr.f_flags & F_EXEC)
        && (hdr.f_flags & F_RELFLG))
        {
        if (hdr.f_opthdr)               /* if there is an optional header */
            {
            symsAbsolute = TRUE;
            pText = (INT8 *)(optHdr.text_start);
            pData = (INT8 *)(optHdr.data_start);

            /* bss follows data segment */
            pBss = (INT8 *) pData + optHdr.dsize;
            pBss += dataAlign(MAX_ALIGNMENT, pBss);
	    }
	}

    seg.addrText = pText;
    seg.addrData = pData;

    /*
     * If pBss is set to LD_NO_ADDRESS, change it to something else.
     * The coff loader allocates one large BSS segment later on, and
     * loadSegmentsAllocate doesn't work correctly for it.
     */

    seg.addrBss = (pBss == LD_NO_ADDRESS ? LD_NO_ADDRESS + 1 : pBss);

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
        }

    /*  Ensure that section starts on the appropriate alignment.
     */
    pText += dataAlign(MAX_ALIGNMENT, pText);
    if (pData == LD_NO_ADDRESS)
        {
	pData = pText + seg.sizeText;
        }

    /*  Ensure that section starts on the appropriate alignment.
     */
    pData += dataAlign(MAX_ALIGNMENT, pData);

    seg.addrText = pText;
    seg.addrData = pData;

    /* load text and data sections */

    if (coffLoadSections (fd, &scnHdr[0], &scnAddr[0], pText, pData) != OK)
	{
	errnoSet (S_loadLib_LOAD_SECTIONS);
	goto error;
	}

    /* get section relocation info */

    if (coffReadRelocInfo (fd, &scnHdr[0], &relsPtr[0], tablesAreLE) != OK)
	{
	errnoSet (S_loadLib_RELOC_READ);
	goto error;
	}

    /* read symbols */

    if (coffReadExtSyms (fd, &externalSyms, &externalsBuf, &hdr, tablesAreLE)
        != OK)
	{
	errnoSet (S_loadLib_EXTSYM_READ);
	goto error;
	}

    /* read string table */

    if (coffReadExtStrings (fd, &stringsBuf, tablesAreLE) != OK)
	{
	errnoSet (S_loadLib_EXTSTR_READ);
	goto error;
	}

    /*  Determine additional amount of memory required to append
     *  common symbols on to the end of the BSS section.
     */
    nbytes = coffTotalCommons(externalSyms, hdr.f_nsyms, seg.sizeBss);

    /* set up for bss */

    seg.sizeBss += nbytes;

    if (pBss == LD_NO_ADDRESS)
        {
        if (seg.sizeBss != 0)
            {
            if ((pBss = malloc (seg.sizeBss)) == NULL) 
                goto error;
            else                                 
		seg.flagsBss |= SEG_FREE_MEMORY;  
            }
        else
            {
            pBss = (char *) ((long) pData + (long) seg.sizeData);
            }
        }

    pBss += dataAlign(MAX_ALIGNMENT, pBss);
    seg.addrBss = pBss;

    /* fix up start address of bss sections */

    addrBss = pBss;
    for (ix = 0; ix < max_scns; ix++)
        {
        if (scnHdr[ix].s_flags & STYP_BSS)
	    {
#if (CPU_FAMILY == I960) 
	    addrBss += dataAlign (scnHdr[ix].s_align, addrBss);
#else /* (CPU_FAMILY == I960) */
	    addrBss += dataAlign (MAX_ALIGNMENT, addrBss);
#endif /* (CPU_FAMILY == I960) */
	    scnAddr[ix] = addrBss;
	    addrBss += scnHdr[ix].s_size;
            }
        }
	    
    /* set up address for first common symbol */

    pCommons = (char *) ((long) seg.addrBss + (seg.sizeBss - nbytes));

    /* add segment names to symbol table before other symbols */

    if (symFlag != LOAD_NO_SYMBOLS)
        addSegNames (fd, pText, pData, pBss, symTbl, group);

    /* process symbol table */

    status = coffSymTab (fd, &hdr, (SYMENT *)externalSyms, &externalsBuf, 
			 symFlag, &seg, stringsBuf, symTbl, pCommons, scnHdr,
			 scnAddr, symsAbsolute, tablesAreLE, group);

    /* relocate text and data segments
     *
     *   note: even if symbol table had missing symbols, continue with
     *   relocation of those symbols that were found.
     *   note: relocation is for changing the values of the relocated
     *   symbols.  bss is uninitialized data, so it is not relocated
     *   in the symbol table.
     */

    if ((status == OK) || (errnoGet() == S_symLib_SYMBOL_NOT_FOUND))
        for (ix = 0; ix < max_scns; ix++)
	    {
	    if ((relsPtr[ix] != NULL)			&&
		!(scnHdr[ix].s_flags & STYP_INFO)	&&
		!(scnHdr[ix].s_flags & STYP_NOLOAD)	&&
		 (scnHdr[ix].s_flags != STYP_REG)	&& 
		!(scnHdr[ix].s_flags & STYP_PAD))
		{
#if (CPU_FAMILY == ARM)
                if (coffRelSegmentArm(relsPtr[ix], scnHdr, externalsBuf,
                               externalSyms, &seg, ix, symTbl) != OK)
#else
#if (CPU_FAMILY == AM29XXX)
                if (coffRelSegmentAm29K(relsPtr[ix], scnHdr, externalsBuf, 
                               externalSyms, &seg, ix, symTbl) != OK)
#else /* (CPU_FAMILY == I960) */
                if (coffRelSegmentI960(relsPtr[ix], scnHdr, scnAddr[ix],
				       externalsBuf, externalSyms, &seg, ix,
				       symTbl) != OK)
#endif /* (CPU_FAMILY == AM29XXX) */
#endif /* (CPU_FAMILY == ARM) */
		    {
	            goto error;
		    }
		}
	    }

    coffFree(externalsBuf, externalSyms, stringsBuf, relsPtr);

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
     * clean up dynamically allocated temporary buffers and return ERROR
     */
error:
    coffFree(externalsBuf, externalSyms, stringsBuf, relsPtr);
    moduleDelete (moduleId);
    return (NULL);
    }

/*******************************************************************************
*
* coffFree - free any malloc'd memory
*
*/

LOCAL void coffFree
    (
    char **pExternalsBuf,       /* pointers to external symbols */
    char *pExternalSyms,        /* buffer for external symbols */
    char *pStringsBuf,		/* string table pointer */
    RELOC  **pRelsPtr 		/* section relocation */
    )
    {
    int ix;

    if (pStringsBuf != NULL)
	free (pStringsBuf);
    if (pExternalsBuf != NULL)
	free ((char *) pExternalsBuf);
    if (pExternalSyms != NULL)
	free ((char *) pExternalSyms);

    for (ix = 0; ix < MAX_SCNS; ix++)
	{
	if (*pRelsPtr != NULL)
	    free((char *) *pRelsPtr);
        pRelsPtr++;
	}
    }


/*******************************************************************************
*
* coffSegSizes - determine segment sizes
*
* This function fills in the size fields in the SEG_INFO structure.
* Note that the bss size may need to be readjusted for common symbols. 
*/

LOCAL void coffSegSizes
    (
    FILHDR *pHdr,               /* pointer to file header */
    SCNHDR *pScnHdrArray,       /* pointer to array of section headers */
    SEG_INFO *pSeg	       /* section addresses and sizes */
    )
    {
    int ix;
    int nbytes;             /* additional bytes required for alignment */
    SCNHDR *pScnHdr;            /* pointer to a section header */

    pSeg->sizeText = 0;
    pSeg->sizeData = 0;
    pSeg->sizeBss = 0;

    /* 
     * SPR #21836: pSeg->flagsText, pSeg->flagsData, pSeg->flagsBss are used
     * to save the max value of each segments. These max values are computed
     * for each sections. These fields of pSeg are only used on output, then
     * a temporary use is allowed.
     */

    pSeg->flagsText = _ALLOC_ALIGN_SIZE;
    pSeg->flagsData = _ALLOC_ALIGN_SIZE;
    pSeg->flagsBss  = _ALLOC_ALIGN_SIZE;

    for (ix=0; ix < pHdr->f_nscns; ix++) /* loop thru all sections */
        {
        pScnHdr = pScnHdrArray + ix;
        if (pScnHdr->s_flags & STYP_TEXT)
            {
            /*  Assume that there was a previous section of same type.
             *  First, align data to boundary from section header.
             *  Add the size of this section to the total segment size.
             */

#if (CPU_FAMILY == ARM)
            sprFixTextOff[ix] = pSeg->sizeText;
#endif
#if (CPU_FAMILY == I960)
            nbytes = dataAlign(pScnHdr->s_align, pSeg->sizeText);

	    /* SPR #21836 */

	    if (pScnHdr->s_align > pSeg->flagsText)
		pSeg->flagsText = pScnHdr->s_align;

#else /* (CPU_FAMILY == I960) */
            nbytes = dataAlign(MAX_ALIGNMENT, pSeg->sizeText);

	    /* SPR #21836 */

	    if (MAX_ALIGNMENT > pSeg->flagsText)
		pSeg->flagsText = MAX_ALIGNMENT;

#endif /* (CPU_FAMILY == I960) */

            pSeg->sizeText += pScnHdr->s_size + nbytes;
            }
        else if (pScnHdr->s_flags & STYP_DATA)
            {
#if (CPU_FAMILY == I960)
            nbytes = dataAlign(pScnHdr->s_align, pSeg->sizeData);

	    /* SPR #21836 */

	    if (pScnHdr->s_align >  pSeg->flagsData)
		pSeg->flagsData = pScnHdr->s_align;

#else /* (CPU_FAMILY == I960) */
            nbytes = dataAlign(MAX_ALIGNMENT, pSeg->sizeData);

	    /* SPR #21836 */

	    if (MAX_ALIGNMENT > pSeg->flagsData)
		pSeg->flagsData = MAX_ALIGNMENT;

#endif /* (CPU_FAMILY == I960) */

            pSeg->sizeData += pScnHdr->s_size + nbytes;
            }
        else if (pScnHdr->s_flags & STYP_BSS)
            {
#if (CPU_FAMILY == I960)
            nbytes = dataAlign(pScnHdr->s_align, pSeg->sizeBss);

	    /* SPR #21836 */

	    if (pScnHdr->s_align > pSeg->flagsBss)
		pSeg->flagsBss = pScnHdr->s_align;

#else /* (CPU_FAMILY == I960) */
            nbytes = dataAlign(MAX_ALIGNMENT, pSeg->sizeBss);

	    /* SPR #21836 */

	    if (MAX_ALIGNMENT > pSeg->flagsBss)
		pSeg->flagsBss = MAX_ALIGNMENT;

#endif /* (CPU_FAMILY == I960) */

            pSeg->sizeBss += pScnHdr->s_size + nbytes;
            }
        }
    }

/*******************************************************************************
*
* coffLoadSections - read sections into memory
* 
*/

LOCAL STATUS coffLoadSections
    (
    int     fd,
    SCNHDR  *pScnHdr,
    char    **pScnAddr,
    char    *pText,
    char    *pData
    )
    {
    int     ix;
    int     textCount = 0;
    int     dataCount = 0;
    INT32   size;               /* section size */
    int     nbytes;             /* additional bytes required for alignment */
    char    *pLoad;             /* address to load data at */

#if (CPU_FAMILY == AM29XXX) || (CPU_FAMILY == ARM)
    textSize = 0;		/* clear "real" text size */
#endif /* (CPU_FAMILY == AM29XXX) || (CPU_FAMILY == ARM) */

    for (ix = 0; ix < max_scns; ix++)
	{
        pLoad = NULL;
        size = pScnHdr->s_size;
#ifdef DEBUG
        DPRINTF("sec size=%#x sec ptr=%#x\n", size, pScnHdr->s_scnptr);
#endif  /* DEBUG */
	if (size != 0 && pScnHdr->s_scnptr != 0)
	    {
            if (pScnHdr->s_flags & STYP_TEXT)
                {
#if (CPU_FAMILY == AM29XXX) || (CPU_FAMILY == ARM)
    	        /* 
		 * if section is not .lit add its size to the "real" 
		 * text size. Note that this is a hack a better way 
		 * should be to add a pLit parameter to coffLoadSections().
		 *
		 * Note: ARM rdata sections have both _LIT and _TEXT flags set.
		 */
	        if (!(pScnHdr->s_flags & STYP_LIT))
    	    	    textSize += size;
#endif /* (CPU_FAMILY == AM29XXX) */

                pLoad = pText;
#if (CPU_FAMILY == I960) 
                nbytes = dataAlign(pScnHdr->s_align, pLoad);
#else /* (CPU_FAMILY == I960) */
                nbytes = dataAlign(MAX_ALIGNMENT, pLoad);
#endif /* (CPU_FAMILY == I960) */
                pLoad += nbytes;
                pText = (char *) ((int) pLoad + size); /* for next load */
                textCount += size + nbytes;
                }
            else if (pScnHdr->s_flags & STYP_DATA)
                {
                pLoad = pData;
#if (CPU_FAMILY == I960) 
                nbytes = dataAlign(pScnHdr->s_align, pLoad);
#else /* (CPU_FAMILY == I960) */
                nbytes = dataAlign(MAX_ALIGNMENT, pLoad);
#endif /* (CPU_FAMILY == I960) */
                pLoad += nbytes;
                pData = (char *) ((int) pLoad + size); /* for next load */
                dataCount += size + nbytes;
                }
            else
                {
#if (CPU_FAMILY == AM29XXX) || (CPU_FAMILY == ARM)
		goto skipSection;
#else
                /* ignore all other sections */
		continue;
#endif /* (CPU_FAMILY == AM29XXX) || (CPU_FAMILY == ARM) */
                }

            /*  Advance to position in file
             *  and read section directly into memory.
             */
	    if ((lseek (fd, pScnHdr->s_scnptr, SEEK_SET) == ERROR) ||
		(fioRead (fd, pLoad, size) != size))
		{
		return (ERROR);
		}
	    }
#if (CPU_FAMILY == AM29XXX) || (CPU_FAMILY == ARM)
skipSection:
#endif /* (CPU_FAMILY == AM29XXX) || (CPU_FAMILY == ARM) */
        pScnAddr[ix] = pLoad;
	pScnHdr++;
	}
    return (OK);
    }

/*******************************************************************************
*
* coffSymTab -
*
* This is passed a pointer to a coff symbol table and processes
* each of the external symbols defined therein.  This processing performs
* two functions:
*
*  1) Defined symbols are entered in the system symbol table as
*     specified by the "symFlag" argument:
*	LOAD_ALL_SYMBOLS = all defined symbols (LOCAL and GLOBAL) are added,
*	LOAD_GLOBAL_SYMBOLS = only external (GLOBAL) symbols are added,
*	LOAD_NO_SYMBOLS	= no symbols are added;
*
*  2) Any undefined externals are looked up in the system symbol table to
*     determine their values.  The values are entered into the specified
*     'externals' array.  This array is indexed by the symbol number
*     (position in the symbol table).
*     Note that all symbol values, not just undefined externals, are entered
*     into the 'externals' array.  The values are used to perform relocations.
*
*     Note that "common" symbols have type undefined external - the value
*     field of the symbol will be non-zero for type common, indicating
*     the size of the object.
*
* If an undefined external cannot be found in the system symbol table,
* an error message is printed, but ERROR is not returned until the entire
* symbol table has been read, which allows all undefined externals to be
* looked up.
*
* RETURNS: OK or ERROR
*/

LOCAL STATUS coffSymTab
    (
    int fd,
    FILHDR *pHdr,               /* pointer to file header */
    SYMENT *externalSyms,	/* pointer to symbols array */
    char ***externals,		/* pointer to pointer to array to fill in 
				   symbol absolute values */
    int symFlag,		/* symbols to be added to table 
				 *   ([NO|GLOBAL|ALL]_SYMBOLS) */
    SEG_INFO *pSeg,		/* pointer to segment information */
    char *symStrings,		/* symbol string table */
    SYMTAB_ID symTbl,		/* symbol table to use */
    char *pNextCommon,		/* next common address in bss */
    SCNHDR *pScnHdrArray,	/* pointer to COFF section header array */
    char **pScnAddr,		/* pointer to section loaded address array */
    BOOL symsAbsolute,          /* TRUE if symbols already absolutely located*/
    BOOL    swapTables,
    UINT16 group
    )
    {
    int status  = OK;		/* return status */
    FAST char *name;		/* symbol name (plus EOS) */
    SYMENT *symbol;		/* symbol struct */
    SYM_TYPE vxSymType;		/* vxWorks symbol type */
    int symNum;			/* symbol index */
    char *adrs;			/* table resident symbol address */
    char *bias = 0;		/* section relative address */
    ULONG bssAlignment;		/* alignment value of common type */
    long scnFlags;              /* flags from section header */
    int auxEnts = 0;            /* auxiliary symbol entries to skip */
    SCNHDR *pScnHdr = NULL;	/* pointer to a COFF section header */
    char *scnAddr = NULL;	/* section loaded address */
    char nameTemp[SYMNMLEN+1];  /* temporary for symbol name string */


    /*  Loop thru all symbol table entries in object file
     */
    for (symNum = 0, symbol = externalSyms; symNum < pHdr->f_nsyms; 
	 symbol++, symNum++)
	{
        if (auxEnts)                    /* if this is an auxiliary entry */
            {
            auxEnts--;
            continue;                   /* skip it */
            }
        auxEnts = symbol->n_numaux;      /* # of aux entries for this symbol */

        if (symbol->n_scnum == N_DEBUG)
            continue;

	/* Setup pointer to symbol name string
	 */
        if (symbol->n_zeroes)            /* if name is in symbol entry */
            {
            name = symbol->n_name;
            if ( *(name + SYMNMLEN -1) != '\0')
                {
                /*  If the symbol name array is full,
                 *  the string is not terminated by a null.
                 *  So, move the string to a temporary array,
                 *  where it can be null terminated.
                 */
                bcopy(name, nameTemp, SYMNMLEN);
                nameTemp[SYMNMLEN] = '\0';
                name = nameTemp;
                }
            }
        else
            name = symStrings + symbol->n_offset;


        if (   ! COFF_UNDF(symbol)
            && ! COFF_COMM(symbol))
            {
            /*  Symbol is not an undefined external.
             *
             *  Determine symbol's section and address bias
             */
            if (symbol->n_scnum > 0)
                {
                pScnHdr = pScnHdrArray + (symbol->n_scnum - 1);
		scnAddr = pScnAddr[symbol->n_scnum - 1];
                scnFlags = pScnHdr->s_flags;
                }
            else
                {
                scnFlags = 0;       /* section has no text, data or bss */
                }

            if (symbol->n_scnum == N_ABS) /* special check for absolute syms */
                {
                bias = 0;
                vxSymType = VX_ABS;
                }
	    else if (scnFlags == STYP_REG)
		{
		printf ("Symbol %s from section of type STYP_REG. Ignored.\n",
			name);
		}
#if (CPU_FAMILY == AM29XXX) || (CPU_FAMILY == ARM) 
            else if ((scnFlags & STYP_TEXT) && !(scnFlags & STYP_LIT))
#else
            else if (scnFlags & STYP_TEXT)
#endif /* (CPU_FAMILY == AM29XXX) || (CPU_FAMILY == ARM) */
                {
#if (CPU_FAMILY == ARM)
                bias = (char *) (scnAddr + sprFixTextOff[symbol->n_scnum - 1] - 
		    (char *) pScnHdr->s_vaddr);
#else
                bias = (char *) (scnAddr - (char *) pScnHdr->s_vaddr);
#endif /* (CPU_FAMILY == ARM) */
                vxSymType = VX_TEXT;
                }
            else if (scnFlags & STYP_DATA)
                {
                bias = (char *) (scnAddr - (char *) pScnHdr->s_vaddr);
                vxSymType = VX_DATA;
                }
            else if (scnFlags & STYP_BSS)
                {
                bias = (char *) (scnAddr - (char *) pScnHdr->s_vaddr);
                vxSymType = VX_BSS;
                }

	    /* If it has the NOLOAD or INFO bit set, ignore it silently */

	    else if ((scnFlags & STYP_NOLOAD) ||
		     (scnFlags & STYP_INFO))
		{
		continue;
		}
#if (CPU_FAMILY == AM29XXX) || (CPU_FAMILY == ARM)
	    /* 
	     * If section is .lit it will be located just behind the text
	     * section. We use the global variable textSize to determine
	     * the end of the "real" text. A better solution would be to
	     * add fieds for the .lit section in the SEG_INFO structure.
	     * Note that the following test is always false for the i960
	     * architecture.
	     */
            else if (scnFlags & STYP_LIT)
                {
#if (CPU_FAMILY == ARM)
                bias = (char *)((char *) pSeg->addrText + 
			sprFixTextOff[symbol->n_scnum - 1] - 
			(char *)pScnHdr->s_vaddr);
#else
                bias = (char *)((char *) pSeg->addrText + textSize -
				(char *)pScnHdr->s_vaddr);
#endif /* (CPU_FAMILY == ARM) */
                vxSymType = VX_TEXT;
                }
	    else if (COFF_UNASSIGNED(symbol))
                continue;
#endif /* (CPU_FAMILY == AM29XXX) || (CPU_FAMILY == ARM) */
            else
                {
                printErr (cantConvSymbolType, name, errnoGet());
                continue;
                }

            /*  If object file is already absolutely located
             *  (by linker on host),
             *  then the symbol values are already correct.
             *  There is no need to bias them. 
             */
            if (symsAbsolute)
                {
                bias = 0x00;
                }

#ifdef DEBUG
	DPRINTF("symbol=%#x bias=%#x vaddr=%#x type=%#x ", (int)symbol, (int)bias, 
		(int)pScnHdr->s_vaddr, vxSymType);
#endif  /* DEBUG */

            /*  Determine if symbol should be put into symbol table.
             */
            if (((symFlag & LOAD_LOCAL_SYMBOLS) && !COFF_EXT(symbol))
                || ((symFlag & LOAD_GLOBAL_SYMBOLS) && COFF_EXT(symbol)))
                {
                if (COFF_EXT(symbol))
                    vxSymType |= VX_EXT;
#if (CPU_FAMILY == I960)
                if (   (symbol->n_sclass == C_LEAFEXT)
                    || (symbol->n_sclass == C_LEAFSTAT))
                    {
                    vxSymType |= BAL_ENTRY; /* can call with branch & link */
                    }
#endif /* (CPU_FAMILY == I960) */

                /*  Add symbol to symbol table.
                 */

#if ((CPU_FAMILY == ARM) && ARM_THUMB)
		/*
		 * Make sure we flag any functions so that relocator knows
		 * to set bit zero
		 */
 
		if ((scnFlags & STYP_TEXT) &&
		    ((unsigned char)symbol->n_sclass == C_THUMBEXTFUNC ||
		     (unsigned char)symbol->n_sclass == C_THUMBSTATFUNC))
		    {
		    vxSymType |= SYM_THUMB;
		    }
#endif /* CPU_FAMILY == ARM */

		if (symSAdd (symTbl, name, symbol->U_SYM_VALUE + bias,
			     vxSymType, group) != OK)
		    printErr (cantAddSymErrMsg, name, errnoGet());
                }

#if ((CPU_FAMILY == ARM) && ARM_THUMB)
	    /*
	     * Make sure we set bit zero of the addresses of any static
	     * functions
	     */

	    if ((scnFlags & STYP_TEXT) &&
		((unsigned char)symbol->n_sclass == C_THUMBEXTFUNC ||
		 (unsigned char)symbol->n_sclass == C_THUMBSTATFUNC))
		{
		bias = (char *)((UINT32)bias | 1);
		}
#endif /* CPU_FAMILY == ARM */

            /* Add symbol address to externals table.
             * For COFF, we add all symbol addresses into the externals
             * table, not only those symbols saved in the vxWorks symbol
             * table.
             */

            (*externals) [symNum] = symbol->U_SYM_VALUE + bias;
#ifdef DEBUG
	DPRINTF("new symbol=%#x \n", (unsigned) (symbol->U_SYM_VALUE + bias));
#endif  /* DEBUG */
	    }
	else
	    {
	    /* Undefined external symbol or "common" symbol
             *
	     *   A common symbol type is denoted by undefined external
             *   with its value set to non-zero.
	     */

	    /* if symbol is a common, then allocate space and add to
	     * symbol table as BSS
	     */

	    /* common symbol - create new symbol */

            if (COFF_COMM(symbol))
		{
		if (symFlag == LOAD_NO_SYMBOLS)
		    ;
		else
		    {
		    /* 
		     *  common symbol - create new symbol 
		     *
		     *  Common symbols are now tacked to the end of the bss
		     *  section.  This is done to accomodate systems that have
		     *  multiple boards sharing memory with common symbols
		     *  over the system bus.  
		     *  This portion of code reads the symbol value
		     *  (which contains the symbol size) and places the symbol
		     *  in the bss section.  The function dataAlign uses
		     *  the next possible address for a common symbol to determine
		     *  the proper alignment.
		     */
		    
		    adrs = pNextCommon;
		    bssAlignment = dataAlign (symbol->U_SYM_VALUE, (ULONG) adrs);
		    adrs += bssAlignment;
		    pNextCommon += (symbol->U_SYM_VALUE + bssAlignment);
		    
		    if (symSAdd (symTbl, name, adrs, (VX_BSS | VX_EXT), group) != OK)
		    printErr (cantAddSymErrMsg, name, errnoGet());
		    
		    }
		}

	    /* look up undefined external symbol in symbol table */

	    else if (symFindByNameAndType (symTbl, name, &adrs, &vxSymType,
					   VX_EXT, VX_EXT) != OK)
                {
                /* symbol not found in symbol table */

                printErr ("undefined symbol: %s\n", name);
                adrs = NULL;
                status = ERROR;
                }

	    /* add symbol address to externals table
             */
	    (*externals) [symNum] = adrs;
	    }
	}
    return (status);
    }
#if (CPU_FAMILY == AM29XXX)
/*******************************************************************************
*
* CoffRelSegmentAm29K - perform relocation for the Am29K family
*
* This routine reads the specified relocation command segment and performs
* all the relocations specified therein.
* Absolute symbol addresses are looked up in the 'externals' table.
*
* This function handles the following types of relocation commands
* for the Am29K family of processor:
*
* 	- R_IREL	instruction relative (jmp/call)
* 	- R_IABS	instruction absolute (jmp/call)
* 	- R_ILOHALF	instruction low half  (const)
* 	- R_IHIHALF	instruction high half (consth) part 1
* 	- R_IHCONST	instruction high half (consth) part 2
*			constant offset of R_IHIHALF relocation
* 	- R_BYTE	relocatable byte value
* 	- R_HWORD	relocatable halfword value
* 	- R_WORD	relocatable word value
*
* 	- R_IGLBLRC	instruction global register RC
* 	- R_IGLBLRA	instruction global register RA
* 	- R_IGLBLRB	instruction global register RB
*
*		
* RETURNS: OK or ERROR
* 
* INTERNAL
* Note that we don't support multi-sections management for the Am29k 
* architecture.
*/

LOCAL STATUS coffRelSegmentAm29K
    (
    RELOC *	pRelCmds,	/* list of relocation commands */
    SCNHDR *	pScnHdrArray,	/* array of section headers */
    char **	pExternals,	/* array of absolute symbol values */
    SYMENT *	pExtSyms,	/* pointer to object file symbols */
    SEG_INFO *	pSeg,		/* section addresses and sizes */
    int 	section,	/* section number -1 for relocation */
    SYMTAB_ID 	symTbl		/* symbol table to use */
    )
    {
    long *	pAdrs;		/* relocation address */
    int		nCmds;		/* # reloc commands for seg */
    RELOC 	relocCmd;	/* relocation structure */
    STATUS 	status = OK;
    UINT32	unsignedValue;	/* const and consth values */
    UINT32	symVal;		/* value associated with symbol */
    SYMENT *	pSym;		/* pointer to an external symbol */
    SCNHDR *	pScnHdr;	/* section header for relocation */
    INT32	signedValue;	/* IREL case temporary */
    /*
     * The 29k loader needs to keep track of some values between
     * the IHICONST and ICONST relocation processing. We use static
     * variables here which makes the loader not re-entrant.
     * This should be fixed later.
     */
    static BOOL consthActive;	/* CONSTH processing in progress flag */
    static UINT32 consthValue;	/* CONSTH processing in progress value */

    pScnHdr = pScnHdrArray + section;

    for (nCmds = pScnHdr->s_nreloc; nCmds > 0; nCmds--)
	{
	/* read relocation command */

	relocCmd = *pRelCmds++;

	/* 
	 * Calculate actual address in memory that needs relocation.
	 */

        if (pScnHdr->s_flags & STYP_TEXT)
	    {
	    if (!(pScnHdr->s_flags & STYP_LIT))
		pAdrs = (long *) ((long) pSeg->addrText + relocCmd.r_vaddr);
	    else
	    /* 
	     * The lit section is located after the text section, we take 
	     * care of this by adding the "real" text size to the text address.
	     * A better solution would be to add .lit fields in the 
	     * SEG_INFO structure.
	     */
		pAdrs = (long *) ((long) pSeg->addrText + textSize 
				   + relocCmd.r_vaddr - pScnHdr->s_vaddr);
	    }
	else 
	    {
	    pAdrs = (long *) ((long) pSeg->addrData + 
		(relocCmd.r_vaddr - pScnHdr->s_vaddr));
	    }

#ifdef DEBUG
        DPRINTF("r_vaddr=%02x pAdrs=%08x ", 
                relocCmd.r_vaddr, (int) pAdrs);
#endif  /* DEBUG */

        switch (relocCmd.r_type)
            {

        case R_IABS:
	    /* Nothing to do in this case */
#ifdef DEBUG
            DPRINTF("IABS ");
            DPRINTF("sym=%08x ", (unsigned) pExternals [relocCmd.r_symndx]);
#endif  /* DEBUG */
	    break;

        case R_IREL:
	    /* 
	     * Relative jmp/call: in this case we must calculate the address
	     * of the target using the symbol value and the offset 
	     * found in the instruction.
	     */
#ifdef DEBUG
            DPRINTF("IREL ");
            DPRINTF("sym=%08x ", (unsigned) pExternals [relocCmd.r_symndx]);
#endif  /* DEBUG */
	    signedValue = EXTRACT_HWORD(*pAdrs) << 2;
	    signedValue = SIGN_EXTEND_HWORD(signedValue);
	    signedValue += (unsigned) pExternals [relocCmd.r_symndx];
	    /*
	     * Now if its an absolute jmp/call set the absolute bit (A) of
	     * the instruction.
	     * See gnu compiler sources src/bfd/coff-a29k.c
	     */
	    if ((signedValue &~ 0x3ffff) == 0)
		{
		*pAdrs = *pAdrs | (1 << 24);
		}
	    else
		{
		signedValue -= relocCmd.r_vaddr;
		if (signedValue > 0x1ffff || signedValue < -0x20000)
		    printErr (outOfRangeErrMsg);
		}
	    
	    signedValue = signedValue >> 2;
	    *pAdrs = INSERT_HWORD(*pAdrs, signedValue);
	    break;

	case R_ILOHALF:
	    /*
	     * This is a CONST instruction, put the 16 lower bits of the
	     * symbol value at the right place in the instruction.
	     *
	     * CONST instruction format:  |OP|const bit 15-8|RA|const bit 7-0|
	     */
#ifdef DEBUG
            DPRINTF("ILOHALF ");
            DPRINTF("sym=%08x ", (unsigned) pExternals [relocCmd.r_symndx]);
#endif  /* DEBUG */
    	    pSym = pExtSyms + relocCmd.r_symndx;
#ifdef DEBUG
            DPRINTF("pSym=%08x ", (unsigned) pSym);
#endif  /* DEBUG */
	    unsignedValue = EXTRACT_HWORD(*(UINT32 *)pAdrs);
	    unsignedValue += (unsigned) pExternals [relocCmd.r_symndx];
	    *(UINT32 *)pAdrs = INSERT_HWORD(*(UINT32 *)pAdrs, unsignedValue);
	    break;

	case R_IHIHALF:
	    /*
	     * This is the first stage of a CONSTH instruction relocation, 
	     * just keep track of the fact and keep the value.
	     * We don't modify the instruction until R_IHCONST.
	     *
	     * CONSTH instruction format:  |OP|const bit 15-8|RA|const bit 7-0|
	     */
#ifdef DEBUG
            DPRINTF("IHIHALF ");
            DPRINTF("sym=%08x ", (unsigned) pExternals [relocCmd.r_symndx]);
#endif  /* DEBUG */
	    pSym = pExtSyms + relocCmd.r_symndx;
#ifdef DEBUG
            DPRINTF("pSym=%08x ", (unsigned) pSym);
#endif  /* DEBUG */
	    consthActive = TRUE;
	    consthValue = (unsigned) pExternals [relocCmd.r_symndx];
	    break;

	case R_IHCONST:
	    /*
	     * This is the second stage of a CONSTH instruction relocation, 
	     * put the 16 higher bits of the symbol value at the right place 
	     * in the instruction.
	     *
	     * CONSTH instruction format:  |OP|const bit 15-8|RA|const bit 7-0|
	     */
#ifdef DEBUG
            DPRINTF("IHCONST ");
            DPRINTF("relocOffset=%08x ", relocCmd.r_symndx);
#endif  /* DEBUG */
	    if (!consthActive)
		{
		printErr (hiHalfErrMsg);
		break;
		}

/* 
 * If the parameter to a CONSTH instruction is a relocatable type, two
 * relocation records are written.  The first has an r_type of R_IHIHALF
 * (33 octal) and a normal r_vaddr and r_symndx.  The second relocation
 * record has an r_type of R_IHCONST (34 octal), a normal r_vaddr (which
 * is redundant), and an r_symndx containing the 32-bit constant offset
 * to the relocation instead of the actual symbol table index.  This
 * second record is always written, even if the constant offset is zero.
 * The constant fields of the instruction are set to zero.
 */

            unsignedValue = relocCmd.r_symndx;
            unsignedValue += consthValue ;
	    unsignedValue = unsignedValue >> 16;
	    *(UINT32 *)pAdrs = INSERT_HWORD(*(UINT32 *)pAdrs, unsignedValue);
	    consthActive = FALSE;
	    break;

	case R_BYTE:
	    /* Relocatable byte, just set *pAdrs with byte value. */
#ifdef DEBUG
            DPRINTF("BYTE ");
            DPRINTF("sym=%08x ", (unsigned) pExternals [relocCmd.r_symndx]);
#endif  /* DEBUG */
	    pSym = pExtSyms + relocCmd.r_symndx;
#ifdef DEBUG
            DPRINTF("pSym=%08x *pAdrs %08x", (unsigned)pSym, *(char *)pAdrs);
#endif  /* DEBUG */
	    symVal = (UINT32) pExternals[relocCmd.r_symndx];
    	    *(UINT8 *)pAdrs = *(UINT8*) pAdrs + symVal;
	    break;
		
	case R_HWORD:
	    /* Relocatable half word, just set *pAdrs with half word value. */
#ifdef DEBUG
            DPRINTF("HWORD ");
            DPRINTF("sym=%08x ", (unsigned) pExternals [relocCmd.r_symndx]);
#endif  /* DEBUG */
	    pSym = pExtSyms + relocCmd.r_symndx;
#ifdef DEBUG
            DPRINTF("pSym=%08x *pAdrs %08x", (unsigned)pSym, *(short *)pAdrs);
#endif  /* DEBUG */
	    symVal = (UINT32) pExternals[relocCmd.r_symndx];
    	    *(UINT16 *)pAdrs = *(UINT16*) pAdrs + symVal;
	    break;

	case R_WORD:
	    /* Relocatable word, just add symbol to current address content. */
#ifdef DEBUG
            DPRINTF("WORD ");
            DPRINTF("sym=%08x ", (unsigned) pExternals [relocCmd.r_symndx]);
#endif  /* DEBUG */
	    pSym = pExtSyms + relocCmd.r_symndx;
#ifdef DEBUG
            DPRINTF("pSym=%08x *pAdrs %08x", (unsigned)pSym, (unsigned)*pAdrs);
#endif  /* DEBUG */

	    symVal = (UINT32) pExternals[relocCmd.r_symndx];
    	    *(UINT32 *)pAdrs = *(UINT32*) pAdrs + symVal;
	    break;

	case R_IGLBLRA:
	    /*
	     * This is a global register A relocation, we have to set the
	     * register A field of the current instruction to the value
	     * associated with symbol.
	     *
	     * instruction format: |OP|RC|RA|RB|
	     */
#ifdef DEBUG
            DPRINTF("IGLBLRA ");
            DPRINTF("sym=%08x ", (unsigned) pExternals [relocCmd.r_symndx]);
#endif  /* DEBUG */

	    symVal = (UINT32) pExternals[relocCmd.r_symndx];
	    *(UINT32 *)pAdrs = (*(UINT32 *)pAdrs | 
			        ((symVal & REG_MASK) << REG_A_OFFSET));
	    break;

	case R_IGLBLRB:
	    /*
	     * This is a global register B relocation, we have to set the
	     * register B field of the current instruction to the value
	     * associated with symbol.
	     *
	     * instruction format: |OP|RC|RA|RB|
	     */
#ifdef DEBUG
            DPRINTF("IGLBLRB ");
            DPRINTF("sym=%08x ", (unsigned) pExternals [relocCmd.r_symndx]);
#endif  /* DEBUG */

	    symVal = (UINT32) pExternals[relocCmd.r_symndx];
	    *(UINT32 *)pAdrs = (*(UINT32 *)pAdrs | 
			        ((symVal & REG_MASK) << REG_B_OFFSET));
	    break;

	case R_IGLBLRC:
	    /*
	     * This is a global register C relocation, we have to set the
	     * register C field of the current instruction to the value
	     * associated with symbol.
	     *
	     * instruction format: |OP|RC|RA|RB|
	     */
#ifdef DEBUG
            DPRINTF("IGLBLRC ");
            DPRINTF("sym=%08x ", (unsigned) pExternals [relocCmd.r_symndx]);
#endif  /* DEBUG */

	    symVal = (UINT32) pExternals[relocCmd.r_symndx];
	    *(UINT32 *)pAdrs = (*(UINT32 *)pAdrs | 
			        ((symVal & REG_MASK) << REG_C_OFFSET));
	    break;

	default:
            printErr("Unrecognized relocation type %d\n",
                     relocCmd.r_type);
            errnoSet (S_loadLib_UNRECOGNIZED_RELOCENTRY);
            return (ERROR);
            break;
	    }
#ifdef DEBUG
            DPRINTF("\n");
#endif  /* DEBUG */
	}
    return(status);
    }
#endif /* (CPU_FAMILY == AM29XXX) */

#if (CPU_FAMILY == I960)
/*******************************************************************************
*
* coffRelSegmentI960 - perform relocation for the I960 family
*
* This routine reads the specified relocation command segment and performs
* all the relocations specified therein.
* Absolute symbol addresses are looked up in the 'externals' table.
*
* This function handles the following types of relocation commands
* for the i960 processor:
*	R_RELLONG  - direct 32 bit relocation
*	R_IPRMED   - 24-bit IP-relative relocation
*       R_OPTCALL  - optimizable call (check if can be changed to BAL)
*       R_OPTCALLX - optimizable callx (check if can be changed to BALX)
*		
* RETURNS: OK or ERROR
*/

LOCAL STATUS coffRelSegmentI960
    (
    RELOC *pRelCmds,		/* list of relocation commands */
    SCNHDR *pScnHdrArray,	/* array of section headers */
    char *scnAddr,		/* section loaded address */
    char **pExternals,		/* array of absolute symbol values */
    SYMENT *pExtSyms,         /* pointer to object file symbols */
    SEG_INFO *pSeg,		/* section addresses and sizes */
    int section,                /* section number -1 for relocation */
    SYMTAB_ID symTbl		/* symbol table to use */
    )
    {
    FAST long *pAdrs;           /* relocation address */
    int nCmds;                  /* # reloc commands for seg */
    RELOC relocCmd;             /* relocation structure */
    INT8 *symVal0 = 0;          /* absolute symbol value */
    INT8 *symVal1 = 0;          /* leaf-proc absolute symbol value */
    INT32 offset;	        /* relocation value accumulator */
    char symName[MAX_SYS_SYM_LEN + 1];      /* temp buffer for symbol name */
    SYM_TYPE symType;           /* type from sys symbol table */
    STATUS status = OK;
    SYMENT *pSym;             /* pointer to an external symbol */
    SCNHDR *pScnHdr;		/* section header for relocation */

    pScnHdr = pScnHdrArray + section;

    for (nCmds = pScnHdr->s_nreloc; nCmds > 0; nCmds--)
	{
	/* read relocation command */

	relocCmd = *pRelCmds++;

	/* 
	 * Calculate actual address in memory that needs relocation.
	 */

        if (pScnHdr->s_flags & STYP_TEXT)
	    {
	    pAdrs = (long *) ((long) scnAddr + (relocCmd.r_vaddr - pScnHdr->s_vaddr));
	    }
	else                            /* (pScnHdr->s_flags & STYP_DATA) */
	    {
	    pAdrs = (long *) ((long) scnAddr + (relocCmd.r_vaddr - pScnHdr->s_vaddr));
	    }

#ifdef DEBUG
        DPRINTF("r_vaddr=%02x *pAdrs=%08x ", 
                relocCmd.r_vaddr, *(UINT32 *)pAdrs);
#endif  /* DEBUG */

        switch (relocCmd.r_type)
            {

        case R_OPTCALL:
        case R_OPTCALLX:
            /* Check for a call to a leaf procedure.  If it is, then
             * replace the CALL opcode with BAL and correct the displacement.
             */
#ifdef DEBUG
            if (relocCmd.r_type == R_OPTCALL)
                {
                DPRINTF("OPTCALL ");
                }
            else
                {
                DPRINTF("OPTCALLX ");
                }
#endif
	    if (fast_load == FALSE)
		{
		if (symFindByValue (symTbl, (UINT)symVal0, (char *) symName,
				    (int *) &symVal0, &symType)
		    != OK)
		    {
		    printErr (symNotFoundErrMsg);
		    status = ERROR;
		    break; 		/* skip to next symbol */
		    }

		if (symType & BAL_ENTRY)
		    {
		    /* get symbol value from externals table */
		    symVal0 = (INT8 *) (pExternals[relocCmd.r_symndx]);

		    /* cat a ".lf" on the end of the symName */
		    strcat ((char *)symName, ".lf");

		    /* Now check if a symbol exists as name.lf
		     * If it does, it is the BAL entry for the leaf proc.
		     * start at symName[1] to ignore the '_'.
		     */
		    if (symFindByName (symTbl, &symName[1],
				       &symVal1, &symType) == OK)
			{
#ifdef DEBUG
			DPRINTF("LEAF ");
#endif  /* DEBUG */
			/* A call to a leaf procedure: replace
			 * CALL opcode with BAL.
			 */

			if (relocCmd.r_type == R_OPTCALL)
			    {
			    offset = (INT32) *pAdrs;
			    offset &= OFFSET24_MASK;	/* clear opcode */
			    offset |= ~OFFSET24_MASK;	/* sign extend */
			    offset += ((INT32) symVal1 - (INT32) symVal0);
			    offset &= OFFSET24_MASK;

			    *(UINT32 *)pAdrs = BAL_OPCODE | offset;
			    }
			else                /* R_OPTCALLX */
			    {
			    /*  Put in opcode for BALX with:
			     *      absolute displacement addressing mode and
			     *      register g14 for the return address.
			     */
			    *(UINT32 *)pAdrs = BALXG14_OPCODE;
			    *(UINT32 *)(pAdrs+1) +=
                            (INT32) symVal1 - (INT32) symVal0; 
#ifdef DEBUG
			    DPRINTF("*(pAdrs+1)=%08x ", *(UINT32 *)(pAdrs+1));
#endif  /* DEBUG */
			    }
			}
		    }
		else
		    {
#ifdef DEBUG
		    DPRINTF("NO LEAF ");
#endif  /* DEBUG */
		    /* No leaf proc entry.
		     * Do nothing with the OPTCALL.
		     * This is a normal call/return routine.
		     */
		    }
		}
            break;

        case R_RELLONG:
            /*
             *  This relocation is performed by adding the absolute address
             *  of the symbol to the relocation value in the code and
             *  subtracting the symbol's value (relative to the section).
             *
             * The new reloc_value =
             *   symbol_absolute_addr + reloc_value_in_code - symbol_value;
             *
             *  Both reloc_value_in_code and symbol_value are relative to
             *  the beginning of the first section in the object file.
             *  For undefined externals, reloc_value_in_code = 0
             *  and symbol_value = 0.
             */

#ifdef DEBUG
            DPRINTF("RELLONG ");
            DPRINTF("sym=%08x ", (unsigned) pExternals [relocCmd.r_symndx]);
#endif  /* DEBUG */
            offset = (INT32) pExternals[relocCmd.r_symndx];
            pSym = pExtSyms + relocCmd.r_symndx;
            if (! COFF_COMM(pSym))
                {
                offset -= pSym->n_value;
#ifdef DEBUG
                DPRINTF("n_value=0x%x ", pSym->n_value);
#endif  /* DEBUG */
                }
            *pAdrs += offset;
            break;

        case R_IPRMED:
#ifdef DEBUG
            DPRINTF("IPRMED  ");
#endif  /* DEBUG */
            pSym = pExtSyms + relocCmd.r_symndx;
            if (COFF_UNDF(pSym))        /* if symbol was undefined external */
                {
                /* Code to be relocated contains an offset
                 * to the beginning of the first section in the object file.
                 * To relocate it, add the absolute address
                 * of the referenced symbol and subtract the
                 * amount by which the input section will be
                 * relocated.
                 */
                offset = (INT32) *pAdrs;
                offset &= OFFSET24_MASK;	/* clear opcode */
                offset |= ~OFFSET24_MASK;	/* sign extend */
                offset += (INT32) pExternals [relocCmd.r_symndx];
#ifdef DEBUG
                DPRINTF("sym=%08x ", (unsigned)pExternals [relocCmd.r_symndx]);
#endif  /* DEBUG */

                /* IP-relative relocations were done assuming
                 * text starts at 0. Adjust for actual start of text. */

                offset -= (INT32)scnAddr - pScnHdr->s_vaddr;
                offset &= 0x00ffffff;
                *(UINT32 *)pAdrs = (*(UINT32 *)pAdrs & 0xff000000) | offset;
                }
            else
                {
#ifdef DEBUG
                DPRINTF("NO RELOC ");
#endif  /* DEBUG */
                /* The symbol we are relocating to came from
                 * the same input file as the code that
                 * references it.  The relative distance
                 * between the two will not change.  Do nothing.
                 */
                }
            break;

	default:
            printErr("Unrecognized relocation type %d\n",
                     relocCmd.r_type);
            errnoSet (S_loadLib_UNRECOGNIZED_RELOCENTRY);
            return (ERROR);
            break;
	    }

#ifdef DEBUG
        DPRINTF("*pAdrs=%08x\n", *(UINT32 *)pAdrs);
#endif  /* DEBUG */
	}
    return(status);
    }
#endif /* (CPU_FAMILY == I960) */

#if (CPU_FAMILY == ARM)
/*******************************************************************************
*            
* CoffRelSegmentArm - perform relocation for the ARM family
*
* This routine reads the specified relocation command segment and performs
* all the relocations specified therein.
* Absolute symbol addresses are looked up in the 'externals' table.
*
* This function handles the following types of relocation commands
* for the ARM family of processor:
*
*	- R_RELBYTE	Byte PC Relative
*	- R_RELWORD	Word PC Relative
*	- R_RELARMLONG	Long PC Relative
*	- R_RELARM26	26 bit PC Relative
*	- R_DONEARM26	completed 26 bit PC relative
*	- R_THUMB9	Thumb  9 bit PC-relative
*	- R_THUMB12	Thumb 12 bit PC-relative
*	- R_THUMB23	Thumb 23 bit PC-relative
*	- R_DONETHUMB23	completed Thumb 23 bit PC-relative
*
* Note that RVA32 relocations are never produced in ARM code.
*
* RETURNS: OK or ERROR
*
* INTERNAL
* Large parts of this code were taken lifted from the host-based loader
* when it had not been fully tested and the tool-chain was full of
* problems. Consequently, there may be bugs...
* The handling of literal sections (.lit/.rdata) is taken from the
* Am29k code above.
*/
 
LOCAL STATUS coffRelSegmentArm
    (
    RELOC *	pRelCmds,	/* list of relocation commands */
    SCNHDR *	pScnHdrArray,	/* array of section headers */
    char **	pExternals,	/* array of absolute symbol values */
    SYMENT *	pExtSyms,	/* pointer to object file symbols */
    SEG_INFO *	pSeg,		/* section addresses and sizes */
    int		section,	/* section number -1 for relocation */
    SYMTAB_ID	symTbl		/* symbol table to use */
    )
    {
    void *	pAdrs;		/* relocation address */
    int		nCmds;		/* # reloc commands for seg */
    RELOC	relocCmd;	/* relocation structure */
    STATUS	status = OK;
    ULONG	value;		/* value being relocated */
    SCNHDR *	pScnHdr;	/* section header for relocation */
    SYMENT *	pSym;		/* pointer to external symbol */
 
    pScnHdr = pScnHdrArray + section;
 
    for (nCmds = pScnHdr->s_nreloc; nCmds > 0; nCmds--)
	{
	/* read relocation command */

	relocCmd = *pRelCmds++;
 

	/* calculate actual address in memory that needs relocation  */
 
	if (pScnHdr->s_flags & STYP_TEXT)
	    {
	    if (!(pScnHdr->s_flags & STYP_LIT))
	        {
		pAdrs = (void *) ((long) pSeg->addrText + relocCmd.r_vaddr -
		pScnHdr->s_vaddr + sprFixTextOff[section]); 
		}
	    else
	        {
		/*
		 * The lit/rdata section is located after the text section.
		 * We take care of this by adding the "real" text size to the
		 * text address, just like the AM29K. A better solution would
		 * be to add .lit fields in the SEG_INFO structure.
		 */

		pAdrs = (void *) ((long) pSeg->addrText + sprFixTextOff[section]
					+ relocCmd.r_vaddr - pScnHdr->s_vaddr);
	        }
	    }
	else
	    pAdrs = (void *) ((long) pSeg->addrData +
		(relocCmd.r_vaddr - pScnHdr->s_vaddr));
 
 
	/* now relocate according to type */
 
	switch (relocCmd.r_type)
	    {
	    case R_RELBYTE:	/* relocate a byte */
		value = *(UINT8 *)pAdrs;

		/*
		 * add address of symbol to the relocation value,
		 * subtracting the symbol's value relative to the section
		 */

		value += (UINT32)pExternals[relocCmd.r_symndx];
		pSym = pExtSyms + relocCmd.r_symndx;
		if (!COFF_COMM(pSym))
		    value -= pSym->n_value;

		/* check for overflow */

		if ((value & ~0xFF) != 0)
		    {
		    printErr (outOfRangeErrMsg);
		    status = ERROR;
		    break;
		    }
		*(UINT8 *)pAdrs = value;
		break;
 
 
	    case R_RELWORD:	/* relocate a 16 bit word */
		value = *(UINT16 *)pAdrs;

		/*
		 * add address of symbol to the relocation value,
		 * subtracting the symbol's value relative to the section
		 */

		value += (UINT32)pExternals[relocCmd.r_symndx];
		pSym = pExtSyms + relocCmd.r_symndx;
		if (!COFF_COMM(pSym))
		    value -= pSym->n_value;

		/* check for overflow */

		if ((value & ~0xFFFF) != 0)
		    {
		    printErr (outOfRangeErrMsg);
		    status = ERROR;
		    break;
		    }
		*(UINT16 *)pAdrs = value;
		break;


	    case R_RELARMLONG:	/* relocate a long */
		{
		char symName[MAX_SYS_SYM_LEN + 1];  /* buffer for sym name */
		int symVal0, symVal1;
		SYM_TYPE symType;

		value = *(UINT32 *)pAdrs;

		/*
		 * add address of symbol to the relocation value,
		 * subtracting the symbol's value relative to the section
		 */

		symVal0 = (UINT32)pExternals[relocCmd.r_symndx];
		value += symVal0;
		pSym = pExtSyms + relocCmd.r_symndx;

		if (!COFF_COMM(pSym))
		    value -= pSym->n_value;

		/* determine whether it's a Thumb code symbol */

		if (symFindByValue (symTbl, symVal0, symName,
				    &symVal1, &symType) != OK)
		    {
		    printErr (symNotFoundErrMsg);
		    status = ERROR;
		    break; 		/* skip to next symbol */
		    }
		else
		    if (symType & SYM_THUMB && symVal0 == symVal1)
			{
			/* it *is* a Thumb code symbol */

			value |= 1;
			}

		*(UINT32 *)pAdrs = value;
		break;
		}
 
 
	    case R_RELARM26:    /* relocate a 26 bit offset e.g. BL */
		{
		ULONG	reloc;

		value = *(UINT32 *)pAdrs;

		/*
		 * extract word offset from instruction, convert to
		 * byte offset and sign-extend it
		 */

		reloc = (value & 0x00ffffff) << 2;	/* byte offset */
		reloc = (reloc ^ 0x02000000) - 0x02000000; /* sign extend  */
 
		/*
		 * relocate it by calculating the offset from the instruction
		 * to the symbol, adjusted by the symbol's value relative
		 * to the section
		 */

		reloc += (UINT32)pExternals[relocCmd.r_symndx] - (UINT32)pAdrs;
		pSym = pExtSyms + relocCmd.r_symndx;
#if FALSE
                /* It is quite possible that we will encounter a assembler that
                 * will generate a relocation literal that has this value encode
                 * as does the I960. For now the GNU ARM assembler does not and
                 * so the code is dissabled.
                 */
 
		if (!COFF_COMM(pSym))
		    reloc -= pSym->n_value;
#endif
		/* check for overflow */

		if ((reloc & 3) ||
		    ((reloc & 0x02000000) &&
		     ((reloc & ~0x03ffffff) != ~0x03ffffffU)) ||
		    ((reloc & 0x02000000) == 0 && (reloc & ~0x03ffffff)))
		    {
		    /* overflow in 26 bit relocation */
		    
		    printErr (outOfRangeErrMsg);
		    status = ERROR;
		    break;
		    }
 
		/* put calculated offset into instruction */
 
		value &= ~0x00ffffff;			/* extract cmd */
		value |= (reloc >> 2) & 0x00ffffff;	/* insert offset */
		*(UINT32 *)pAdrs = value;
		break;
		}
 

	    case R_RELTHUMB9:	/* relocate a Thumb 9 bit offset e.g. BEQ */
	    case R_RELTHUMB12:	/* relocate a Thumb 12 bit offset e.g. B */
		printErr ("THUMB%d relocations not supported\n",
			  relocCmd.r_type == R_RELTHUMB9 ? 9 : 12);
		errnoSet (S_loadLib_NO_RELOCATION_ROUTINE);
		return ERROR;
		break;


	    case R_RELTHUMB23:	/* relocate a Thumb 23 bit offset e.g. BL */
		{
		ULONG	reloc;

		value = *(UINT16 *)pAdrs | (*(UINT16 *)(pAdrs + 2) << 16);

		/*
		 * extract offset from instruction, convert to
		 * byte offset and sign-extend it
		 */

#if defined(ARMEB)
		reloc = ((value & 0x7ff) << 1) | ((value & 0x07ff0000) >> 4);
#else
		reloc = ((value & 0x7ff) << 12) | ((value & 0x07ff0000) >> 15);
#endif
		reloc = (reloc ^ 0x00400000) - 0x00400000; /* sign extend */

		/*
		 * relocate it by calculating the offset from the instruction
		 * to the symbol, and subtract the symbol's value relative
		 * to the section
		 */

		reloc += (UINT32)pExternals[relocCmd.r_symndx] & ~1;
		reloc -= (UINT32)pAdrs;
		pSym = pExtSyms + relocCmd.r_symndx;

#if FALSE
                /* It is quite possible that we will encounter a assembler that
                 * will generate a relocation literal that has this value encode
                 * as does the I960. For now the GNU ARM assembler does not and
                 * so the code is dissabled.
                 */
 
		if (!COFF_COMM(pSym))
		    reloc -= pSym->n_value;
#endif

		/* check for overflow */

		if ((reloc & 1) ||
		     ((reloc & 0x00400000) &&
		      ((reloc & ~0x007fffff) != ~0x007fffffU)) ||
		     ((reloc & 0x00400000) == 0 && (reloc & ~0x007fffff)))
		    {
		    /* overflow in 23 bit relocation */

		    printErr (outOfRangeErrMsg);
		    status = ERROR;
		    break;
		    }

		/* put calculated offset into instruction */

		value &= ~0x07ff07ff;			/* Extract cmd */
#if defined(ARMEB)
		value |= ((reloc & 0xffe) >> 1) | ((reloc << 4) & 0x7ff0000);
#else
		value |= ((reloc & 0xffe) << 15) | ((reloc >> 12) & 0x7ff);
#endif

		*(UINT16 *)pAdrs = value;
		*(UINT16 *)(pAdrs + 2) = value >> 16;
		break;
		}
 

	    case R_DONEARM26:	/* already relocated */
	    case R_DONETHUMB23:	/* already relocated */
		break;
 
 
	    default:
		printErr("Unrecognized relocation type %d\n", relocCmd.r_type);
		errnoSet (S_loadLib_UNRECOGNIZED_RELOCENTRY);
		return ERROR;
		break;
	    }
	}
    return status;
    }
#endif /* (CPU_FAMILY == ARM) */

/*******************************************************************************
*
* coffHdrRead - read in COFF header and swap if necessary
* 
* Note:  To maintain code portability, we can not just read sizeof(FILHDR) 
*	bytes.  Compilers pad structures differently,
*	resulting in different structure sizes.
*	So, we read the structure in an element at a time, using the size
*	of each element.
*/

LOCAL STATUS coffHdrRead
    (
    int fd,
    FILHDR *pHdr,
    BOOL *pSwap
    )
    {
    int status;

    if (fioRead (fd, (char *) &(pHdr->f_magic), sizeof (pHdr->f_magic) )
		!= sizeof (pHdr->f_magic))
	{
	errnoSet ((int) S_loadLib_FILE_READ_ERROR);
	return (ERROR);
	}

    switch (pHdr->f_magic)
	{
        case I960ROMAGIC:
        case I960RWMAGIC:
	case AM29KBIGMAGIC:
	case ARMMAGIC:
            *pSwap = FALSE;
            break;

        case SWAB_SHORT(I960ROMAGIC):
        case SWAB_SHORT(I960RWMAGIC):
	case AM29KLITTLEMAGIC:
	case SWAB_SHORT(ARMMAGIC):
            *pSwap = TRUE;
            break;

	default:
#ifndef MULTIPLE_LOADERS
	    printErr (fileTypeUnsupported, pHdr->f_magic);
#endif
	    errnoSet (S_loadLib_FILE_ENDIAN_ERROR);
	    return (ERROR);
	    break;
	}

    status = fileRead (fd, &pHdr->f_nscns, sizeof(pHdr->f_nscns), *pSwap);
    status |= fileRead (fd, &pHdr->f_timdat, sizeof(pHdr->f_timdat), *pSwap);
    status |= fileRead (fd, &pHdr->f_symptr, sizeof(pHdr->f_symptr), *pSwap);
    status |= fileRead (fd, &pHdr->f_nsyms, sizeof(pHdr->f_nsyms), *pSwap);
    status |= fileRead (fd, &pHdr->f_opthdr, sizeof(pHdr->f_opthdr), *pSwap);
    status |= fileRead (fd, &pHdr->f_flags, sizeof(pHdr->f_flags), *pSwap);

#if (CPU_FAMILY == ARM)
    if (status != ERROR)
	{
	/* check endianness of object matches endianness of target */

#if (_BYTE_ORDER == _BIG_ENDIAN)
	if ((pHdr->f_flags & (F_AR32W | F_AR32WR)) != F_AR32W)
#else
	if ((pHdr->f_flags & (F_AR32W | F_AR32WR)) != F_AR32WR)
#endif
	    {
	    printErr (incorrectEndiannessErrMsg);
	    errnoSet (S_loadLib_FILE_ENDIAN_ERROR);
	    return (ERROR);
	    }
	}
#endif	/* CPU_FAMILY == ARM */

    max_scns = pHdr->f_nscns;
    return (status);
    }

/*******************************************************************************
*
* coffOpthdrRead - read in COFF optional header and swap if necessary
* 
*/

LOCAL STATUS coffOpthdrRead
    (
    int     fd,
    AOUTHDR *pOptHdr,
    BOOL    swapTables
    )
    {
    int status;

    status = fileRead(fd, &pOptHdr->magic, sizeof(pOptHdr->magic),
                            swapTables);
    status |= fileRead(fd, &pOptHdr->vstamp, sizeof(pOptHdr->vstamp),
                            swapTables);
    status |= fileRead(fd, &pOptHdr->tsize, sizeof(pOptHdr->tsize),
                            swapTables);
    status |= fileRead(fd, &pOptHdr->dsize, sizeof(pOptHdr->dsize),
                            swapTables);
    status |= fileRead(fd, &pOptHdr->bsize, sizeof(pOptHdr->bsize),
                            swapTables);
    status |= fileRead(fd, &pOptHdr->entry, sizeof(pOptHdr->entry),
                            swapTables);
    status |= fileRead(fd, &pOptHdr->text_start, sizeof(pOptHdr->text_start),
                            swapTables);
    status |= fileRead(fd, &pOptHdr->data_start, sizeof(pOptHdr->data_start),
                            swapTables);
#if (CPU_FAMILY == I960)
    status |= fileRead(fd, &pOptHdr->tagentries, sizeof(pOptHdr->tagentries),
                            swapTables);
#endif /* (CPU_FAMILY == I960) */

    return (status);
    }


/*******************************************************************************
*
* coffSecHdrRead - read in COFF section header and swap if necessary
* 
*/

LOCAL STATUS coffSecHdrRead
    (
    int    fd,
    SCNHDR *pScnHdr,
    FILHDR *pHdr,
    BOOL   swapTables
    )
    {
    int ix;
    int status = 0;

    /* check for correct section count */

    if (pHdr->f_nscns > MAX_SCNS)
	{
        errnoSet(S_loadLib_SCNHDR_READ);
	return (ERROR);
	}

    for (ix = 0; ix < pHdr->f_nscns; ix++)
	{
        status = fileRead(fd, pScnHdr->s_name, sizeof(pScnHdr->s_name),
                          FALSE);
        status |= fileRead(fd, &pScnHdr->s_paddr, sizeof(pScnHdr->s_paddr),
                           swapTables);
        status |= fileRead(fd, &pScnHdr->s_vaddr, sizeof(pScnHdr->s_vaddr),
                            swapTables);
        status |= fileRead(fd, &pScnHdr->s_size, sizeof(pScnHdr->s_size),
                            swapTables);
        status |= fileRead(fd, &pScnHdr->s_scnptr, sizeof(pScnHdr->s_scnptr),
                            swapTables);
        status |= fileRead(fd, &pScnHdr->s_relptr, sizeof(pScnHdr->s_relptr),
                            swapTables);
        status |= fileRead(fd, &pScnHdr->s_lnnoptr, sizeof(pScnHdr->s_lnnoptr),
                            swapTables);
        status |= fileRead(fd, &pScnHdr->s_nreloc, sizeof(pScnHdr->s_nreloc),
                            swapTables);
        status |= fileRead(fd, &pScnHdr->s_nlnno, sizeof(pScnHdr->s_nlnno),
                            swapTables);
        status |= fileRead(fd, &pScnHdr->s_flags, sizeof(pScnHdr->s_flags),
                            swapTables);
#if (CPU_FAMILY == I960)
        status |= fileRead(fd, &pScnHdr->s_align, sizeof(pScnHdr->s_align),
                            swapTables);
#endif /* (CPU_FAMILY == I960) */
	pScnHdr++;
	}

    return (status);
    }

/*******************************************************************************
*
* coffReadRelocInfo - read in COFF relocation information and swap if necessary
* 
* Assumptions:  The file pointer is positioned at the start of the relocation
*		records.
*
*		The relocation records are ordered by section.
*/

LOCAL STATUS coffReadRelocInfo
    (
    int     fd,
    SCNHDR  *pScnHdr,
    RELOC   **pRelsPtr,
    BOOL    swapTables
    )
    {
    int ix;
    int iy;
    int relocSize;
    int status = OK;
    RELOC *pRels;                       /* pointer to single reloc entry */

    for (ix = 0; ix < max_scns; ix++)
	{
        if (pScnHdr->s_nreloc > 0)
	    {
	    relocSize = (int) pScnHdr->s_nreloc * RELSZ;
	    if ((*pRelsPtr = (RELOC *)malloc (relocSize)) == NULL)
		{
		return (ERROR);
		}

	if (lseek (fd, pScnHdr->s_relptr, SEEK_SET) == ERROR)
	    {
	    return (ERROR);
	    }

            for (iy = 0, pRels = *pRelsPtr;
                 (iy < (int) pScnHdr->s_nreloc) && (status == OK);
                 iy++, pRels++)
                {
                status = fileRead(fd, &pRels->r_vaddr,
                                   sizeof(pRels->r_vaddr), swapTables);
                status |= fileRead(fd, &pRels->r_symndx,
                                   sizeof(pRels->r_symndx), swapTables);
                status |= fileRead(fd, &pRels->r_type,
                                   sizeof(pRels->r_type), swapTables);
#if ((CPU_FAMILY == I960) || (CPU_FAMILY == ARM))
                status |= fileRead(fd, pRels->pad,
                                   sizeof(pRels->pad), swapTables);
#endif /* (CPU_FAMILY == I960) */
                }

            if (status != OK)
                return(status);
	    }

	pScnHdr++;
	pRelsPtr++;
        }

    return (OK);
    }


/*******************************************************************************
*
* coffReadSym - read one COFF symbol entry
* 
*/

LOCAL STATUS coffReadSym
    (
    int     fd,
    SYMENT *pSym,                       /* where to read symbol entry */
    BOOL    swapTables
    )
    {
    int status = OK;

    if (fioRead (fd, pSym->n_name, sizeof(pSym->n_name)) 
			!= sizeof(pSym->n_name))
	status = ERROR;	

    if (swapTables && (status == OK))
        {
        /*  The n_name field is part of a union in the syment struct.
         *  If the union uses n_name (array of char), then no
         *  swabbing in required.
         *  If the union uses n_offset (into string table), then
         *  the n_offset must be swabbed.
         */
        if (pSym->n_zeroes == 0)
            {
            swabData(&pSym->n_offset, sizeof(pSym->n_offset));
            }
        }
     status |= fileRead (fd, &pSym->n_value, sizeof(pSym->n_value),
                        swapTables);
    status |= fileRead (fd, &pSym->n_scnum, sizeof(pSym->n_scnum),
                        swapTables);
#if (CPU_FAMILY == I960)
    status |= fileRead (fd, &pSym->n_flags, sizeof(pSym->n_flags),
                        swapTables);
#endif /* (CPU_FAMILY == I960) */
    status |= fileRead (fd, &pSym->n_type, sizeof(pSym->n_type),
                        swapTables);
    status |= fileRead (fd, &pSym->n_sclass, sizeof(pSym->n_sclass),
                        swapTables);
    status |= fileRead (fd, &pSym->n_numaux, sizeof(pSym->n_numaux),
                        swapTables);
#if (CPU_FAMILY == I960)
    status |= fileRead (fd, pSym->pad2, sizeof(pSym->pad2),
                        swapTables);
#endif /* (CPU_FAMILY == I960) */
    return(status);
    }


/*******************************************************************************
*
* coffReadExtSyms - read COFF symbols
* 
*  For low memory systems, we can't afford to keep the entire
*  symbol table in memory at once.
*  For each symbol entry, the fields required for relocation
*  are saved in the externalSyms array.
*/

LOCAL STATUS coffReadExtSyms
    (
    int fd,
    SYMENT **pSymbols,      	/* pointer to array of symbol entries */
    char ***pSymAddr,         	/* pointer to array of addresses */
    FILHDR *pHdr,
    BOOL    swapTables
    )
    {
    int status;
    int ix;
    SYMENT *pSym;                     /* pointer to symbol entry */

    if (pHdr->f_nsyms)
        {
        /*  malloc space for array of absolute addresses
         */
        if ((*pSymAddr = malloc((UINT)(pHdr->f_nsyms) * sizeof(char *)))
            == NULL)
	    {
	    printErr (extMemErrMsg, (UINT)(pHdr->f_nsyms) * sizeof (char *));
	    return (ERROR);
	    }

        /*  malloc space for symbols
         */
        if ((*pSymbols = malloc((UINT)(pHdr->f_nsyms) * SYMESZ))
            == NULL)
	    {
	    return (ERROR);
	    }

	if (lseek (fd, pHdr->f_symptr, SEEK_SET) == ERROR)
	    {
	    return (ERROR);
	    }

        for (ix = 0, pSym = *pSymbols; ix < pHdr->f_nsyms; ix++, pSym++)
            {
            if ((status = coffReadSym(fd, pSym, swapTables) != OK))
		return (status);
            }
      }
    return (OK);
    }
/*******************************************************************************
*
* coffReadExtStrings - read in COFF external strings
* 
* Assumptions:  The file pointer is positioned at the start of the
*		string table.
*/
#define STR_BYTES_SIZE 4                /* size of string table length field */

LOCAL STATUS coffReadExtStrings
    (
    int fd,
    char **pStrBuf,
    BOOL    swapTables
    )
    {
    unsigned long strBytes;
    int status;

    /* Read length of string table
     */
    if ((status = fileRead (fd, &strBytes, STR_BYTES_SIZE, swapTables))
        != OK)
        {
        return (status);
        }

    if (strBytes > STR_BYTES_SIZE)
        {
        if ((*pStrBuf = malloc(strBytes)) == NULL)
	    {
            printErr (stringMemErrMsg, strBytes);
	    return (ERROR);
	    }

        bcopy((char *) &strBytes, (char *) *pStrBuf, STR_BYTES_SIZE);
        strBytes -= STR_BYTES_SIZE;     /* # bytes left to read  */

	    if (fioRead (fd, *pStrBuf + STR_BYTES_SIZE, strBytes) 
			!= strBytes)
	    {
	    return (ERROR);
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

#define SEEKBUF	1024

LOCAL STATUS softSeek
    (
    int fd,			/* fd to seek with */
    int startOfFileOffset	/* byte index to seek to */
    )
    {
    int position;		/* present file position */
    int bytesToRead;		/* bytes needed to read */
    char tempBuffer[SEEKBUF];	/* temp buffer for bytes */

    position = ioctl (fd, FIOWHERE, startOfFileOffset);

    if (position > startOfFileOffset)
	return(ERROR);

    /* calculate bytes to read */

    bytesToRead = startOfFileOffset - position;

    while (bytesToRead >= SEEKBUF)
	{
        if (fioRead (fd, tempBuffer, SEEKBUF) != SEEKBUF)
	    return (ERROR);
	bytesToRead -= SEEKBUF;
	}

    if (bytesToRead > 0)
	{
        if (fioRead (fd, tempBuffer, bytesToRead) != bytesToRead)
	    return (ERROR);
	}
#ifdef NO_COFF_SOFT_SEEK
    if (ioctl (fd, FIOSEEK, startOfFileOffset) == ERROR)
        return (ERROR);
#endif
    return(OK);
    }

/*******************************************************************************
*
* fileRead - loader file read function
* 
*/

LOCAL STATUS fileRead
    (
    int     fd,
    char    *pBuf,                      /* buffer to read data into */
    UINT    size,                       /* size of data field in bytes */
    BOOL    swap
    )
    {
    int     status = OK;

    if (fioRead (fd, pBuf, size) != size)
        {
	errnoSet (S_loadLib_FILE_READ_ERROR);
        return(ERROR);
        }
    if (swap)
        {
        status = swabData(pBuf, size);
        }
    return(status);
    }




/*******************************************************************************
*
*  swabData - swap endianess of data field of any length
*
*/

LOCAL STATUS swabData
    (
    char    *pBuf,                      /* buffer containing data */
    UINT    size                       /* size of data field in bytes */
    )
    {
#define TEMP_BUF_SIZE 256

    int     status = OK;
    char    tempBuf[TEMP_BUF_SIZE];

    switch(size)
        {
        case sizeof(char):
            break;                      /* ok as is */

        case sizeof(short):
            *(unsigned short *) pBuf = SWAB_SHORT(*(unsigned short *) pBuf);
            break;

        case sizeof(long):
            swabLong(pBuf, tempBuf);
            bcopy(tempBuf, pBuf, size);
            break;

        default:
            if (size <= TEMP_BUF_SIZE)
                {
                swab(pBuf, tempBuf, size);
                bcopy(tempBuf, pBuf, size);
                }
            else
                status = ERROR;
            break;
        }
    return(status);
    }

/*******************************************************************************
*
*  swabLong - swap endianess of long word
*
*/

LOCAL void swabLong
    (
    char input[],
    char output[]
    )
    {
    output[0] = input[3];
    output[1] = input[2];
    output[2] = input[1];
    output[3] = input[0];
    }


/*******************************************************************************
*
* coffTotalCommons - 
* 
*  INTERNAL
*  All common external symbols are being tacked on to the end
*  of the BSS section.  This function determines the additional amount
*  of memory required for the common symbols.
*  
*/

LOCAL ULONG coffTotalCommons
    (
    SYMENT *pSym,             /* pointer to external symbols */
    FAST int nEnts,		/* # of entries in symbol table */
    ULONG startAddr             /* address to start from */
    )
    {
    int ix;
    ULONG alignBss;			/* next boundry for common entry */
    int nbytes;				/* bytes for alignment + data */
    int totalBytes = 0;			/* total additional count for bss */
    int auxEnts = 0;                    /* auxiliary symbol entries to skip */

    /* calculate room for external commons in bss */

    alignBss = startAddr;
    for (ix = 0; ix < nEnts; ix++, pSym++)
	{
        if (auxEnts)                    /* if this is an auxiliary entry */
            {
            auxEnts--;
            continue;                   /* skip it */
            }
        auxEnts = pSym->n_numaux;       /* # of aux entries for this symbol */

	if (COFF_COMM(pSym))
	    {
            nbytes = dataAlign(pSym->U_SYM_VALUE, alignBss)
                    + pSym->U_SYM_VALUE;
            totalBytes += nbytes;
            alignBss += nbytes;
            }
	}
    return(totalBytes);
    }

/*******************************************************************************
*
*  dataAlign - determine address alignment for a data operand in memory
* 
*  The address of a data operand must be correctly aligned for data access.
*  This is architecture dependent. 
*  Generally, data operands are aligned on 'natural' boundaries as follows:
*          bytes on byte boundaries (they already are)
*          half-word operands on half-word boundaries
*          word operands on word boundaries
*          double-word operands on double-word boundaries
*
*  However, if a structure of size = 7 had to be aligned, it would need
*  to be aligned on a boundary that is a multiple of 4 bytes.
*
*  RETURNS:
*	nbytes - number of bytes to be added to the address to obtain the
*                 correct alignment for the data
*/

LOCAL ULONG dataAlign
    (
    ULONG size,                         /* size of data to be aligned */
    ULONG addr                         /* next possible address for data */
    )
    {
    ULONG nbytes;                 /* # bytes for alignment */
    int align;                        /* address should be multiple of align */

    if (size <= 1)
        align = 1;
    else if (size < 4)
        align = 2;
    else if (size < 8)
        align = 4;
    else if (size < 16)
        align = 8;
    else
        align = MAX_ALIGNMENT;          /* max required alignment */

    nbytes = addr % align;
    if (nbytes != 0)                    /* if not on correct boundary */
        nbytes = align - nbytes;

    return(nbytes);
    }

/*******************************************************************************
*
* bootCoffModule - load an object module into memory
*
* This routine loads an object module in COFF format from the specified
* file, and places the code, data, and BSS at the locations specified within
* the file.  The entry point of the module is returned in <pEntry>.  This 
* routine is generally used for bootstrap loading.
*
* RETURNS:
* OK, or
* ERROR if the routine cannot read the file
*
* SEE ALSO: loadModuleAt()
*/

STATUS bootCoffModule
    (
    int	fd,				/* fd from which to read load module */
    FUNCPTR *pEntry			/* entry point of module */
    )
    {
    FILHDR hdr;				/* module's COFF header */
    AOUTHDR optHdr;             	/* module's COFF optional header */
    SCNHDR scnHdr[MAX_SCNS];		/* module's COFF section headers */
    int nBytes;
    int tablesAreLE;
    ULONG pos;
    char c;
    unsigned short idx;          /* index into section headers */

    /* read object module header */

    if (coffHdrRead (fd, &hdr, &tablesAreLE) != OK)
	return (ERROR);

    /* read in optional header */

    if (coffOpthdrRead (fd, &optHdr, tablesAreLE) != OK)
	return (ERROR);

    /* read in the section headers */

    if (coffSecHdrRead (fd, &scnHdr[0], &hdr, tablesAreLE) != OK)
	return(ERROR);

    /*
     * Read until start of raw data for first section (to cope with stripped
     * ARM kernels).
     *
     * We can't do an ioctl FIOWHERE on a rcmd opened file descriptor
     * so must determine where we are by what we've read so far.
     * Note that this uses single-byte reads which are slow but should be OK.
     */

    for (pos = FILHSZ + AOUTSZ + hdr.f_nscns * SCNHSZ;
	    pos < (ULONG)scnHdr[0].s_scnptr; ++pos)
	if (nBytes = fioRead(fd, &c, 1), nBytes < 1)
	    return ERROR;

    /* now read in the .text and .data sections (SPR #22452) */

    for (idx = 0; idx < hdr.f_nscns; idx++)
        {
        if ((scnHdr[idx].s_flags & STYP_TEXT) ||
            (scnHdr[idx].s_flags & STYP_LIT)  ||  /* Am29k only ? */
            (scnHdr[idx].s_flags & STYP_DATA)
           )
            {
            printf ("%ld + ", scnHdr[idx].s_size);

            nBytes = fioRead (fd, (char *) scnHdr[idx].s_vaddr, 
			      scnHdr[idx].s_size);

            if (nBytes < scnHdr[idx].s_size)
                {
                return (ERROR);
                }
            }
        }

    printf ("%ld\n", optHdr.bsize);

    /*  bss is zeroed by vxWorks startup code. 
     */

    *pEntry = (FUNCPTR) optHdr.entry;
    return (OK);
    }

/******************************************************************************
*
* bootCoffInit - Initialize the system for coff load modules
*
* This routine initialized VxWorks to use the COFF format for
* boot loading modules.
*
* RETURNS:
* OK, or
* ERROR if XXX
*
* SEE ALSO: loadModuleAt()
*/
STATUS bootCoffInit
    (
    )
    {
    /* XXX check for installed ? */
    bootLoadRoutine = bootCoffModule;
    return (OK);
    }

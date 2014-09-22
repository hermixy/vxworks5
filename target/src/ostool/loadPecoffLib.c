/* loadPecoffLib.c - pecoff object module loader */

/* Copyright 1998-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01l,11may02,fmk  SPR 77007 - improve common symbol support
01k,13mar02,pfl  removed GNU_LD_ADDS_VMA, fixed data symbol relocation 
                 calculation (SPR 73145)
01j,08mar02,pfl  rewind to the beginning of the file before reading the
                 header information (SPR 73136)
01i,19oct01,pad  Fixed erroneous relocation of non-text segments and made
		 compliant with a Microsoft-directed bizarreness (BSS section
		 size set in BSS section header's physical address field) in
		 the Gnu toolchain (SPR 70767). This fixes SPR 71534 along the
		 way. Now discardable sections are really not relocated.
		 Now really handles absolute symbols. No longer complains
		 because of .stab and .stabstr symbols. Tidied setting of
		 errnos and other little things of the like.
01h,26sep01,jmp  increased MAX_SCNS to 100 (SPR# 63431).
01g,30nov98,dbt  no longer clear seg.flags<xxx> after loadSegmentAllocate()
                 call. (SPR #23553).
01f,05oct98,pcn  Initialize all the fields in the SEG_INFO structure.
01e,23sep98,cym  adding check for STYP_INFO,NO_LOAD,REG,PAD before relocating.
01d,18sep98,pcn  Set to _ALLOC_ALIGN_SIZE the flags field in seg structure
                 (SPR #21836).
01c,15sep98,cym  ignoring s_vaddr for relocatable objects until ldsimpc -r
		 is fixed to sych it correctly.
01b,17jul98,pcn  Fixed SPR #21836: alignment mismatch between sections and
                 target.
01a,19mar98,cym  created from on loadCoffLib.c
*/

/*
DESCRIPTION
This library provides an object module loading facility for loading
PECOFF format object files.  The object files are loaded into memory, 
relocated properly, their external references resolved, and their external 
definitions added to the system symbol table for use by other modules and 
from the shell.  Modules may be loaded from any I/O stream.

EXAMPLE
.CS
    fdX = open ("/devX/objFile", READ);
    loadModule (fdX, ALL_SYMBOLS);
    close (fdX);
.CE
This code fragment would load the PECOFF file "objFile" located on
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

/* includes */

#include "vxWorks.h"
#include "bootLoadLib.h"
#include "cacheLib.h"
#include "errnoLib.h"
#include "fioLib.h"
#include "ioLib.h"
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
#include "private/loadLibP.h"

#if CPU==SIMNT
#include "loadPecoffLib.h"

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

/* 
 * SYM_BASIC_NOT_COMM_MASK and SYM_BASIC_MASK are temporary masks until
 * symbol values are harmonized between host and target sides
 */

#define SYM_BASIC_NOT_COMM_MASK   0x0d   /* basic mask but unset bit two for common symbol */
#define SYM_BASIC_MASK            0x0f   /* only basic types are of interest */

/*
 *  Abridged edition of a object file symbol entry SYMENT
 *
 *  This is used to minimize the amount of malloc'd memory.
 *  The abridged edition contains all elements of SYMENT required
 *  to perform relocation.
 */

struct symentAbridged {
	long		n_value;	/* value of symbol		*/
	short		n_scnum;	/* section number		*/
	char		n_sclass;	/* storage class		*/
	char		n_numaux;       /* number of auxiliary entries  */
};

#define	SYMENT_A struct symentAbridged
#define	SYMESZ_A sizeof(SYMENT_A)	

/* 
 *  PECOFF symbol types and classes as macros
 *
 *   'sym' in the macro defintion must be defined as:   SYMENT *sym;
 */
#define U_SYM_VALUE    n_value

#define PECOFF_EXT(sym) \
    ((sym)->n_sclass == C_EXT)

/*  PECOFF Undefined External Symbol */
#define PECOFF_UNDF(sym) \
    (((sym)->n_sclass == C_EXT) && ((sym)->n_scnum == N_UNDEF))

/*  PECOFF Common Symbol
 *    A common symbol type is denoted by undefined external
 *    with its value set to non-zero.
 */
#define PECOFF_COMM(sym) ((PECOFF_UNDF(sym)) && ((sym)->n_value != 0))

/* PECOFF Unassigned type
 *    An unassigned type symbol of class C_STAT generated by
 *    some assemblers
 */
#define PECOFF_UNASSIGNED(sym) (((sym)->n_sclass == C_STAT) && ((sym)->n_type == T_NULL))


/* misc defines */

#define INSERT_HWORD(WORD,HWORD)        \
    (((WORD) & 0xff00ff00) | (((HWORD) & 0xff00) << 8) | ((HWORD)& 0xff))
#define EXTRACT_HWORD(WORD) \
    (((WORD) & 0x00ff0000) >> 8) | ((WORD)& 0xff)
#define SIGN_EXTEND_HWORD(HWORD) \
    ((HWORD) & 0x8000 ? (HWORD)|0xffff0000 : (HWORD))

#define SWAB_SHORT(s) \
  ((((unsigned short) s & 0x00ff) << 8) | (((unsigned short) s & 0xff00) >> 8))

#define MAX_SCNS        100		/* max sections supported */
#define NO_SCNS         0

#define MAX_ALIGNMENT	4		/* max boundary for architecture */

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


/* forward declarations */

LOCAL void pecoffFree ();
LOCAL void pecoffSegSizes ();
LOCAL STATUS pecoffReadSym ();
LOCAL STATUS fileRead ();
LOCAL STATUS swabData ();
LOCAL void swabLong ();
LOCAL ULONG pecoffTotalCommons ();
LOCAL ULONG dataAlign ();
LOCAL STATUS pecoffHdrRead ();
LOCAL STATUS pecoffOpthdrRead ();
LOCAL STATUS pecoffSecHdrRead ();
LOCAL STATUS pecoffLoadSections ();
LOCAL STATUS pecoffReadRelocInfo ();
LOCAL STATUS pecoffReadExtSyms ();
LOCAL STATUS pecoffReadExtStrings ();
LOCAL STATUS pecoffRelSegmentI386 ();
LOCAL STATUS pecoffSymTab (int fd, struct filehdr * pHdr, SYMENT * pSymbols, 
			   char *** externals, int symFlag, SEG_INFO * pSeg,
			   char * symStrings, SYMTAB_ID symTbl,
			   char * pNextCommon, struct scnhdr * pScnHdrArray,
			   char ** pScnAddr, BOOL fullyLinked,
			   UINT16 group, int imageBase);
LOCAL STATUS loadPecoffCommonManage (char * symName, SYMTAB_ID symTbl, 
                                     SYM_ADRS * pSymAddr,
                                     SYM_TYPE * pSymType, int loadFlag);

/******************************************************************************
*
* ldPecoffModAtSym - load object module into memory with specified symbol table
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

MODULE_ID ldPecoffModAtSym
    (
    int fd,		/* fd from which to read module */
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
    FILHDR hdr;				/* module's PECOFF header */
    PEOPTION optHdr;             	/* module's PECOFF optional header */
    SCNHDR scnHdr[MAX_SCNS];		/* module's PECOFF section headers */
    char *scnAddr[MAX_SCNS];            /* array of section loaded address */
    RELOC  *relsPtr[MAX_SCNS]; 		/* section relocation */
    SEG_INFO seg;			/* file segment info */
    BOOL tablesAreLE;			/* boolean for table sex */
    BOOL fullyLinked = FALSE;          /* TRUE if already absolutely located*/
    int status;				/* return value */
    int ix;				/* temp counter */
    int nbytes;				/* temp counter */
    char *pCommons;                     /* start of common symbols in bss */
    char *addrBss;
    char        fileName[255];
    UINT16      group;
    MODULE_ID   moduleId;
    int hdrEnd;

    /* initialization */

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
	scnAddr[ix] = NULL;
	relsPtr[ix] = NULL;
	bzero((char *) &scnHdr[ix], (int) sizeof(SCNHDR));
	}

    /* read object module header */

    if (pecoffHdrRead (fd, &hdr, &tablesAreLE) != OK)
	{
	/*  Preserve errno value from subroutine. */

	goto error;
	}

    hdrEnd = lseek (fd, 0, SEEK_CUR);

    /* Read in optional header if one exists */

    if (hdr.f_opthdr)
        {
        if (pecoffOpthdrRead (fd, &optHdr, tablesAreLE) != OK)
            {
	    errnoSet(S_loadLib_OPTHDR_READ);
            goto error;
            }
        }

    lseek (fd,hdrEnd + hdr.f_opthdr, SEEK_SET);

    /* read in section headers */

    if (pecoffSecHdrRead (fd, &scnHdr[0], &hdr, tablesAreLE) != OK)
	{
	errnoSet(S_loadLib_SCNHDR_READ);
	goto error;
	}

    /* Determine segment sizes */

    /* 
     * XXX seg.flagsXxx mustn't be used between coffSegSizes() and 
     * loadSegmentsAllocate().
     */

    pecoffSegSizes (&hdr, scnHdr, &seg);

    /* 
     * If object file is already absolutely located (by linker on host),
     *  then override parameters pText, pData and pBss.
     */

    if ((hdr.f_flags & F_EXEC) || (hdr.f_flags & F_RELFLG))
        {
        if (hdr.f_opthdr)               /* if there is an optional header */
            {
            fullyLinked = TRUE;
            pText = (INT8 *)(optHdr.text_start + optHdr.image_base);
            pData = (INT8 *)(optHdr.data_start + optHdr.image_base);

            /* bss follows data segment */
            pBss = (INT8 *) pData + optHdr.dsize;
            pBss += dataAlign(MAX_ALIGNMENT, pBss);
	    }
	}

    seg.addrText = pText;
    seg.addrData = pData;

    /*
     * If pBss is set to LD_NO_ADDRESS, change it to something else.
     * The pecoff loader allocates one large BSS segment later on, and
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
        pBss = LD_NO_ADDRESS;
        }

    /*  Ensure that section starts on the appropriate alignment.
     */
    pText += dataAlign (MAX_ALIGNMENT, pText);
    if (pData == LD_NO_ADDRESS)
        {
	pData = pText + seg.sizeText;
        }

    /*  Ensure that section starts on the appropriate alignment.
     */
    pData += dataAlign (MAX_ALIGNMENT, pData);

    seg.addrText = pText;
    seg.addrData = pData;

    /* load text and data sections */

    if (!fullyLinked)
        {
	if (pecoffLoadSections (fd, &scnHdr[0], &scnAddr[0], pText,
				pData) != OK)
	    {
	    errnoSet (S_loadLib_LOAD_SECTIONS);
	    goto error;
	    }

	/* get section relocation info */

        if (pecoffReadRelocInfo (fd, &scnHdr[0], &relsPtr[0],
				 tablesAreLE) != OK)
   	    {
	    errnoSet (S_loadLib_RELOC_READ);
	    goto error;
	    }
        }

    /* read symbols */

    if (pecoffReadExtSyms (fd, &externalSyms, &externalsBuf, &hdr,
			   tablesAreLE) != OK)
	{
	errnoSet (S_loadLib_EXTSYM_READ);
	goto error;
	}

    /* read string table */

    if (pecoffReadExtStrings (fd, &stringsBuf, tablesAreLE) != OK)
	{
	errnoSet (S_loadLib_EXTSTR_READ);
	goto error;
	}

    /*  Determine additional amount of memory required to append
     *  common symbols on to the end of the BSS section.
     */
    nbytes = pecoffTotalCommons(externalSyms, hdr.f_nsyms, seg.sizeBss);

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
	    addrBss += dataAlign (MAX_ALIGNMENT, addrBss);
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

    status = pecoffSymTab (fd, &hdr, (SYMENT *)externalSyms, &externalsBuf, 
			 symFlag, &seg, stringsBuf, symTbl, pCommons, scnHdr,
			 scnAddr, fullyLinked, group, optHdr.image_base);

    /* 
     * Relocate text and data segments
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
	    /*
	     * Only meaningful sections are relocated. Note that the section
	     * flags set for PECOFF do not allow to test for STYP_REG. The
	     * PE extension IMAGE_SCN_MEM_DISCARDABLE should be used instead.
	     */

	    if ((relsPtr[ix] != NULL)				  &&
                !(scnHdr[ix].s_flags & STYP_INFO)		  &&
                !(scnHdr[ix].s_flags & STYP_NOLOAD)		  &&
                !(scnHdr[ix].s_flags & IMAGE_SCN_MEM_DISCARDABLE) &&
                !(scnHdr[ix].s_flags & STYP_PAD))
		{
                if (pecoffRelSegmentI386 (relsPtr[ix], scnHdr, externalsBuf, 
                               externalSyms, &seg, ix) != OK)
		    {
	            goto error;
		    }
		}
	    }

    pecoffFree(externalsBuf, externalSyms, stringsBuf, relsPtr);

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
    pecoffFree(externalsBuf, externalSyms, stringsBuf, relsPtr);
    moduleDelete (moduleId);
    return (NULL);
    }

/******************************************************************************
*
* loadPecoffInit - initialize the system for pecoff load modules.
*
* This routine initializes VxWorks to use a PECOFF format for loading 
* modules.
*
* RETURNS: OK
*
* SEE ALSO: loadModuleAt()
*/

STATUS loadPecoffInit (void)
    {
    /* XXX check for installed? */
    loadRoutine = (FUNCPTR) ldPecoffModAtSym;
    return (OK);
    }

/*******************************************************************************
*
* pecoffFree - free any malloc'd memory
*
*/

LOCAL void pecoffFree
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
* pecoffSegSizes - determine segment sizes
*
* This function fills in the size fields in the SEG_INFO structure.
* Note that the bss size may need to be readjusted for common symbols. 
*/

LOCAL void pecoffSegSizes
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

            nbytes = dataAlign(MAX_ALIGNMENT, pSeg->sizeText);
            pSeg->sizeText += pScnHdr->s_size + nbytes;

	    /* SPR #21836 */

	    if (MAX_ALIGNMENT > pSeg->flagsText)
		pSeg->flagsText = MAX_ALIGNMENT;
            }
        else if (pScnHdr->s_flags & STYP_DATA)
            {
            nbytes = dataAlign(MAX_ALIGNMENT, pSeg->sizeData);
            pSeg->sizeData += pScnHdr->s_size + nbytes;

	    /* SPR #21836 */

	    if (MAX_ALIGNMENT > pSeg->flagsData)
		pSeg->flagsData = MAX_ALIGNMENT;
            }
        else if (pScnHdr->s_flags & STYP_BSS)
            {
            nbytes = dataAlign(MAX_ALIGNMENT, pSeg->sizeBss);
            pSeg->sizeBss += pScnHdr->s_size + nbytes;

	    /* SPR #21836 */

	    if (MAX_ALIGNMENT > pSeg->flagsBss)
		pSeg->flagsBss = MAX_ALIGNMENT;
            }
        }
    }

/*******************************************************************************
*
* pecoffLoadSections - read sections into memory
* 
*/

LOCAL STATUS pecoffLoadSections
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

    for (ix = 0; ix < max_scns; ix++)
	{
        pLoad = NULL;
        size = pScnHdr->s_size;
	if (size != 0 && pScnHdr->s_scnptr != 0)
	    {
            if (pScnHdr->s_flags & STYP_TEXT)
                {
                pLoad = pText;
                nbytes = dataAlign(MAX_ALIGNMENT, pLoad);
                pLoad += nbytes;
                pText = (char *) ((int) pLoad + size); /* for next load */
                textCount += size + nbytes;
                }
            else if (pScnHdr->s_flags & STYP_DATA)
                {
                pLoad = pData;
                nbytes = dataAlign(MAX_ALIGNMENT, pLoad);
                pLoad += nbytes;
                pData = (char *) ((int) pLoad + size); /* for next load */
                dataCount += size + nbytes;
                }
            else
                {
                /* ignore all other sections */
                pScnAddr[ix] = pLoad;
      	        pScnHdr++;
		continue;
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
        pScnAddr[ix] = pLoad;
	pScnHdr++;
	}
    return (OK);
    }

/*******************************************************************************
*
* loadPecoffCommonManage - process a common symbol
*
* This routine processes the common symbols found in the object module.
* Common symbols are symbols declared global without being assigned a
* value.  
*
* loadPecoffCommonManage() is derived from loadCommonMange() in
* loadLib.c, but unlike loadCommonManage(), loadPecoffCommonManage() does
* not allocate memory or add symbols to the symbol table:
* loadPecoffCommonManage() leaves the chores of allocating memory
* or adding symbols to the symbol table up to the caller.
*
* For more information on common symbol types,LOAD_COMMON_MATCH_NONE,
* LOAD_COMMON_MATCH_USER and LOAD_COMMON_MATCH_ALL see
* loadCommonManage() in loadLib.c.
*
* NOMANUAL 
*/

LOCAL STATUS loadPecoffCommonManage
    (
    char *       symName,        /* symbol name */
    SYMTAB_ID    symTbl,        /* target symbol table */
    SYM_ADRS *   pSymAddr,       /* where to return symbol's address */
    SYM_TYPE *   pSymType,       /* where to return symbol's type */
    int          loadFlag       /* control of loader's behavior */
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

        /* Get the last occurences of matching global symbols in bss and data  */

        loadCommonMatch (&commInfo, symTbl);

        /* Prefered order for matching symbols is : bss then data */

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

        /* If we found a matching symbol, stop here */

        if(*pSymAddr != NULL)
            return (OK);
	}
    
    /* if a matching common symbol is not found, return ERROR*/

    return (ERROR);
    }


/*******************************************************************************
*
* pecoffSymTab - process an object module symbol table
*
* This is passed a pointer to a pecoff symbol table and processes
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

LOCAL STATUS pecoffSymTab
    (
    int		fd,		/* file descriptor of module being loaded */
    FILHDR *	pHdr,		/* pointer to file header */
    SYMENT *	externalSyms,	/* pointer to symbols array */
    char ***	externals,	/* pointer to pointer to array to fill in 
				   symbol absolute values */
    int		symFlag,	/* symbols to be added to table 
				 *   ([NO|GLOBAL|ALL]_SYMBOLS) */
    SEG_INFO *	pSeg,		/* pointer to segment information */
    char *	symStrings,	/* symbol string table */
    SYMTAB_ID	symTbl,		/* symbol table to use */
    char *	pNextCommon,	/* next common address in bss */
    SCNHDR *	pScnHdrArray,	/* pointer to PECOFF section header array */
    char **	pScnAddr,	/* pointer to section loaded address array */
    BOOL	fullyLinked,	/* TRUE if module already absolutely located */
    UINT16	group,		/* module's group */
    int		imageBase	/* base addr of the simulateur image in RAM */
    )
    {
    int		status  = OK;	/* return status */
    char *	name;		/* symbol name (plus EOS) */
    SYMENT *	symbol;		/* symbol struct */
    SYM_TYPE	vxSymType;	/* vxWorks symbol type */
    int		symNum;		/* symbol index */
    char *	adrs;		/* table resident symbol address */
    char *	bias;		/* section relative address */
    ULONG	bssAlignment;	/* alignment value of common type */
    long	scnFlags;	/* flags from section header */
    int		auxEnts = 0;	/* auxiliary symbol entries to skip */
    SCNHDR *	pScnHdr = NULL;	/* pointer to a PECOFF section header */
    char *	scnAddr = NULL;	/* section loaded address */
    char	nameTemp[SYMNMLEN+1]; /* temporary for symbol name string */

    /*  Loop thru all symbol table entries in object file */

    for (symNum = 0, symbol = externalSyms; symNum < pHdr->f_nsyms; 
	 symbol++, symNum++)
	{
        if (auxEnts)                    /* if this is an auxiliary entry */
            {
            auxEnts--;
            continue;                   /* skip it */
            }
        auxEnts = symbol->n_numaux;      /* # of aux entries for this symbol */

	/* Get rid of debug symbols */

        if (symbol->n_scnum == N_DEBUG)
            continue;

	/* Setup pointer to symbol name string */

        if (symbol->n_zeroes)            /* if name is in symbol entry */
            {
            name = symbol->n_name;
            if (*(name + SYMNMLEN -1) != '\0')
                {
                /*
		 * If the symbol name array is full,
                 * the string is not terminated by a null.
                 * So, move the string to a temporary array,
                 * where it can be null terminated.
                 */

                bcopy (name, nameTemp, SYMNMLEN);
                nameTemp[SYMNMLEN] = '\0';
                name = nameTemp;
                }
            }
        else
            name = symStrings + symbol->n_offset;


        if (! PECOFF_UNDF(symbol) && ! PECOFF_COMM(symbol))
            {
            /*
	     * The symbol is a defined, local or global (though not common).
             *
             * Determine symbol's section and address bias
             */

            if (symbol->n_scnum > 0)
                {
                pScnHdr = pScnHdrArray + (symbol->n_scnum - 1);
		scnAddr = pScnAddr[symbol->n_scnum - 1];
                scnFlags = pScnHdr->s_flags;
                }
            else
                scnFlags = 0;       /* section has no text, data or bss */

            if (symbol->n_scnum == N_ABS) /* special check for absolute syms */
                {
		/*
		 * No bias is applied to absolute symbols: they have to be
		 * valid for the actual location in Window's memory space
		 * where the simulator is installed.
		 *
		 * PLEASE, DO NOT CHANGE THIS AS THIS IS A CONSCIOUS AND
		 * DOCUMENTED DECISION.
		 */

                bias = 0;
                vxSymType = VX_ABS;
                }
            else if (scnFlags & STYP_TEXT)
                {
                bias = (char *) scnAddr;
                vxSymType = VX_TEXT;
		}
            else if (scnFlags & STYP_DATA)
                {
                bias = (char *) scnAddr;
                vxSymType = VX_DATA;
		}
            else if (scnFlags & STYP_BSS)
                {
                bias = (char *) scnAddr;
                vxSymType = VX_BSS;
		}
            else if (scnFlags & STYP_DRECTVE)
                {
		/* Ignore this section */

                continue;
                }
            else
                {
		/*
		 * Lets not print error messages for debug-related symbols
		 * that are not flagged as N_DEBUG...
		 */

		if (!strcmp (name, ".stab") || !strcmp (name, ".stabstr"))
		    continue;

                printErr (cantConvSymbolType, name, errnoGet());
                continue;
                }

	    /*
	     * Since, in the VxWorks simulator's image all the symbols' values
	     * are offsets from zero a bias has to be introduced to get the
	     * real (post-installation) addresses of these symbols in
	     * Windows' memory space. The bias computation is however
	     * different from the one for the regular object modules and must
	     * overwrite whatever has been computed just above: the bias is
	     * the base address of the simulator in Windows' memory space
	     * plus the offset to the section which holds the symbol.
	     *
	     * Note: the section header's s_vaddr field holds this offset
	     *       _not_ a real virtual address...
	     */

            if (fullyLinked)
		bias = (char *)(pScnHdr->s_vaddr + imageBase);

            /*  Determine if symbol should be put into symbol table */

            if (((symFlag & LOAD_LOCAL_SYMBOLS) && !PECOFF_EXT(symbol))
                || ((symFlag & LOAD_GLOBAL_SYMBOLS) && PECOFF_EXT(symbol)))
                {
                if (PECOFF_EXT(symbol))
                    vxSymType |= VX_EXT;

                /*  Add symbol to symbol table but cut out Local tags */

                if (name[0] != '$')
    		    if (symSAdd (symTbl, name, symbol->U_SYM_VALUE + bias,
			     vxSymType, group) != OK)
		        printErr (cantAddSymErrMsg, name, errnoGet());
                }

            /* Add symbol address to externals table.
             * For PECOFF, we add all symbol addresses into the externals
             * table, not only those symbols saved in the vxWorks symbol
             * table.
             */

            (*externals) [symNum] = symbol->U_SYM_VALUE + bias;
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

            if (PECOFF_COMM(symbol))
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
		     *  This portion of code checks for a matching
                     *  common symbol. If a matching common symbol is not
                     *  found the symbol value is read
		     *  (which contains the symbol size) and the symbol is
                     *  placed in the bss section.  The function dataAlign uses
		     *  the next possible address for a common symbol to
		     *  determine the proper alignment.
		     */
		      
                    if (loadPecoffCommonManage (name, symTbl, (void *) &adrs,
                        &vxSymType, symFlag) != OK)
	                {
	                adrs = pNextCommon;
	                bssAlignment = dataAlign (symbol->U_SYM_VALUE,
                                                  (ULONG)adrs);
	                adrs += bssAlignment;
	                pNextCommon += (symbol->U_SYM_VALUE + bssAlignment);
		    
	                if (symSAdd (symTbl, name, adrs, (VX_BSS | VX_EXT),
	                    group) != OK)
		            printErr (cantAddSymErrMsg, name, errnoGet());
		        }
		    }
		}

	    /* look up undefined external symbol in symbol table */

	    else if (symFindByNameAndType (symTbl, name, &adrs, &vxSymType,
					   VX_EXT, VX_EXT) != OK)
                {
                /* Symbol not found in symbol table */

                printErr ("undefined symbol: %s\n", name);
                adrs = NULL;
                status = ERROR;
                }

	    /* Add symbol address to externals table */

	    (*externals) [symNum] = adrs;
	    }
	}
		    
    return status;
    }

/*******************************************************************************
*
* pecoffRelSegmentI386 - perform relocation for the I386 family
*
* This routine reads the specified relocation command segment and performs
* all the relocation specified therein.
* Absuolute symbol addresses are looked up in the 'externals' table.
*
* This function handles the following types of relocation commands for the
* I386 family of processor:
* 	IMAGE_REL_I386_DIR32  - direct 32 bit relocation
* 	IMAGE_REL_I386_REL32  - relative 32b bit relocation
*
* RETURNS: OK or ERROR
*/

LOCAL STATUS pecoffRelSegmentI386
    (
    RELOC *	pRelCmds,	/* list of relocation commands */
    SCNHDR *	pScnHdrArray, 	/* array of section headers */
    char **	pExternals,	/* array of absolute symbols values */
    SYMENT *	pExtSyms,	/* pointer to object file symbols */ 
    SEG_INFO * 	pSeg,		/* section addresses and sizes */
    int         section         /* section number -1 for relocation */
    )
    {
    long *      pAdrs = NULL;   /* relocation address */
    int         nCmds;          /* # reloc commands for seg */
    RELOC       relocCmd;       /* relocation structure */
    SCNHDR *    pScnHdr;        /* section header for relocation */
    STATUS      status = OK;

    pScnHdr = pScnHdrArray + section;

    for (nCmds = pScnHdr->s_nreloc; nCmds > 0; nCmds--)
        {
        /* read relocation command */

        relocCmd = *pRelCmds++;

	/*
         * Calculate actual address in memory that needs relocation.
	 * XXX PAD - we now apply the fix for SPR 70767 
         */

	if (pScnHdr->s_flags & STYP_TEXT)
	    {
	    if (!(pScnHdr->s_flags & STYP_LIT))
		{
		pAdrs = (long *)((long) pSeg->addrText + relocCmd.r_vaddr);
		}
	    else
		{
		printf ("STYP_LIT type sections are unsupported.\n");
		return ERROR;
		}
	    }
	else
	    /*
	     * The computation of the address of relocable data symbol 
             * should take into account the offset from the beginning of the
             * file that the compiler toolchain may require to account for
             * in some cases. The code was fixed to substract this offset
             * for data symbol, SPR 73145. 
	     */

	    {
	    pAdrs = (long *)((long) pSeg->addrData +
		             (relocCmd.r_vaddr - pScnHdr->s_vaddr));
	    }

	/* do relocations */

        switch (relocCmd.r_type)
            {
	    case IMAGE_REL_I386_DIR32:
		/*
		 * This relocation is preformed by adding the absolute address
		 * of the symbol to the relocation value in the code.
		 */
		*pAdrs += (INT32) pExternals [relocCmd.r_symndx];
  		break;

	    case IMAGE_REL_I386_REL32:
		/*
		 * Call near, displacement relative to the next instruction.
		 * First, find the next instruction addr, then subtract it from
		 * the addr of the found symbol to obtain the relocation addr.
		 */
	
                *pAdrs = (UINT32)pExternals[relocCmd.r_symndx] -
		         ((UINT32)pAdrs + 4);
                break;

	    default:
		printf("Unknown Relocation Error\n");
		errnoSet (S_loadLib_UNRECOGNIZED_RELOCENTRY);
		status = ERROR;
		break;
            }
        }

    return status;
    }

/*******************************************************************************
*
* pecoffHdrRead - read in PECOFF header and swap if necessary
* 
* Note:  To maintain code portability, we can not just read sizeof(FILHDR) 
*	bytes.  Compilers pad structures differently,
*	resulting in different structure sizes.
*	So, we read the structure in an element at a time, using the size
*	of each element.
*/

LOCAL STATUS pecoffHdrRead
    (
    int fd,
    FILHDR *pHdr,
    BOOL *pSwap
    )
    {
    int status;
    int i;
    struct dosheader dhead;

    *pSwap = 0;

    ioctl(fd, FIOSEEK, 0);

    if (fioRead (fd, (char *) &(pHdr->f_magic), sizeof (pHdr->f_magic))
		!= sizeof (pHdr->f_magic))
	{
	errnoSet (S_loadLib_FILE_READ_ERROR);
	return ERROR;
	}

    switch (pHdr->f_magic)
	{
	case (SWAB_SHORT(IMAGE_DOS_SIGNATURE)):
            *pSwap = TRUE;

	case (IMAGE_DOS_SIGNATURE):
	    /* Remove DOS EXE header */

            fioRead(fd,(char *)&dhead + 2,sizeof(dhead)-2);
	    lseek (fd, dhead.e_lfanew, SEEK_SET);

            /* Check for NT SIGNATURE */

            fioRead (fd, (char *)&i, 4);
            if ( i != IMAGE_NT_SIGNATURE )
                {
                printf("Bad Image Signature %x %x\n",i,IMAGE_NT_SIGNATURE);
		errnoSet (S_loadLib_HDR_READ);
                return ERROR;
                }
            fioRead (fd, (char *)&pHdr->f_magic, 2);
	    break;

	case (SWAB_SHORT(IMAGE_FILE_MACHINE_I386)):
            *pSwap = TRUE;
	    break;

        case (IMAGE_FILE_MACHINE_I386):
	    *pSwap = FALSE;
	    break;
	    
	default:
#ifndef MULTIPLE_LOADERS
	    printErr (fileTypeUnsupported, pHdr->f_magic);
#endif
	    errnoSet (S_loadLib_FILE_ENDIAN_ERROR);
	    return ERROR;
	    break;
	}

    status = fileRead (fd, &pHdr->f_nscns, sizeof(pHdr->f_nscns), *pSwap);
    status |= fileRead (fd, &pHdr->f_timdat, sizeof(pHdr->f_timdat), *pSwap);
    status |= fileRead (fd, &pHdr->f_symptr, sizeof(pHdr->f_symptr), *pSwap);
    status |= fileRead (fd, &pHdr->f_nsyms, sizeof(pHdr->f_nsyms), *pSwap);
    status |= fileRead (fd, &pHdr->f_opthdr, sizeof(pHdr->f_opthdr), *pSwap);
    status |= fileRead (fd, &pHdr->f_flags, sizeof(pHdr->f_flags), *pSwap);

    max_scns = pHdr->f_nscns;

    return status;
    }

/*******************************************************************************
*
* pecoffOpthdrRead - read in PECOFF optional header and swap if necessary
* 
*/

LOCAL STATUS pecoffOpthdrRead
    (
    int     	fd,
    PEOPTION 	*pOptHdr,
    BOOL    	swapTables
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
    status |= fileRead(fd, &pOptHdr->image_base, sizeof(pOptHdr->image_base),
                            swapTables);
    status |= fileRead(fd, &pOptHdr->section_align, sizeof(pOptHdr->section_align),
                            swapTables);
    status |= fileRead(fd, &pOptHdr->file_align, sizeof(pOptHdr->file_align),
                            swapTables);

    return (status);
    }


/*******************************************************************************
*
* pecoffSecHdrRead - read in PECOFF section header and swap if necessary
* 
*/

LOCAL STATUS pecoffSecHdrRead
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

	/*
	 * In order to comply with an obscure requirement from Microsoft, the
	 * GNU toolchain puts the size of the bss section in the section
	 * header's physical address field. This piece of code addresses the
	 * SPR 70767 by reseting the s_size field as appropriate for the rest
	 * of the loader code.
	 */

	if (pScnHdr->s_flags & STYP_BSS)
	    {
	    if ((pScnHdr->s_size == 0) && (pScnHdr->s_paddr != 0))
		pScnHdr->s_size = pScnHdr->s_paddr;
	    }

	pScnHdr++;
	}

    return (status);
    }

/*******************************************************************************
*
* pecoffReadRelocInfo - read in PECOFF relocation info and swap if necessary
* 
* Assumptions:  The file pointer is positioned at the start of the relocation
*		records.
*
*		The relocation records are ordered by section.
*/

LOCAL STATUS pecoffReadRelocInfo
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
* pecoffReadSym - read one PECOFF symbol entry
* 
*/

LOCAL STATUS pecoffReadSym
    (
    int     fd,
    SYMENT *pSym,                       /* where to read symbol entry */
    BOOL    swapTables
    )
    {
    int status;

    status = fioRead (fd, pSym->n_name, sizeof(pSym->n_name));
    if(status != sizeof(pSym->n_name))
        {
        printf(" Status %x %x : %x %x %x %x\n",status,sizeof(pSym->n_name),
            pSym->n_name[0], 
            pSym->n_name[1], 
            pSym->n_name[2], 
            pSym->n_name[3]);
    status = fioRead (fd, pSym->n_name, sizeof(pSym->n_name));
        printf(" Status  #2 %x : %x %x %x %x\n",status,
            pSym->n_name[0], 
            pSym->n_name[1], 
            pSym->n_name[2], 
            pSym->n_name[3]);
	status = ERROR;	
        }
    else
        status = OK;

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
    status |= fileRead (fd, &pSym->n_type, sizeof(pSym->n_type),
                        swapTables);
    status |= fileRead (fd, &pSym->n_sclass, sizeof(pSym->n_sclass),
                        swapTables);
    status |= fileRead (fd, &pSym->n_numaux, sizeof(pSym->n_numaux),
                        swapTables);
    return(status);
    }


/*******************************************************************************
*
* pecoffReadExtSyms - read PECOFF symbols
* 
*  For low memory systems, we can't afford to keep the entire
*  symbol table in memory at once.
*  For each symbol entry, the fields required for relocation
*  are saved in the externalSyms array.
*/

LOCAL STATUS pecoffReadExtSyms
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
            if ((status = pecoffReadSym(fd, pSym, swapTables) != OK))
		return (status);
            }
      }
    return (OK);
    }
/*******************************************************************************
*
* pecoffReadExtStrings - read in PECOFF external strings
* 
* Assumptions:  The file pointer is positioned at the start of the
*		string table.
*/
#define STR_BYTES_SIZE 4                /* size of string table length field */

LOCAL STATUS pecoffReadExtStrings
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

#if FALSE /* XXX PAD - this is no longer used apparently */
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
#ifdef NO_PECOFF_SOFT_SEEK
    if (ioctl (fd, FIOSEEK, startOfFileOffset) == ERROR)
        return (ERROR);
#endif
    return(OK);
    }
#endif /* FALSE */

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
* pecoffTotalCommons - 
* 
*  INTERNAL
*  All common external symbols are being tacked on to the end
*  of the BSS section.  This function determines the additional amount
*  of memory required for the common symbols.
*  
*/

LOCAL ULONG pecoffTotalCommons
    (
    SYMENT *pSym,	/* pointer to external symbols */
    int nEnts,		/* # of entries in symbol table */
    ULONG startAddr	/* address to start from */
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

	if (PECOFF_COMM(pSym))
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
* bootPecoffModule - load an object module into memory
*
* This routine loads an object module in PECOFF format from the specified
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

STATUS bootPecoffModule
    (
    int	fd,				/* fd from which to read load module */
    FUNCPTR *pEntry			/* entry point of module */
    )
    {
    FILHDR hdr;				/* module's PECOFF header */
    PEOPTION optHdr;             	/* module's PECOFF optional header */
    int nBytes;
    char tempBuf[MAX_SCNS * SCNHSZ];
    int tablesAreLE;

    /* read object module header */

    if (pecoffHdrRead (fd, &hdr, &tablesAreLE) != OK)
	return (ERROR);

    /*  read in optional header */

    if (pecoffOpthdrRead (fd, &optHdr, tablesAreLE) != OK)
	return (ERROR);

    /*  Read until start of text
     *
     *  Section headers aren't needed, but this is the easiest way to
     *  read passed them.
     */
    if (pecoffSecHdrRead (fd, (SCNHDR *)tempBuf, &hdr, tablesAreLE) != OK)
	{
	return(ERROR);
	}

    printf ("%d", (int)optHdr.tsize);
    nBytes = fioRead (fd, (char *) optHdr.text_start, (int) optHdr.tsize);
    if (nBytes < optHdr.tsize)
        return (ERROR);

    printf (" + %d", (int)optHdr.dsize);
    nBytes = fioRead (fd, (char *) optHdr.data_start, (int) optHdr.dsize);
    if (nBytes < optHdr.dsize)
        return (ERROR);

    printf (" + %d\n", (int)optHdr.bsize);

    /*  bss is zeroed by vxWorks startup code. 
     */

    *pEntry = (FUNCPTR) optHdr.entry;
    return (OK);
    }

/******************************************************************************
*
* bootPecoffInit - Initialize the system for pecoff load modules
*
* This routine initialized VxWorks to use the PECOFF format for
* boot loading modules.
*
* RETURNS:
* OK, or
* ERROR if XXX
*
* SEE ALSO: loadModuleAt()
*/
STATUS bootPecoffInit
    (
    )
    {
    /* XXX check for installed ? */
    bootLoadRoutine = bootPecoffModule;
    return (OK);
    }

#endif /* CPU==SIMNT */

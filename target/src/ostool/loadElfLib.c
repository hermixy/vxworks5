/* loadElfLib.c - UNIX elf object module loader */

/* Copyright 1996-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01x,09may02,fmk  use loadCommonManage() instead of loadElfCommonManage()
01w,23apr02,jn   SPR 75177 - correct inaccurate test for whether CPU is
                 SIMSPARCSOLARIS 
01v,26mar02,jn   fix alignment handling for COMMON's (SPR 74567)
01u,07mar02,jn   Add STT_ARM_16BIT to recognized types (SPR # 73992)
01t,07mar02,jn   Refix SPR# 30588 - load should return NULL when there are
                 unresolved symbols.
01s,14jan02,jn   Improve comments about setting SYM_THUMB
01r,25jan02,rec  Merge in coldfire changes
01q,12dec01,pad  Moved MIPS support code out in the ELF/MIPS relocation unit.
01p,28nov01,jn   Fix alignment of segments (SPR #28353).  Also removed 
                 declarations of unsupported relocation types for SH.
01o,19nov01,pch  Provide misalignment prevention based on _WRS_STRICT_ALIGNMENT
		 definition instead of testing a specific CPU type.
01n,08nov01,jn   Added support for ARM/THUMB architecture now using ELF
01m,17sep01,pad  Introduced usage of relocation unit's init routines. Added
		 support for I86 relocation unit. Symbols from a read-only data
 		 sections are now flagged as data, like in VxWorks AE, rather
		 than text. Made the determination of text sections more
		 flexible in loadElfSegSizeGet().
01l,28jun01,agf  add logic to MIPS that tests for attempted relative jumps
                 across a 26 bit boundary
01k,03mar00,zl   merged SH support from T1
01j,30nov98,dbt  no longer clear seg.flags<xxx> after loadSegmentAllocate()
                 call. (SPR #23553).
01i,05oct98,pcn  Initialize all the fields in the SEG_INFO structure.
01h,16sep98,pcn  Set to _ALLOC_ALIGN_SIZE the flags field in seg structure
                 (SPR #21836).
01g,17jul98,pcn  Fixed SPR #21836: alignment mismatch between sections and
                 target.
01f,18apr97,kkk  fixed printing of debug msgs for MIPS (spr# 8276)
01e,05dec96,dbt  fixed a bug in loadElfSizeGet() when loading an object file
                 with a data section null and a text and bss sections not
                 null.
01d,31oct96,elp  Replaced symAdd() call by symSAdd() call (symtbls synchro).
01c,22oct96,dbt  Zero out bss section (fixed SPR #7376).
01c,02oct96,dbt  Added support for SIMSPARCSOLARIS.
01b,02oct96,dbt  Unknown moduleformats are now correctly managed (SPR #7263).
01b,03aug96,kkk  throw away __gnu_compiled_c signatures.
01a,20jun96,dbt	 created from /host/src/tgtsvr/server/loadelf.c v01t
		 and /host/src/tgtsvr/server/elfppc.c v01e
		 and /host/src/tgtsvr/server/elfmips.c v01c
*/

/*
DESCRIPTION
This library provides an object module loading facility.  Any SYSV elf
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
This code fragment would load the ELF file "objFile" located on
device "/devX/" into memory which would be allocated from the system
memory pool.  All external and static definitions from the file would be
added to the system symbol table.

This could also have been accomplished from the shell, by typing:
.CS
    -> ld (1) </devX/objFile
.CE

INCLUDE FILE: loadElfLib.h

SEE ALSO: loadLib, usrLib, symLib, memLib,
.pG "Basic OS"
*/

/* defines */

#undef INCLUDE_SDA		/* SDA is not supported for the moment */
#define _CACHE_SUPPORT         	/* cache           */

/* includes */

#include "vxWorks.h"
#include "stdio.h"
#include "loadElfLib.h"
#include "elf.h"
#include "elftypes.h"
#include "ioLib.h"
#include "fioLib.h"
#include "bootLoadLib.h"
#include "loadLib.h"
#include "memLib.h"
#include "pathLib.h"
#include "string.h"
#include "symLib.h"
#include "sysSymTbl.h"
#ifdef  _CACHE_SUPPORT
#include "cacheLib.h"
#endif  /* _CACHE_SUPPORT */
#include "errnoLib.h"
#include "stdlib.h"
#include "symbol.h"     /* for SYM_TYPE typedef */
#include "moduleLib.h"
#include "private/vmLibP.h"

#if   ((CPU_FAMILY == MIPS) || (CPU_FAMILY == PPC) || \
       (CPU_FAMILY == SIMSPARCSOLARIS) || (CPU_FAMILY == SH) || \
       (CPU_FAMILY == I80X86) || (CPU_FAMILY == ARM) || \
       (CPU_FAMILY==COLDFIRE))

/* define */

#ifndef EM_ARCH_MACHINE
#define EM_ARCH_MACHINE -1              /* default */
#endif /* EM_ARCH_MACHINE */

#undef DEBUGG
#ifdef DEBUGG
int elfDebug=1;
#define DSMINST(x)
#define DBG(x) if (elfDebug) { printf x; fflush(stdout); }
#else
#define DSMINST(x)
#define DBG(x)
#endif /* DEBUG */

#ifdef	_WRS_STRICT_ALIGNMENT
#define ELFOUTMSBU32(b,w) \
  ((b)[0] = (w >> 24), \
   (b)[1] = (w >> 16), \
   (b)[2] = (w >> 8), \
   (b)[3]  = w)
#endif /* _WRS_STRICT_ALIGNMENT */

/* globals */

/* externals */
	
#ifdef INCLUDE_SDA
IMPORT char SDA_BASE[];		/* Base address of SDA Area */
IMPORT char SDA2_BASE[];	/* Base address of SDA2 Area */
IMPORT int SDA_SIZE;		/* size of SDA area */
IMPORT int SDA2_SIZE;		/* size of SDA2 area */
#endif /* INCLUDE_SDA */

/* locals */

#ifdef INCLUDE_SDA
LOCAL STATUS moduleElfSegAdd (MODULE_ID moduleId, int type, void * location,
    				int length, int flags, PART_ID memPartId);

LOCAL BOOL segElfFindByType (SEGMENT_ID segmentId, MODULE_ID moduleId,
    				int type);
#endif /* INCLUDE_SDA */

LOCAL UINT32 loadElfAlignGet (UINT32 alignment, void * pAddrOrSize);

LOCAL BOOL (* pElfModuleVerifyRtn) (UINT32 machType,
				    BOOL * sda) = NULL; /* verif rtn ptr */

LOCAL STATUS (* pElfSegRelRtn) (int fd, MODULE_ID moduleId, int loadFlag,
		    		int posCurRelocCmd, Elf32_Shdr * pScnHdrTbl,
			    	Elf32_Shdr * pRelHdr, SCN_ADRS * pScnAddr,
	    			SYM_INFO_TBL symInfoTbl,
			       	Elf32_Sym * pSymsArray, SYMTAB_ID symTbl,
				SEG_INFO * pSeg) = NULL; /* seg reloc rtn ptr */

#if     (CPU_FAMILY == PPC)

LOCAL STATUS elfPpcSegReloc (int fd, MODULE_ID moduleId, int loadFlag,
		    		int posCurRelocCmd, Elf32_Shdr * pScnHdrTbl,
			    	Elf32_Shdr * pRelHdr, SCN_ADRS * pScnAddr,
	    			SYM_INFO_TBL symInfoTbl,
				Elf32_Sym *  pSymsArray,
			    	SYMTAB_ID    symTbl,
				SEG_INFO *   pSeg);
LOCAL STATUS elfPpcAddr32Reloc (void * pAdrs, Elf32_Rela * pRelocCmd, 
				SYM_INFO_TBL symInfoTbl, MODULE_ID moduleId);
LOCAL STATUS elfPpcAddr24Reloc (void * pAdrs, Elf32_Rela * pRelocCmd, 
				SYM_INFO_TBL symInfoTbl);
LOCAL STATUS elfPpcAddr16Reloc (void * pAdrs, Elf32_Rela * pRelocCmd, 
				SYM_INFO_TBL symInfoTbl);
LOCAL STATUS elfPpcAddr16LoReloc (void * pAdrs, Elf32_Rela * pRelocCmd, 
				SYM_INFO_TBL symInfoTbl);
LOCAL STATUS elfPpcAddr16HiReloc (void * pAdrs, Elf32_Rela * pRelocCmd, 
				SYM_INFO_TBL symInfoTbl);
LOCAL STATUS elfPpcAddr16HaReloc (void * pAdrs, Elf32_Rela * pRelocCmd, 
				SYM_INFO_TBL symInfoTbl);
LOCAL STATUS elfPpcAddr14Reloc (void * pAdrs, Elf32_Rela * pRelocCmd, 
				SYM_INFO_TBL symInfoTbl);
LOCAL STATUS elfPpcRel24Reloc (void * pAdrs, Elf32_Rela * pRelocCmd, 
				SYM_INFO_TBL symInfoTbl);
LOCAL STATUS elfPpcRel14Reloc (void * pAdrs, Elf32_Rela * pRelocCmd, 
				SYM_INFO_TBL symInfoTbl);
LOCAL STATUS elfPpcUaddr32Reloc (void * pAdrs, Elf32_Rela * pRelocCmd, 
				SYM_INFO_TBL symInfoTbl);
LOCAL STATUS elfPpcUaddr16Reloc (void * pAdrs, Elf32_Rela * pRelocCmd, 
				SYM_INFO_TBL symInfoTbl);
LOCAL STATUS elfPpcRel32Reloc (void * pAdrs, Elf32_Rela * pRelocCmd, 
				SYM_INFO_TBL symInfoTbl);
LOCAL STATUS elfPpcEmbNaddr32Reloc (void * pAdrs, Elf32_Rela * pRelocCmd, 
				SYM_INFO_TBL symInfoTbl);
LOCAL STATUS elfPpcEmbNaddr16Reloc (void * pAdrs, Elf32_Rela * pRelocCmd, 
				SYM_INFO_TBL symInfoTbl);
LOCAL STATUS elfPpcEmbNaddr16LoReloc (void * pAdrs, Elf32_Rela * pRelocCmd, 
				SYM_INFO_TBL symInfoTbl);
LOCAL STATUS elfPpcEmbNaddr16HiReloc (void * pAdrs, Elf32_Rela * pRelocCmd, 
				SYM_INFO_TBL symInfoTbl);
LOCAL STATUS elfPpcEmbNaddr16HaReloc (void * pAdrs, Elf32_Rela * pRelocCmd, 
				SYM_INFO_TBL symInfoTbl);
#ifdef INCLUDE_SDA
LOCAL STATUS elfPpcSdaRel16Reloc (void * pAdrs, Elf32_Rela * pRelocCmd, 
				SYM_INFO_TBL symInfoTbl, SEG_INFO * pSeg);
LOCAL STATUS elfPpcEmbSda21Reloc (void * pAdrs, Elf32_Rela * pRelocCmd, 
				SYM_INFO_TBL symInfoTbl, SEG_INFO * pSeg);
LOCAL STATUS elfPpcEmbSda2RelReloc (void * pAdrs, Elf32_Rela * pRelocCmd, 
				SYM_INFO_TBL symInfoTbl, SEG_INFO * pSeg);
LOCAL STATUS elfPpcEmbRelSdaReloc (void * pAdrs, Elf32_Rela * pRelocCmd, 
				SYM_INFO_TBL symInfoTbl, SEG_INFO * pSeg);
#endif /* INCLUDE_SDA */
#endif     /* CPU_FAMILY == PPC */

#if	(CPU_FAMILY == SIMSPARCSOLARIS)

LOCAL STATUS elfSparcSegReloc (int fd, MODULE_ID moduleId, int loadFlag, 
			      int posCurRelocCmd, Elf32_Shdr * pScnHdrTbl,
    			      Elf32_Shdr * pRelHdr, SCN_ADRS * pScnAddr,
    			      SYM_INFO_TBL symInfoTbl, Elf32_Sym * pSymsArray,
    			      SYMTAB_ID symTbl, SEG_INFO * pSeg);
LOCAL STATUS elfSparc32Reloc (void * pAdrs, Elf32_Rela * pRelocCmd, 
				SYM_INFO_TBL symInfoTbl);
LOCAL STATUS elfSparc16Reloc (void * pAdrs, Elf32_Rela * pRelocCmd, 
				SYM_INFO_TBL symInfoTbl);
LOCAL STATUS elfSparc8Reloc (void * pAdrs, Elf32_Rela * pRelocCmd, 
				SYM_INFO_TBL symInfoTbl);
LOCAL STATUS elfSparcDisp32Reloc (void * pAdrs, Elf32_Rela * pRelocCmd,
				SYM_INFO_TBL symInfoTbl);
LOCAL STATUS elfSparcDisp16Reloc (void * pAdrs, Elf32_Rela * pRelocCmd, 
				SYM_INFO_TBL symInfoTbl);
LOCAL STATUS elfSparcDisp8Reloc (void * pAdrs, Elf32_Rela * pRelocCmd, 
				SYM_INFO_TBL symInfoTbl);
LOCAL STATUS elfSparcWDisp30Reloc (void * pAdrs, Elf32_Rela * pRelocCmd, 
				SYM_INFO_TBL symInfoTbl);
LOCAL STATUS elfSparcWDisp22Reloc (void * pAdrs, Elf32_Rela * pRelocCmd, 
				SYM_INFO_TBL symInfoTbl);
LOCAL STATUS elfSparcHi22Reloc (void * pAdrs, Elf32_Rela * pRelocCmd, 
				SYM_INFO_TBL symInfoTbl);
LOCAL STATUS elfSparc22Reloc (void * pAdrs, Elf32_Rela * pRelocCmd, 
				SYM_INFO_TBL symInfoTbl);
LOCAL STATUS elfSparc13Reloc (void * pAdrs, Elf32_Rela * pRelocCmd, 
				SYM_INFO_TBL symInfoTbl);
LOCAL STATUS elfSparcLo10Reloc (void * pAdrs, Elf32_Rela * pRelocCmd, 
				SYM_INFO_TBL symInfoTbl);
LOCAL STATUS elfSparcPc10Reloc (void * pAdrs, Elf32_Rela * pRelocCmd, 
				SYM_INFO_TBL symInfoTbl);
LOCAL STATUS elfSparcPc22Reloc (void * pAdrs, Elf32_Rela * pRelocCmd, 
				SYM_INFO_TBL symInfoTbl);
LOCAL STATUS elfSparcUa32Reloc (void * pAdrs, Elf32_Rela * pRelocCmd, 
				SYM_INFO_TBL symInfoTbl);
#endif	/* (CPU_FAMILY == SIMSPARCSOLARIS) */

#if 	(CPU_FAMILY == COLDFIRE)
LOCAL STATUS elfM68k32Reloc (void * pAdrs, Elf32_Rela * pRelocCmd, 
			     SYM_INFO_TBL symInfoTbl);
LOCAL STATUS elfM68k16Reloc (void * pAdrs, Elf32_Rela * pRelocCmd, 
			     SYM_INFO_TBL symInfoTbl);
LOCAL STATUS elfM68kDisp32Reloc (void * pAdrs, Elf32_Rela * pRelocCmd, 
				 SYM_INFO_TBL symInfoTbl);
LOCAL STATUS elfM68kDisp16Reloc (void * pAdrs, Elf32_Rela * pRelocCmd, 
				 SYM_INFO_TBL symInfoTbl);
LOCAL STATUS elfM68kSegReloc (int fd, MODULE_ID moduleId, int loadFlag, 
			      int posCurRelocCmd, Elf32_Shdr * pScnHdrTbl,
    			      Elf32_Shdr * pRelHdr, SCN_ADRS * pScnAddr,
    			      SYM_INFO_TBL symInfoTbl, Elf32_Sym * pSymsArray,
    			      SYMTAB_ID symTbl, SEG_INFO * pSeg);
#endif 	/* (CPU_FAMILY == COLDFIRE) */

#if	(CPU_FAMILY == SH)

LOCAL STATUS elfShSegReloc (int fd, MODULE_ID moduleId, int loadFlag,
		    		int posCurRelocCmd, Elf32_Shdr * pScnHdrTbl,
			    	Elf32_Shdr * pRelHdr, SCN_ADRS * pScnAddr,
	    			SYM_INFO_TBL symInfoTbl,
				Elf32_Sym *  pSymsArray,
			    	SYMTAB_ID    symTbl,
				SEG_INFO *   pSeg);
LOCAL STATUS elfShDir32Reloc (void * pAdrs, Elf32_Rela * pRelocCmd, 
				SYM_INFO_TBL symInfoTbl, MODULE_ID moduleId);
#if FALSE /* Relocation types not currently supported. */
LOCAL STATUS elfShRel32Reloc (void * pAdrs, Elf32_Rela * pRelocCmd, 
				SYM_INFO_TBL symInfoTbl);
LOCAL STATUS elfShDir8wpnReloc (void * pAdrs, Elf32_Rela * pRelocCmd, 
				SYM_INFO_TBL symInfoTbl);
LOCAL STATUS elfShInd12wReloc (void * pAdrs, Elf32_Rela * pRelocCmd, 
				SYM_INFO_TBL symInfoTbl);
LOCAL STATUS elfShDir8wplReloc (void * pAdrs, Elf32_Rela * pRelocCmd, 
				SYM_INFO_TBL symInfoTbl);
LOCAL STATUS elfShDir8bpReloc (void * pAdrs, Elf32_Rela * pRelocCmd, 
				SYM_INFO_TBL symInfoTbl);
LOCAL STATUS elfShSwitch16Reloc (void * pAdrs, Elf32_Rela * pRelocCmd, 
				SYM_INFO_TBL symInfoTbl);
#endif /* relocation types not currently supported. */

#endif	/* (CPU_FAMILY == SH) */

LOCAL BOOL 	loadElfModuleIsOk (Elf32_Ehdr *pHdr);
LOCAL STATUS 	loadElfMdlHdrCheck (Elf32_Ehdr *pHdr);
LOCAL STATUS 	loadElfMdlHdrRd (int fd, Elf32_Ehdr *pHdr);
LOCAL STATUS 	loadElfProgHdrCheck (Elf32_Phdr *pProgHdr, int progHdrNum);
LOCAL STATUS 	loadElfProgHdrTblRd (int fd, int posProgHdrField,
				 Elf32_Phdr *pProgHdrTbl, int progHdrNumber);
LOCAL STATUS 	loadElfScnHdrCheck (Elf32_Shdr *pScnHdr, int scnHdrNum);
LOCAL STATUS 	loadElfScnHdrIdxDispatch (Elf32_Shdr *pScnHdrTbl, int scnHdrIdx,
				IDX_TBLS *pIndexTables);
LOCAL STATUS 	loadElfScnHdrRd (int fd, int posScnHdrField, 
				Elf32_Shdr * pScnHdrTbl,
    				int sectionNumber, IDX_TBLS * pIndexTables);
LOCAL void 	loadElfSegSizeGet (char * pScnStrTbl, 
				UINT32 *pLoadScnHdrIdxs, 
				Elf32_Shdr *pScnHdrTbl, SEG_INFO *pSeg);
LOCAL STATUS 	loadElfScnRd (int fd, char * pScnStrTbl, 
				UINT32 *pLoadScnHdrIdxs, Elf32_Shdr *pScnHdrTbl,
				SCN_ADRS_TBL sectionAdrsTbl, SEG_INFO *pSeg);
LOCAL int	loadElfSymEntryRd (int fd, int symEntry, Elf32_Sym *pSymbol);
LOCAL STATUS 	loadElfSymTabRd (int fd, int nextSym, UINT32 nSyms, 
				Elf32_Sym *pSymsArray);
LOCAL int 	loadElfSymTablesHandle (UINT32 *pSymTabScnHdrIdxs, 
				Elf32_Shdr *pScnHdrTbl, int fd,	
				SYMTBL_REFS *pSymTblRefs,
				SYMINFO_REFS * pSymsAdrsRefs);
LOCAL SYM_TYPE 	loadElfSymTypeGet (Elf32_Sym *pSymbol, Elf32_Shdr *pScnHdrTbl, 
				char * pScnStrTbl);
LOCAL BOOL 	loadElfSymIsVisible (UINT32	symAssoc, UINT32 symBinding, 
				int loadFlag);
LOCAL STATUS 	loadElfSymTabProcess (MODULE_ID moduleId, int loadFlag, 
				Elf32_Sym *pSymsArray, 
				SCN_ADRS_TBL sectionAdrsTbl,
				SYM_INFO_TBL symsAdrsTbl, char * pStringTable,
				SYMTAB_ID symTbl, UINT32 symNumber,
				Elf32_Shdr * pScnHdrTbl, char * pScnStrTbl,
				SEG_INFO * pSeg);
LOCAL STATUS 	loadElfSymTableBuild (MODULE_ID moduleId, int loadFlag,
				SYMTBL_REFS  symTblRefs, 
				SCN_ADRS_TBL sectionAdrsTbl,
				SYMINFO_REFS symsAdrsRefs, 
				IDX_TBLS *pIndexTables, SYMTAB_ID symTbl, 
				int fd, Elf32_Shdr * pScnHdrTbl,
				char * pScnStrTbl, SEG_INFO * pSeg);
LOCAL FUNCPTR 	loadElfRelSegRtnGet (void);
LOCAL STATUS 	loadElfSegReloc (int fd, int loadFlag, MODULE_ID moduleId, 
				Elf32_Ehdr * pHdr, IDX_TBLS *pIndexTables, 
				Elf32_Shdr *pScnHdrTbl,
			    	SCN_ADRS_TBL sectionAdrsTbl, 
				SYMTBL_REFS  symTblRefs,
				SYMINFO_REFS symsAdrsRefs, SYMTAB_ID symTbl,
				SEG_INFO * pSeg);
LOCAL STATUS 	loadElfTablesAlloc (Elf32_Ehdr *pHdr, Elf32_Phdr **ppProgHdrTbl,
				Elf32_Shdr **ppScnHdrTbl, 
				IDX_TBLS *pIndexTables);
LOCAL MODULE_ID loadElfFmtManage (FAST int fd, int loadFlag, void **ppText,
			  	void **ppData, void **ppBss, SYMTAB_ID symTbl);
LOCAL void 	loadElfBufferFree (void ** ppBuf);
LOCAL STATUS 	loadElfRelocMod (SEG_INFO * pSeg, int fd, char * pScnStrTbl,
				IDX_TBLS * pIndexTables, Elf32_Ehdr * pHdr,
    				Elf32_Shdr * pScnHdrTbl,
    				SCN_ADRS_TBL * pSectionAdrsTbl);
LOCAL STATUS 	loadElfSegStore (SEG_INFO * pSeg, int loadFlag, int fd,
				char * pScnStrTbl,
    				IDX_TBLS * pIndexTables, Elf32_Ehdr * pHdr,
    				Elf32_Shdr * pScnHdrTbl, 
				Elf32_Phdr * pProgHdrTbl,
    				SCN_ADRS_TBL * pSectionAdrsTbl);
LOCAL char * 	loadElfScnStrTblRd (int fd, Elf32_Shdr * pScnHdrTbl, 
				Elf32_Ehdr * pHdr);

#ifdef INCLUDE_SDA
LOCAL STATUS 	loadElfSdaAllocate (SDA_INFO * pSda);

LOCAL STATUS 	loadElfSdaCreate (void);

LOCAL SDA_SCN_TYPE loadElfSdaScnDetermine (char * pScnStrTbl, 
				Elf32_Shdr * pScnHdr, char * sectionName);

LOCAL BOOL      sdaIsRequired = FALSE;  /* TRUE if SDA are used by the arch */
LOCAL PART_ID   sdaMemPartId = NULL;    /* keeps SDA memory partition id */
LOCAL PART_ID   sda2MemPartId = NULL;   /* keeps SDA2 memory partition id */
LOCAL void *    sdaBaseAddr = NULL;     /* keeps SDA area base address */
LOCAL void *    sda2BaseAddr = NULL;    /* keeps SDA2 area base address */
LOCAL int     	sdaAreaSize = 0;     	/* keeps SDA area size */
LOCAL int     	sda2AreaSize = 0;     	/* keeps SDA area size */
#endif /* INCLUDE_SDA */

/*******************************************************************************
*
* elfRelocRelaEntryRd - read in ELF relocation entry for RELA relocation
* 
* This routine fills a relocation structure with information from the object
* module in memory.
*
* RETURNS : the address of the next relocation entry or ERROR if entry not read.
*/

int elfRelocRelaEntryRd
    (
    int 	 fd,		/* file to read in */
    int 	 posRelocEntry,	/* position of reloc. command in object file */
    Elf32_Rela * pReloc		/* ptr on relocation structure to fill */
    )
    {
    int nbytes;		/* number of bytes to copy */

    nbytes = sizeof (Elf32_Rela);

    if ((ioctl (fd, FIOSEEK, posRelocEntry)) == ERROR)
        return ERROR; 

    if ((fioRead (fd, (char *) pReloc, nbytes)) == ERROR)
        return ERROR;

    return (posRelocEntry + nbytes);
    }
     
/*******************************************************************************
*
* elfRelocRelEntryRd - read in ELF relocation entry for REL relocation
* 
* This routine fills a relocation structure with information from the object
* module in memory. 
*
* RETURNS : the address of the next relocation entry or ERROR if entry not read.
*/

int elfRelocRelEntryRd
    (
    int          fd,            /* file to read in */
    int          posRelocEntry, /* position of reloc. command in object file */
    Elf32_Rel *  pReloc		/* ptr on relocation structure to fill */
    )
    {
    int nbytes;         /* number of bytes to copy */

    nbytes = sizeof (Elf32_Rel);

    if ((ioctl (fd, FIOSEEK, posRelocEntry)) == ERROR)
        return ERROR; 

    if ((fioRead (fd, (char *) pReloc, nbytes)) == ERROR)
        return ERROR;

    return (posRelocEntry + nbytes);
    }

#if     (CPU_FAMILY == PPC)

/*******************************************************************************
*
* elfPpcSegReloc - perform relocation for the PowerPC family
*
* This routine reads the specified relocation command segment and performs
* all the relocations specified therein. Only relocation command from sections
* with section type SHT_RELA are considered here.
*
* Absolute symbol addresses are looked up in the 'externals' table.
*
* This function handles the following types of relocation commands
* for the PowerPC processor:
*
* System V ABI:
*	R_PPC_NONE		: none
*	R_PPC_ADDR32		: word32,	S + A
*	R_PPC_ADDR24		: low24,	(S + A) >> 2
*	R_PPC_ADDR16		: half16,	S + A
*	R_PPC_ADDR16_LO		: half16,	#lo(S + A)
*	R_PPC_ADDR16_HI		: half16,	#hi(S + A)
*	R_PPC_ADDR16_HA		: half16,	#ha(S + A)
*	R_PPC_ADDR14		: low14,	(S + A) >> 2
*	R_PPC_ADDR14_BRTAKEN	: low14,	(S + A) >> 2
*	R_PPC_ADDR14_BRNTAKEN	: low14,	(S + A) >> 2
*	R_PPC_REL24		: low24,	(S + A - P) >> 2
*	R_PPC_REL14		: low14,	(S + A - P) >> 2
*	R_PPC_REL14_BRTAKEN	: low14,	(S + A - P) >> 2
*	R_PPC_REL14_BRNTAKEN	: low14,	(S + A - P) >> 2
*	R_PPC_UADDR32		: word32,	S + A
*	R_PPC_UADDR16		: half16,	S + A
*	R_PPC_REL32		: word32,	S + A - P
*	R_PPC_SDAREL		: half16,	S + A - SDA_BASE
*
* PowerPC EABI:
*	R_PPC_EMB_NADDR32	: uword32,	A - S
*	R_PPC_EMB_NADDR16	: uhalf16,	A - S
*	R_PPC_EMB_NADDR16_LO	: uhalf16,	#lo(A - S)
*	R_PPC_EMB_NADDR16_HI	: uhalf16,	#hi(A - S)
*	R_PPC_EMB_NADDR16_HA	: uhalf16,	#ha(A - S)
*	R_PPC_EMB_MRKREF	: none
*	R_PPC_EMB_SDA21		: ulow21,	complex
*	R_PPC_EMB_SDA2REL	: uhalf16,	S + A - SDA2_BASE
*	R_PPC_EMB_RELSDA	: uhalf16,	complex
*
* RETURNS: OK or ERROR
*/

LOCAL STATUS elfPpcSegReloc
    (
    int		 fd,		/* file to read in */
    MODULE_ID	 moduleId,	/* module id */
    int		 loadFlag,	/* not used */
    int	 	 posCurRelocCmd,/* position of current relocation command */
    Elf32_Shdr * pScnHdrTbl,	/* not used */
    Elf32_Shdr * pRelHdr,	/* pointer to relocation section header */
    SCN_ADRS *	 pScnAddr,	/* section address once loaded */
    SYM_INFO_TBL symInfoTbl,	/* array of absolute symbol values and types */
    Elf32_Sym *	 pSymsArray,	/* pointer to symbols array */
    SYMTAB_ID 	 symTbl,	/* not used */
    SEG_INFO *	 pSeg		/* section addresses and sizes */
    )
    {
    Elf32_Rela   relocCmd;	/* relocation structure */
    UINT32	 relocNum;	/* number of reloc entries in section */
    UINT32	 relocIdx;	/* index of the reloc entry being processed */
    void *	 pAdrs;		/* relocation address */
    Elf32_Sym *	 pSym;		/* pointer to an external symbol */
    STATUS	 status = OK;	/* whether or not the relocation was ok */

    /* Some sanity checking */

    if (pRelHdr->sh_type != SHT_RELA)
	{
        errnoSet (S_loadElfLib_RELA_SECTION);
	return (OK);
	}

    if (pRelHdr->sh_entsize != sizeof (Elf32_Rela))
	{
        errnoSet (S_loadElfLib_RELA_SECTION);
	return (ERROR);
	}

    /* Relocation loop */

    relocNum = pRelHdr->sh_size / pRelHdr->sh_entsize;

    for (relocIdx = 0; relocIdx < relocNum; relocIdx++)
	{
	/* read relocation command */

	if ((posCurRelocCmd = 
	     elfRelocRelaEntryRd (fd, posCurRelocCmd, &relocCmd)) == ERROR)
	    {
	    errnoSet (S_loadElfLib_READ_SECTIONS);
	    return (ERROR);
	    }

	/*
	 * Calculate actual remote address that needs relocation, and
	 * perform external or section relative relocation.
	 */

	pAdrs = (void *)((Elf32_Addr)*pScnAddr + relocCmd.r_offset);

	pSym = pSymsArray + ELF32_R_SYM (relocCmd.r_info);

	/*
	 * System V ABI defines the following notations:
	 *
	 * A - the addend used to compute the value of the relocatable field.
	 * S - the value (address) of the symbol whose index resides in the
	 *     relocation entry. Note that this function uses the value stored
	 *     in the external symbol value array instead of the symbol's
	 *     st_value field.
	 * P - the place (section offset or address) of the storage unit being
	 *     relocated (computed using r_offset) prior to the relocation.
	 */

        switch (ELF32_R_TYPE (relocCmd.r_info))
            {
	    case (R_PPC_NONE):				/* none */
		break;

	    case (R_PPC_ADDR32):			/* word32, S + A */
		if (elfPpcAddr32Reloc (pAdrs, &relocCmd, symInfoTbl,
				       moduleId) != OK)
		    status = ERROR;
		break;

	    case (R_PPC_ADDR24):			/* low24, (S+A) >> 2 */
		if (elfPpcAddr24Reloc (pAdrs, &relocCmd, symInfoTbl) != OK)
		    status = ERROR;
		break;

	    case (R_PPC_ADDR16):			/* half16, S + A */
		if (elfPpcAddr16Reloc (pAdrs, &relocCmd, symInfoTbl) != OK)
		    status = ERROR;
		break;

	    case (R_PPC_ADDR16_LO):			/* half16, #lo(S + A) */
		if (elfPpcAddr16LoReloc (pAdrs, &relocCmd, symInfoTbl) != OK)
		    status = ERROR;
		break;

	    case (R_PPC_ADDR16_HI):			/* half16, #hi(S + A) */
		if (elfPpcAddr16HiReloc (pAdrs, &relocCmd, symInfoTbl) != OK)
		    status = ERROR;
		break;

	    case (R_PPC_ADDR16_HA):			/* half16, #ha(S + A) */
		if (elfPpcAddr16HaReloc (pAdrs, &relocCmd, symInfoTbl) != OK)
		    status = ERROR;
		break;

	    case (R_PPC_ADDR14):
	    case (R_PPC_ADDR14_BRTAKEN):
	    case (R_PPC_ADDR14_BRNTAKEN):		/* low14, (S+A) >> 2 */
		if (elfPpcAddr14Reloc (pAdrs, &relocCmd, symInfoTbl) != OK)
		    status = ERROR;
		break;

	    case (R_PPC_REL24):				/* low24, (S+A-P) >> 2*/
		if (elfPpcRel24Reloc (pAdrs, &relocCmd, symInfoTbl) != OK)
		    status = ERROR;
		break;

	    case (R_PPC_REL14):
	    case (R_PPC_REL14_BRTAKEN):
	    case (R_PPC_REL14_BRNTAKEN):		/* low14, (S+A-P) >> 2*/
		if (elfPpcRel14Reloc (pAdrs, &relocCmd, symInfoTbl) != OK)
		    status = ERROR;
		break;

	    case (R_PPC_UADDR32):			/* word32, S + A */
		if (elfPpcUaddr32Reloc (pAdrs, &relocCmd, symInfoTbl) != OK)
		    status = ERROR;
		break;

	    case (R_PPC_UADDR16):			/* half16, S + A */
		if (elfPpcUaddr16Reloc (pAdrs, &relocCmd, symInfoTbl) != OK)
		    status = ERROR;
		break;

	    case (R_PPC_REL32):				/* word32, S + A - P */
		if (elfPpcRel32Reloc (pAdrs, &relocCmd, symInfoTbl) != OK)
		    status = ERROR;
		break;

	    case (R_PPC_EMB_NADDR32):			/* uword32, A - S */
		if (elfPpcEmbNaddr32Reloc (pAdrs, &relocCmd, symInfoTbl) != OK)
		    status = ERROR;
		break;

	    case (R_PPC_EMB_NADDR16):			/* uhalf16, A - S */
		if (elfPpcEmbNaddr16Reloc (pAdrs, &relocCmd, symInfoTbl) != OK)
		    status = ERROR;
		break;

	    case (R_PPC_EMB_NADDR16_LO):		/* uhalf16, #lo(A - S)*/
		if (elfPpcEmbNaddr16LoReloc (pAdrs, &relocCmd, symInfoTbl) 
						!= OK)
		    status = ERROR;
		break;

	    case (R_PPC_EMB_NADDR16_HI):		/* uhalf16, #hi(A - S)*/
		if (elfPpcEmbNaddr16HiReloc (pAdrs, &relocCmd, symInfoTbl) 
						!= OK)
		    status = ERROR;
		break;

	    case (R_PPC_EMB_NADDR16_HA):		/* uhalf16, #ha(A - S)*/
		if (elfPpcEmbNaddr16HaReloc (pAdrs, &relocCmd, symInfoTbl) 
						!= OK)
		    status = ERROR;
		break;

	    case (R_PPC_EMB_MRKREF):			/* none */
		break;
#ifdef INCLUDE_SDA
	    case (R_PPC_SDAREL):		/* half16, S + A - SDA_BASE */
		if (elfPpcSdaRel16Reloc (pAdrs, &relocCmd, symInfoTbl,
					 pSeg) != OK)
		    status = ERROR;
		break;

	    case (R_PPC_EMB_SDA21):			/* ulow21, complex ! */
		if (elfPpcEmbSda21Reloc (pAdrs, &relocCmd, symInfoTbl,
					 pSeg) != OK)
		    status = ERROR;
		break;

	    case (R_PPC_EMB_SDA2REL):		/* uhalf16, S + A - SDA2_BASE */
		if (elfPpcEmbSda2RelReloc (pAdrs, &relocCmd, symInfoTbl,
					   pSeg) != OK)
		    status = ERROR;
		break;

	    case (R_PPC_EMB_RELSDA):			/* uhalf16, complex ! */
		if (elfPpcEmbRelSdaReloc (pAdrs, &relocCmd, symInfoTbl,
					  pSeg) != OK)
		    status = ERROR;
		break;
#endif /* INCLUDE_SDA */
	    default:
        	printErr ("Unsupported relocation type %d\n",
			      ELF32_R_TYPE (relocCmd.r_info));
        	status = ERROR;
        	break;
	    }
	}

    return (status);
    }

/*******************************************************************************
*
* elfPpcAddr32Reloc - perform the R_PPC_ADDR32 relocation
* 
* This routine handles the R_PPC_ADDR32 relocation (word32, S + A).
*
* RETURNS : OK or ERROR if computed value can't be written down in target
*	    memory.
*/

LOCAL STATUS elfPpcAddr32Reloc
    (
    void *	 pAdrs,		/* relocation address */
    Elf32_Rela * pRelocCmd,	/* points to a relocation structure */
    SYM_INFO_TBL symInfoTbl,	/* array of absolute symbol values and types */
    MODULE_ID	 moduleId	/* module id */
    )
    {
    UINT32	 value;		/* relocation value */

    value = (UINT32)symInfoTbl [ELF32_R_SYM (pRelocCmd->r_info)].pAddr +
	    pRelocCmd->r_addend;

    *((UINT32 *)pAdrs) = value;

    return (OK);
    }

/*******************************************************************************
*
* elfPpcAddr24Reloc - perform the R_PPC_ADDR24 relocation
* 
* This routine handles the R_PPC_ADDR24 relocation (low24, S + A).
*
* RETURNS : OK or ERROR if computed value can't be written down in target
*	    memory or offset too large for relocation.
*/

LOCAL STATUS elfPpcAddr24Reloc
    (
    void *	 pAdrs,		/* relocation address */
    Elf32_Rela * pRelocCmd,	/* points to a relocation structure */
    SYM_INFO_TBL symInfoTbl	/* array of absolute symbol values and types */
    )
    {
    UINT32	 value;		/* relocation value */
    UINT32	 offset;	/* previous value in memory */

    value = (UINT32)symInfoTbl [ELF32_R_SYM (pRelocCmd->r_info)].pAddr +
	    pRelocCmd->r_addend;

    /*
     * Do some checking: R_PPC_ADDR24 relocation must fit in
     * 24 bits and lower 2 bits should always be null.
     */

    if (!CHECK_LOW24 (value))
	{
        printErr ("Relocation value does not fit in 24 bits.\n");
	return (ERROR);
	}

    if (value & 0x3)
        printErr ("Relocation value's lower 2 bits not zero.\n");

    offset = *((UINT32 *)pAdrs); 

    LOW24_INSERT (offset, value);

    *((UINT32 *)pAdrs) = offset;

    return (OK);
    }

/*******************************************************************************
*
* elfPpcAddr16Reloc - perform the R_PPC_ADDR16 relocation
* 
* This routine handles the R_PPC_ADDR16 relocation (half16, S + A).
*
* RETURNS : OK or ERROR if computed value can't be written down in target
*	    memory or offset too large for relocation.
*/

LOCAL STATUS elfPpcAddr16Reloc
    (
    void *	 pAdrs,		/* relocation address */
    Elf32_Rela * pRelocCmd,	/* points to a relocation structure */
    SYM_INFO_TBL symInfoTbl	/* array of absolute symbol values and types */
    )
    {
    UINT32	 value;		/* relocation value */

    value = (UINT32)symInfoTbl [ELF32_R_SYM (pRelocCmd->r_info)].pAddr +
	    pRelocCmd->r_addend;

    /*
     * Do some checking: R_PPC_ADDR16 relocation must fit in
     * 16 bits.
     */

    if (!CHECK_LOW16 (value))
	{
        printErr ("Relocation value does not fit in 16 bits.\n");
	return (ERROR);
	}

    *((UINT16 *)pAdrs) = (UINT16) value;

    return (OK);
    }

/*******************************************************************************
*
* elfPpcAddr16LoReloc - perform the R_PPC_ADDR16_LO relocation
* 
* This routine handles the R_PPC_ADDR16_LO relocation (half16, #lo(S + A)).
*
* RETURNS : OK or ERROR if computed value can't be written down in target
*	    memory.
*/

LOCAL STATUS elfPpcAddr16LoReloc
    (
    void *	 pAdrs,		/* relocation address */
    Elf32_Rela * pRelocCmd,	/* points to a relocation structure */
    SYM_INFO_TBL symInfoTbl	/* array of absolute symbol values and types */
    )
    {
    UINT32	 value;		/* relocation value */

    value = (UINT32)symInfoTbl [ELF32_R_SYM (pRelocCmd->r_info)].pAddr +
	    pRelocCmd->r_addend;

    *((UINT16 *)pAdrs) = (UINT16) LO_VALUE (value);

    return (OK);
    }

/*******************************************************************************
*
* elfPpcAddr16HiReloc - perform the R_PPC_ADDR16_HI relocation
* 
* This routine handles the R_PPC_ADDR16_HI relocation (half16, #hi(S + A)).
*
* RETURNS : OK or ERROR if computed value can't be written down in target
*	    memory.
*/

LOCAL STATUS elfPpcAddr16HiReloc
    (
    void *	 pAdrs,		/* relocation address */
    Elf32_Rela * pRelocCmd,	/* points to a relocation structure */
    SYM_INFO_TBL symInfoTbl	/* array of absolute symbol values and types */
    )
    {
    UINT32	 value;		/* relocation value */

    value = (UINT32)symInfoTbl [ELF32_R_SYM (pRelocCmd->r_info)].pAddr +
	    pRelocCmd->r_addend;

    *((UINT16 *)pAdrs) = (UINT16) (HI_VALUE (value));

    return (OK);
    }

/*******************************************************************************
*
* elfPpcAddr16HaReloc - perform the R_PPC_ADDR16_HA relocation
* 
* This routine handles the R_PPC_ADDR16_HA relocation (half16, #ha(S + A)).
*
* RETURNS : OK or ERROR if computed value can't be written down in target
*	    memory.
*/

LOCAL STATUS elfPpcAddr16HaReloc
    (
    void *	 pAdrs,		/* relocation address */
    Elf32_Rela * pRelocCmd,	/* points to a relocation structure */
    SYM_INFO_TBL symInfoTbl	/* array of absolute symbol values and types */
    )
    {
    UINT32	 value;		/* relocation value */

    value = (UINT32)symInfoTbl [ELF32_R_SYM (pRelocCmd->r_info)].pAddr +
	    pRelocCmd->r_addend;

    *((UINT16 *)pAdrs) = (UINT16) HA_VALUE (value);

    return (OK);
    }

/*******************************************************************************
*
* elfPpcAddr14Reloc - perform the R_PPC_ADDR14 relocation
* 
* This routine handles the R_PPC_ADDR14 relocation (low14, (S + A) >> 2).
*
* RETURNS : OK or ERROR if computed value can't be written down in target
*	    memory or offset too large for relocation.
*/

LOCAL STATUS elfPpcAddr14Reloc
    (
    void *	 pAdrs,		/* relocation address */
    Elf32_Rela * pRelocCmd,	/* points to a relocation structure */
    SYM_INFO_TBL symInfoTbl	/* array of absolute symbol values and types */
    )
    {
    UINT32	 value;		/* relocation value */
    UINT32	 offset;	/* previous value in memory */

    value = (UINT32)symInfoTbl [ELF32_R_SYM (pRelocCmd->r_info)].pAddr +
    	pRelocCmd->r_addend;

    /*
     * Do some checking: R_PPC_ADDR14* relocations must fit in
     * 14 bits and lower 2 bits should always be null.
     */

    if (!CHECK_LOW14 (value))
	{
        printErr ("Relocation value does not fit in 14 bits.\n");
	return (ERROR);
	}

    if (value & 0x3)
        printErr ("Relocation value's lower 2 bits not zero.\n");

    offset = *((UINT32 *)pAdrs);

    LOW14_INSERT (offset, value);

    *((UINT32 *)pAdrs) = offset;

    return (OK);
    }

/*******************************************************************************
*
* elfPpcRel24Reloc - perform the R_PPC_REL24 relocation
* 
* This routine handles the R_PPC_REL24 relocation (low24, (S + A - P) >> 2).
*
* RETURNS : OK or ERROR if computed value can't be written down in target
*	    memory or offset too large for relocation.
*/

LOCAL STATUS elfPpcRel24Reloc
    (
    void *	 pAdrs,		/* relocation address */
    Elf32_Rela * pRelocCmd,	/* points to a relocation structure */
    SYM_INFO_TBL symInfoTbl	/* array of absolute symbol values and types */
    )
    {
    UINT32	 value;		/* relocation value */
    UINT32	 offset;	/* previous value in memory */

    value = (UINT32)symInfoTbl [ELF32_R_SYM (pRelocCmd->r_info)].pAddr +
	    pRelocCmd->r_addend - ((UINT32) pAdrs);

    /*
     * Do some checking: R_PPC_REL24 relocation must fit in
     * 24 bits and lower 2 bits should always be null.
     */

    if (!CHECK_LOW24 (value))
	{
        printErr ("Relocation value does not fit in 24 bits.\n");
	return (ERROR);
	}

    if (value & 0x3)
	printErr ("Relocation value's lower 2 bits not zero.\n");

    offset = *((UINT32 *)pAdrs);

    LOW24_INSERT (offset, value);

    *((UINT32 *)pAdrs) = offset;

    return (OK);
    }

/*******************************************************************************
*
* elfPpcRel14Reloc - perform the R_PPC_REL14 relocation
* 
* This routine handles the R_PPC_REL14 relocation (low14, (S + A - P) >> 2).
*
* RETURNS : OK or ERROR if computed value can't be written down in target
*	    memory or offset too large for relocation.
*/

LOCAL STATUS elfPpcRel14Reloc
    (
    void *	 pAdrs,		/* relocation address */
    Elf32_Rela * pRelocCmd,	/* points to a relocation structure */
    SYM_INFO_TBL symInfoTbl	/* array of absolute symbol values and types */
    )
    {
    UINT32	 value;		/* relocation value */
    UINT32	 offset;	/* previous value in memory */

    value = (UINT32)symInfoTbl [ELF32_R_SYM (pRelocCmd->r_info)].pAddr +
	    pRelocCmd->r_addend - ((UINT32) pAdrs);

    /*
     * Do some checking: R_PPC_REL14* relocations must fit in
     * 14 bits and lower 2 bits should always be null.
     */

    if (!CHECK_LOW14 (value))
	{
        printErr ("Relocation value does not fit in 14 bits.\n");
	return (ERROR);
	}

    if (value & 0x3)
        printErr ("Relocation value's lower 2 bits not zero.\n");

    offset = *((UINT32 *)pAdrs);

    LOW14_INSERT (offset, value);

    *((UINT32 *)pAdrs) = offset;

    return (OK);
    }

/*******************************************************************************
*
* elfPpcUaddr32Reloc - perform the R_PPC_UADDR32 relocation
* 
* This routine handles the R_PPC_UADDR32 relocation (word32, S + A).
*
* RETURNS : OK or ERROR if computed value can't be written down in target
*	    memory.
*/

LOCAL STATUS elfPpcUaddr32Reloc
    (
    void *	 pAdrs,		/* relocation address */
    Elf32_Rela * pRelocCmd,	/* points to a relocation structure */
    SYM_INFO_TBL symInfoTbl	/* array of absolute symbol values and types */
    )
    {
    UINT32	 value;		/* relocation value */

    value = (UINT32)symInfoTbl [ELF32_R_SYM (pRelocCmd->r_info)].pAddr +
	    pRelocCmd->r_addend;

#ifdef	 _WRS_STRICT_ALIGNMENT
    ELFOUTMSBU32 ((char *)pAdrs, value);
#else
    *((UINT32 *)pAdrs) = value;
#endif /* _WRS_STRICT_ALIGNMENT */
    return (OK);
    }

/*******************************************************************************
*
* elfPpcUaddr16Reloc - perform the R_PPC_UADDR16 relocation
* 
* This routine handles the R_PPC_UADDR16 relocation (half16, S + A).
*
* RETURNS : OK or ERROR if computed value can't be written down in target
*	    memory or offset too large for relocation.
*/

LOCAL STATUS elfPpcUaddr16Reloc
    (
    void *	 pAdrs,		/* relocation address */
    Elf32_Rela * pRelocCmd,	/* points to a relocation structure */
    SYM_INFO_TBL symInfoTbl	/* array of absolute symbol values and types */
    )
    {
    UINT32	 value;		/* relocation value */

    value = (UINT32)symInfoTbl [ELF32_R_SYM (pRelocCmd->r_info)].pAddr +
	    pRelocCmd->r_addend;

    /*
     * Do some checking: R_PPC_UADDR16 relocation must fit in
     * 16 bits.
     */

    if (!CHECK_LOW16 (value))
	{
        printErr ("Relocation value does not fit in 16 bits.\n");
	return (ERROR);
	}

    /* handle unalignment */
    
    *((UINT16 *)pAdrs) = (UINT16) value;

    return (OK);
    }

/*******************************************************************************
*
* elfPpcRel32Reloc - perform the R_PPC_REL32 relocation
* 
* This routine handles the R_PPC_REL32 relocation (word32, S + A - P).
*
* RETURNS : OK or ERROR if computed value can't be written down in target
*	    memory.
*/

LOCAL STATUS elfPpcRel32Reloc
    (
    void *	 pAdrs,		/* relocation address */
    Elf32_Rela * pRelocCmd,	/* points to a relocation structure */
    SYM_INFO_TBL symInfoTbl	/* array of absolute symbol values and types */
    )
    {
    UINT32	 value;		/* relocation value */

    value = (UINT32)symInfoTbl [ELF32_R_SYM (pRelocCmd->r_info)].pAddr +
	    pRelocCmd->r_addend - ((UINT32) pAdrs);

    *((UINT32 *)pAdrs) = value;

    return (OK);
    }


/*******************************************************************************
*
* elfPpcEmbNaddr32Reloc - perform the R_PPC_EMB_NADDR32 relocation
* 
* This routine handles the R_PPC_EMB_NADDR32 relocation (uword32, A - S).
*
* RETURNS : OK or ERROR if computed value can't be written down in target
*	    memory.
*/

LOCAL STATUS elfPpcEmbNaddr32Reloc
    (
    void *	 pAdrs,		/* relocation address */
    Elf32_Rela * pRelocCmd,	/* points to a relocation structure */
    SYM_INFO_TBL symInfoTbl	/* array of absolute symbol values and types */
    )
    {
    UINT32	 value;		/* relocation value */

    value = pRelocCmd->r_addend -
	    (UINT32)symInfoTbl [ELF32_R_SYM (pRelocCmd->r_info)].pAddr;

    *((UINT32 *)pAdrs) = value;

    return (OK);
    }

/*******************************************************************************
*
* elfPpcEmbNaddr16Reloc - perform the R_PPC_EMB_NADDR16 relocation
* 
* This routine handles the R_PPC_EMB_NADDR16 relocation (uhalf16, A - S).
*
* RETURNS : OK or ERROR if computed value can't be written down in target
*	    memory or offset too large for relocation.
*/

LOCAL STATUS elfPpcEmbNaddr16Reloc
    (
    void *	 pAdrs,		/* relocation address */
    Elf32_Rela * pRelocCmd,	/* points to a relocation structure */
    SYM_INFO_TBL symInfoTbl	/* array of absolute symbol values and types */
    )
    {
    UINT32	 value;		/* relocation value */

    value = pRelocCmd->r_addend -
	    (UINT32)symInfoTbl [ELF32_R_SYM (pRelocCmd->r_info)].pAddr;

    /*
     * Do some checking: R_PPC_EMB_NADDR16 relocation must fit in
     * 16 bits.
     */

    if (!CHECK_LOW16 (value))
	{
        printErr ("Relocation value does not fit in 16 bits.\n");
	return (ERROR);
	}

    *((UINT16 *)pAdrs) = (UINT16) value;

    return (OK);
    }

/*******************************************************************************
*
* elfPpcEmbNaddr16LoReloc - perform the R_PPC_EMB_NADDR16_LO relocation
* 
* This routine handles the R_PPC_EMB_NADDR16_LO relocation (uhalf16,
* #lo (A - S)).
*
* RETURNS : OK or ERROR if computed value can't be written down in target
*	    memory.
*/

LOCAL STATUS elfPpcEmbNaddr16LoReloc
    (
    void *	 pAdrs,		/* relocation address */
    Elf32_Rela * pRelocCmd,	/* points to a relocation structure */
    SYM_INFO_TBL symInfoTbl	/* array of absolute symbol values and types */
    )
    {
    UINT32	 value;		/* relocation value */

    value = pRelocCmd->r_addend -
	    (UINT32)symInfoTbl [ELF32_R_SYM (pRelocCmd->r_info)].pAddr;

    *((UINT16 *)pAdrs) = (UINT16) LO_VALUE (value);

    return (OK);
    }

/*******************************************************************************
*
* elfPpcEmbNaddr16HiReloc - perform the R_PPC_EMB_NADDR16_HI relocation
* 
* This routine handles the R_PPC_EMB_NADDR16_HI relocation (uhalf16,
* #hi (A - S)).
*
* RETURNS : OK or ERROR if computed value can't be written down in target
*	    memory.
*/

LOCAL STATUS elfPpcEmbNaddr16HiReloc
    (
    void *	 pAdrs,		/* relocation address */
    Elf32_Rela * pRelocCmd,	/* points to a relocation structure */
    SYM_INFO_TBL symInfoTbl	/* array of absolute symbol values and types */
    )
    {
    UINT32	 value;		/* relocation value */

    value = pRelocCmd->r_addend -
	    (UINT32)symInfoTbl [ELF32_R_SYM (pRelocCmd->r_info)].pAddr;

    *((UINT16 *)pAdrs) = (UINT16) HI_VALUE (value);

    return (OK);
    }

/*******************************************************************************
*
* elfPpcEmbNaddr16HaReloc - perform the R_PPC_EMB_NADDR16_HA relocation
* 
* This routine handles the R_PPC_EMB_NADDR16_HA relocation (uhalf16,
* #ha (A - S)).
*
* RETURNS : OK or ERROR if computed value can't be written down in target
*	    memory.
*/

LOCAL STATUS elfPpcEmbNaddr16HaReloc
    (
    void *	 pAdrs,		/* relocation address */
    Elf32_Rela * pRelocCmd,	/* points to a relocation structure */
    SYM_INFO_TBL symInfoTbl	/* array of absolute symbol values and types */
    )
    {
    UINT32	 value;		/* relocation value */

    value = pRelocCmd->r_addend -
	    (UINT32)symInfoTbl [ELF32_R_SYM (pRelocCmd->r_info)].pAddr;

    *((UINT16 *)pAdrs) = (UINT16) HA_VALUE (value);

    return (OK);
    }

#ifdef INCLUDE_SDA

/*******************************************************************************
*
* elfPpcSdaRel16Reloc - perform the R_PPC_SDAREL relocation
* 
* This routine handles the R_PPC_SDAREL relocation (half16, S + A - SDA_BASE).
*
* RETURNS : OK or ERROR if computed value can't be written down in target
*	    memory or offset too large for relocation.
*/

LOCAL STATUS elfPpcSdaRel16Reloc
    (
    void *	 pAdrs,		/* relocation address */
    Elf32_Rela * pRelocCmd,	/* points to a relocation structure */
    SYM_INFO_TBL symInfoTbl,	/* array of absolute symbol values and types */
    SEG_INFO *	 pSeg		/* section addresses and sizes */
    )
    {
    UINT32	 value;		/* relocation value */

    if ((symInfoTbl[ELF32_R_SYM (pRelocCmd->r_info)].type & SYM_SDA) 
					!= SYM_SDA)
	{
        printErr ("Referenced symbol does not belong to SDA.\n");
	return (ERROR);
	}

    value = (UINT32)symInfoTbl [ELF32_R_SYM (pRelocCmd->r_info)].pAddr +
	    pRelocCmd->r_addend -
	    (UINT32)(((SDA_INFO *)pSeg->pAdnlInfo)->sdaBaseAddr);

    /*
     * Do some checking: R_PPC_SDAREL relocation must fit in
     * 16 bits.
     */

    if (!CHECK_LOW16 (value))
	{
        printErr ("Relocation value does not fit in 16 bits.\n");
	return (ERROR);
	}

    /* handle unalignment */

    *((UINT16 *)pAdrs) = (UINT16) value;

    return (OK);
    }

/*******************************************************************************
*
* elfPpcEmbSda21Reloc - perform the R_PPC_EMB_SDA21 relocation
* 
* This routine handles the R_PPC_EMB_SDA21 relocation. The most significant 11
* bits of the address pointed to by the relocation entry are untouched (opcode
* and rS/rD field). The next most significant 5 bits (rA field) will hold the
* value 13 (SDA symbol) or 2 (SDA2 symbol). The least significant 16 bits are
* set to the two's complement of the following computation:
*     symbol address + r_addend value - SDA_BASE (or SDA2_BASE) value.
*
* RETURNS : OK or ERROR if computed value can't be written down in target
*	    memory.
*/

LOCAL STATUS elfPpcEmbSda21Reloc
    (
    void *	 pAdrs,		/* relocation address */
    Elf32_Rela * pRelocCmd,	/* points to a relocation structure */
    SYM_INFO_TBL symInfoTbl,	/* array of absolute symbol values and types */
    SEG_INFO *	 pSeg		/* section addresses and sizes */
    )
    {
    UINT32	 value;		/* relocation value */
    UINT32	 instr;		/* previous instruction in memory */
    BOOL	 symbolIsInSda;	/* TRUE if symbol belongs to the SDA area */
    BOOL	 sizeFit;	/* TRUE if computed value is 16 bit long */

    /*
     * Only symbols (or reference to) belonging to a .sdata, .sbss, .sdata2
     * or .sbss2 section are supported.
     */

    switch (symInfoTbl[ELF32_R_SYM (pRelocCmd->r_info)].type & SYM_SDA_MASK)
	{
	case (SYM_SDA):
	    symbolIsInSda = TRUE;
	    break;

	case (SYM_SDA2):
	    symbolIsInSda = FALSE;
	    break;

	default:
	    printErr ("Referenced symbol does not belong to SDA/SDA2.\n");
	    return (ERROR);
	    break;
	}

    value = (UINT32)symInfoTbl [ELF32_R_SYM (pRelocCmd->r_info)].pAddr +
	    pRelocCmd->r_addend;

    instr = *((UINT32 *)pAdrs);

    /*
     * We applied here the same relocation process as the GNU does since
     * the EABI document is totally phony on this one...
     */

    if (symbolIsInSda)
	{
	value -= (UINT32)(((SDA_INFO *)pSeg->pAdnlInfo)->sdaBaseAddr);
	sizeFit = CHECK_LOW16_STRICT (value);
	value = (instr & ~RA_MSK) | GPR13_MSK | ((~value + 1) & 0xffff);
	}
    else
	{
	value -= (UINT32)(((SDA_INFO *)pSeg->pAdnlInfo)->sda2BaseAddr);
	sizeFit = CHECK_LOW16_STRICT (value);
	value = (instr & ~RA_MSK) | GPR2_MSK | ((~value + 1) & 0xffff);
	}

    /*
     * Do some checking: R_PPC_EMB_SDA21 relocation must fit in
     * 21 bits, thus the offset must fit in 16 bits.
     */

    if (!sizeFit)
	{
        printErr ("Relocation offset does not fit in 16 bits.\n");
	return (ERROR);
	}

    *((UINT32 *)pAdrs) = value;

    return (OK);
    }

/*******************************************************************************
*
* elfPpcEmbSda2RelReloc - perform the R_PPC_EMB_SDA2REL relocation
* 
* This routine handles the R_PPC_EMB_SDA2REL relocation (uhalf16,
* S + A - SDA2_BASE).
*
* RETURNS : OK or ERROR if computed value can't be written down in target
*	    memory.
*/

LOCAL STATUS elfPpcEmbSda2RelReloc
    (
    void *	 pAdrs,		/* relocation address */
    Elf32_Rela * pRelocCmd,	/* points to a relocation structure */
    SYM_INFO_TBL symInfoTbl,	/* array of absolute symbol values and types */
    SEG_INFO *	 pSeg		/* section addresses and sizes */
    )
    {
    UINT32	 value;		/* relocation value */

    if ((symInfoTbl[ELF32_R_SYM (pRelocCmd->r_info)].type & SYM_SDA2)
				!= SYM_SDA2)
	{
        printErr ("Referenced symbol does not belong to SDA2.\n");
	return (ERROR);
	}

    value = (UINT32)symInfoTbl [ELF32_R_SYM (pRelocCmd->r_info)].pAddr +
	    pRelocCmd->r_addend -
	    (UINT32)(((SDA_INFO *)pSeg->pAdnlInfo)->sda2BaseAddr);

    *((UINT16 *)pAdrs) = (UINT16) value;

    return (OK);
    }

/*******************************************************************************
*
* elfPpcEmbRelSdaReloc - perform the R_PPC_EMB_RELSDA relocation
* 
* This routine handles the R_PPC_EMB_RELSDA relocation.
*
* RETURNS : OK or ERROR if computed value can't be written down in target
*	    memory.
*/

LOCAL STATUS elfPpcEmbRelSdaReloc
    (
    void *	 pAdrs,		/* relocation address */
    Elf32_Rela * pRelocCmd,	/* points to a relocation structure */
    SYM_INFO_TBL symInfoTbl,	/* array of absolute symbol values and types */
    SEG_INFO *	 pSeg		/* section addresses and sizes */
    )
    {
    UINT32	 value;		/* relocation value */
    BOOL	 symbolIsInSda;	/* TRUE if symbol belongs to the SDA area */

    /*
     * Only symbols (or reference to) belonging to a .sdata, .sbss, .sdata2
     * or .sbss2 section are supported.
     */

    switch (symInfoTbl[ELF32_R_SYM (pRelocCmd->r_info)].type & SYM_SDA_MASK)
	{
	case (SYM_SDA):
	    symbolIsInSda = TRUE;
	    break;

	case (SYM_SDA2):
	    symbolIsInSda = FALSE;
	    break;

	default:
	    printErr ("Referenced symbol does not belong to SDA/SDA2.\n");
	    return (ERROR);
	    break;
	}

    value = (UINT32)symInfoTbl [ELF32_R_SYM (pRelocCmd->r_info)].pAddr +
	    pRelocCmd->r_addend;

    if (symbolIsInSda)
	value -= (UINT32)(((SDA_INFO *)pSeg->pAdnlInfo)->sdaBaseAddr);
    else
	value -= (UINT32)(((SDA_INFO *)pSeg->pAdnlInfo)->sda2BaseAddr);

    /*
     * Do some checking: R_PPC_EMB_RELSDA relocation must fit in
     * 16 bits.
     */

    if (!CHECK_LOW16 (value))
	{
        printErr ("Relocation value does not fit in 16 bits.\n");
	return (ERROR);
	}

    *((UINT16 *)pAdrs) = (UINT16) value;

    return (OK);
    }
#endif 	/* INCLUDE_SDA */ 
#endif 	/* CPU_FAMILY == PPC */

#if	(CPU_FAMILY == SIMSPARCSOLARIS)

/*******************************************************************************
*
* elfSparcSegReloc - perform relocation for the PowerPC family
*
* This routine reads the specified relocation command segment and performs
* all the relocations specified therein. Only relocation command from sections
* with section type SHT_RELA are considered here.
*
* Absolute symbol addresses are looked up in the 'externals' table.
*
* RETURNS: OK or ERROR
*/

STATUS elfSparcSegReloc
    (
    int		 fd,		/* file to read in */
    MODULE_ID	 moduleId,	/* module id */
    int		 loadFlag,	/* not used */
    int		 posCurRelocCmd,/* position of current relocation command */
    Elf32_Shdr * pScnHdrTbl,	/* pointer to section header table */
    Elf32_Shdr * pRelHdr,	/* pointer to relocation section header */
    SCN_ADRS *	 pScnAddr,	/* section address once loaded */
    SYM_INFO_TBL symInfoTbl,	/* array of absolute symbol values and types */
    Elf32_Sym *	 pSymsArray,	/* pointer to symbols array */
    SYMTAB_ID 	 symTbl,	/* not used */
    SEG_INFO *	 pSeg		/* section addresses and sizes */
    )
    {
    Elf32_Shdr * pScnHdr;	/* section to which the relocations apply */
    Elf32_Rela   relocCmd;	/* relocation structure */
    UINT32	 relocNum;	/* number of reloc entries in section */
    UINT32	 relocIdx;	/* index of the reloc entry being processed */
    void *	 pAdrs;		/* relocation address */
    Elf32_Sym *	 pSym;		/* pointer to an external symbol */
    STATUS	 status = OK;	/* whether or not the relocation was ok */

    pScnHdr = pScnHdrTbl + pRelHdr->sh_info;

    /* Some sanity checking */

    if (pRelHdr->sh_type != SHT_RELA)
	{
	errnoSet (S_loadElfLib_RELA_SECTION);
	printErr ("Relocation sections of type %d are not supported.\n",
		     pRelHdr->sh_type);
	return (OK);
	}

    if (pRelHdr->sh_entsize != sizeof (Elf32_Rela))
	{
	printErr ("Wrong relocation entry size.\n");
	errnoSet (S_loadElfLib_RELA_SECTION);
	return (ERROR);
	}

    /* Relocation loop */

    relocNum = pRelHdr->sh_size / pRelHdr->sh_entsize;

    for (relocIdx = 0; relocIdx < relocNum; relocIdx++)
	{
	/* read relocation command */

	if ((posCurRelocCmd = 
	     elfRelocRelaEntryRd (fd, posCurRelocCmd, &relocCmd)) == ERROR)
	    {
	    errnoSet (S_loadElfLib_READ_SECTIONS);
	    return (ERROR);
	    }

	/*
	 * Calculate actual remote address that needs relocation, and
	 * perform external or section relative relocation.
	 */

	pAdrs = (void *)((Elf32_Addr)*pScnAddr + relocCmd.r_offset);

	pSym = pSymsArray + ELF32_R_SYM (relocCmd.r_info);

	/*
	 * System V ABI defines the following notations:
	 *
	 * A - the addend used to compute the value of the relocatable field.
	 * S - the value (address) of the symbol whose index resides in the
	 *     relocation entry. Note that this function uses the value stored
	 *     in the external symbol value array instead of the symbol's
	 *     st_value field.
	 * P - the place (section offset or address) of the storage unit being
	 *     relocated (computed using r_offset) prior to the relocation.
	 */

        switch (ELF32_R_TYPE (relocCmd.r_info))
            {
	    case (R_SPARC_8):
		if (elfSparc8Reloc (pAdrs, &relocCmd, symInfoTbl) != OK)
		    status = ERROR;
		break;

	    case (R_SPARC_16):
		if (elfSparc16Reloc (pAdrs, &relocCmd, symInfoTbl) != OK)
		    status = ERROR;
		break;

	    case (R_SPARC_32):
		if (elfSparc32Reloc (pAdrs, &relocCmd, symInfoTbl) != OK)
		    status = ERROR;
		break;

	    case (R_SPARC_DISP8):
		if (elfSparcDisp8Reloc(pAdrs, &relocCmd, symInfoTbl) != OK)
		    status = ERROR;
		break;

	    case (R_SPARC_DISP16):
		if (elfSparcDisp16Reloc(pAdrs, &relocCmd, symInfoTbl) != OK)
		    status = ERROR;
		break;

	    case (R_SPARC_DISP32):
		if (elfSparcDisp32Reloc(pAdrs, &relocCmd, symInfoTbl) != OK)
		    status = ERROR;
		break;

	    case (R_SPARC_WDISP30):
		if (elfSparcWDisp30Reloc (pAdrs, &relocCmd, symInfoTbl) != OK)
		    status = ERROR;
		break;

	    case (R_SPARC_WDISP22):
		if (elfSparcWDisp22Reloc (pAdrs, &relocCmd, symInfoTbl) != OK)
		    status = ERROR;
		break;

	    case (R_SPARC_HI22):
		if (elfSparcHi22Reloc (pAdrs, &relocCmd, symInfoTbl) != OK)
		    status = ERROR;
		break;

	    case (R_SPARC_22):
		if (elfSparc22Reloc (pAdrs, &relocCmd, symInfoTbl) != OK)
		    status = ERROR;
		break;

	    case (R_SPARC_13):
		if (elfSparc13Reloc (pAdrs, &relocCmd, symInfoTbl) != OK)
		    status = ERROR;
		break;

	    case (R_SPARC_LO10):
		if (elfSparcLo10Reloc (pAdrs, &relocCmd, symInfoTbl) != OK)
		    status = ERROR;
		break;

	    case (R_SPARC_PC10):
		if (elfSparcPc10Reloc (pAdrs, &relocCmd, symInfoTbl) != OK)
		    status = ERROR;
		break;

	    case (R_SPARC_PC22):
		if (elfSparcPc22Reloc (pAdrs, &relocCmd, symInfoTbl) != OK)
		    status = ERROR;
		break;

	    case (R_SPARC_UA32):
	    case (R_SPARC_GLOB_DAT):
		if (elfSparcUa32Reloc (pAdrs, &relocCmd, symInfoTbl) != OK)
		    status = ERROR;
		break;

	    default:
        	printErr ("Unsupported relocation type %d\n",
			    ELF32_R_TYPE (relocCmd.r_info));
        	status = ERROR;
        	break;
	    }
	}

    return (status);
    }

/*******************************************************************************
*
* elfSparcAddr32Reloc - perform the R_PPC_ADDR32 relocation
* 
* This routine handles the R_PPC_ADDR32 relocation (word32, S + A).
*
* RETURNS : OK or ERROR if computed value can't be written down in target
*	    memory.
*/

LOCAL STATUS elfSparc32Reloc
    (
    void *	 pAdrs,		/* relocation address */
    Elf32_Rela * pRelocCmd,	/* points to a relocation structure */
    SYM_INFO_TBL symInfoTbl	/* array of absolute symbol values */
    )
    {
    UINT32	 value;		/* relocation value */

    value = (UINT32)symInfoTbl [ELF32_R_SYM (pRelocCmd->r_info)].pAddr +
	    pRelocCmd->r_addend;

    *((UINT32 *)pAdrs) = value;

    return (OK);
    }

/*******************************************************************************
*
* elfSparcAddr24Reloc - perform the R_PPC_ADDR24 relocation
* 
* This routine handles the R_PPC_ADDR24 relocation (low24, S + A).
*
* RETURNS : OK or ERROR if computed value can't be written down in target
*	    memory.
*/

LOCAL STATUS elfSparc16Reloc
    (
    void *	 pAdrs,		/* relocation address */
    Elf32_Rela * pRelocCmd,	/* points to a relocation structure */
    SYM_INFO_TBL symInfoTbl	/* array of absolute symbol values */
    )
    {
    UINT32	 value;		/* relocation value */

    value = (UINT32)symInfoTbl [ELF32_R_SYM (pRelocCmd->r_info)].pAddr +
	    pRelocCmd->r_addend;

    *((UINT16 *)pAdrs) = (UINT16) value;

    return (OK);
    }

/*******************************************************************************
*
* elfSparcAddr8Reloc - perform the R_PPC_ADDR16 relocation
* 
* This routine handles the R_PPC_ADDR16 relocation (half16, S + A).
*
* RETURNS : OK or ERROR if computed value can't be written down in target
*	    memory.
*/

LOCAL STATUS elfSparc8Reloc
    (
    void *	 pAdrs,		/* relocation address */
    Elf32_Rela * pRelocCmd,	/* points to a relocation structure */
    SYM_INFO_TBL symInfoTbl	/* array of absolute symbol values */
    )
    {
    UINT32	 value;		/* relocation value */

    value = (UINT32)symInfoTbl [ELF32_R_SYM (pRelocCmd->r_info)].pAddr +
	    pRelocCmd->r_addend;

    *((UINT8 *)pAdrs) = (UINT8) value;

    return (OK);
    }

/*******************************************************************************
*
* elfSparcAddr16LoReloc - perform the R_PPC_ADDR16_LO relocation
* 
* This routine handles the R_PPC_ADDR16_LO relocation (half16, #lo(S + A)).
*
* RETURNS : OK or ERROR if computed value can't be written down in target
*	    memory.
*/

LOCAL STATUS elfSparcDisp32Reloc
    (
    void *	 pAdrs,		/* relocation address */
    Elf32_Rela * pRelocCmd,	/* points to a relocation structure */
    SYM_INFO_TBL symInfoTbl	/* array of absolute symbol values */
    )
    {
    UINT32	 value;		/* relocation value */
    UINT32	 tmpVal;	/* holds temporary calculation results */

    value = (UINT32) symInfoTbl [ELF32_R_SYM (pRelocCmd->r_info)].pAddr +
	    pRelocCmd->r_addend - ((UINT32) pAdrs);

    *((UINT32 *)pAdrs) = value;

    return (OK);
    }

/*******************************************************************************
*
* elfSparcAddr16HiReloc - perform the R_PPC_ADDR16_HI relocation
* 
* This routine handles the R_PPC_ADDR16_HI relocation (half16, #hi(S + A)).
*
* RETURNS : OK or ERROR if computed value can't be written down in target
*	    memory.
*/

LOCAL STATUS elfSparcDisp16Reloc
    (
    void *	 pAdrs,		/* relocation address */
    Elf32_Rela * pRelocCmd,	/* points to a relocation structure */
    SYM_INFO_TBL symInfoTbl	/* array of absolute symbol values */
    )
    {
    UINT32	 value;		/* relocation value */

    value = (UINT32) symInfoTbl [ELF32_R_SYM (pRelocCmd->r_info)].pAddr +
	    pRelocCmd->r_addend - ((UINT32) pAdrs);

    *((UINT32 *)pAdrs) = value;

    return (OK);
    }

/*******************************************************************************
*
* elfSparcAddr16HaReloc - perform the R_PPC_ADDR16_HA relocation
* 
* This routine handles the R_PPC_ADDR16_HA relocation (half16, #ha(S + A)).
*
* RETURNS : OK or ERROR if computed value can't be written down in target
*	    memory.
*/

LOCAL STATUS elfSparcDisp8Reloc
    (
    void *	 pAdrs,		/* relocation address */
    Elf32_Rela * pRelocCmd,	/* points to a relocation structure */
    SYM_INFO_TBL symInfoTbl	/* array of absolute symbol values */
    )
    {
    UINT32	 value;		/* relocation value */

    value = (UINT32) symInfoTbl [ELF32_R_SYM (pRelocCmd->r_info)].pAddr +
	    pRelocCmd->r_addend - ((UINT32) pAdrs);

    *((UINT32 *)pAdrs) = value;

    return (OK);
    }

/*******************************************************************************
*
* elfSparcAddr14Reloc - perform the R_PPC_ADDR14 relocation
* 
* This routine handles the R_PPC_ADDR14 relocation (low14, (S + A) >> 2).
*
* RETURNS : OK or ERROR if computed value can't be written down in target
*	    memory.
*/

LOCAL STATUS elfSparcWDisp30Reloc
    (
    void *	 pAdrs,		/* relocation address */
    Elf32_Rela * pRelocCmd,	/* points to a relocation structure */
    SYM_INFO_TBL symInfoTbl	/* array of absolute symbol values */
    )
    {
    unsigned int value, tmpVal;

    value = ((UINT32)symInfoTbl [ELF32_R_SYM (pRelocCmd->r_info)].pAddr +
            pRelocCmd->r_addend - ((UINT32)pAdrs))>>2;

    tmpVal = *((UINT32 *) pAdrs);

    tmpVal &= (unsigned int)0xc0000000;
    tmpVal |= (unsigned int)value;

    *((UINT32 *)pAdrs) = (UINT32) tmpVal;

    return (OK);
    }

/*******************************************************************************
*
* elfSparcRel24Reloc - perform the R_PPC_REL24 relocation
* 
* This routine handles the R_PPC_REL24 relocation (low24, (S + A - P) >> 2).
*
* RETURNS : OK or ERROR if computed value can't be written down in target
*	    memory.
*/

LOCAL STATUS elfSparcWDisp22Reloc
    (
    void *	 pAdrs,		/* relocation address */
    Elf32_Rela * pRelocCmd,	/* points to a relocation structure */
    SYM_INFO_TBL symInfoTbl	/* array of absolute symbol values */
    )
    {
    unsigned int value, tmpVal;

    value = ((UINT32)symInfoTbl [ELF32_R_SYM (pRelocCmd->r_info)].pAddr +
            pRelocCmd->r_addend - ((UINT32)pAdrs))>>2;

    tmpVal = *((UINT32 *) pAdrs);

    tmpVal &= (unsigned int)0xffc00000;
    tmpVal |= (unsigned int)value;

    *((UINT32 *)pAdrs) = (UINT32) tmpVal;

    return (OK);
    }

/*******************************************************************************
*
* elfSparcRel14Reloc - perform the R_PPC_REL14 relocation
* 
* This routine handles the R_PPC_REL14 relocation (low14, (S + A - P) >> 2).
*
* RETURNS : OK or ERROR if computed value can't be written down in target
*	    memory.
*/

LOCAL STATUS elfSparcHi22Reloc
    (
    void *	 pAdrs,		/* relocation address */
    Elf32_Rela * pRelocCmd,	/* points to a relocation structure */
    SYM_INFO_TBL symInfoTbl	/* array of absolute symbol values */
    )
    {
    unsigned int value, tmpVal;

    value = ((UINT32)symInfoTbl [ELF32_R_SYM (pRelocCmd->r_info)].pAddr +
            pRelocCmd->r_addend)>>10;

    tmpVal = *((UINT32 *) pAdrs);

    tmpVal &= (unsigned int)0xffc00000;
    tmpVal |= (unsigned int)value;

    *((UINT32 *)pAdrs) = (UINT32) tmpVal;

    return (OK);
    }

/*******************************************************************************
*
* elfSparcUaddr32Reloc - perform the R_PPC_UADDR32 relocation
* 
* This routine handles the R_PPC_UADDR32 relocation (word32, S + A).
*
* RETURNS : OK or ERROR if computed value can't be written down in target
*	    memory.
*/

LOCAL STATUS elfSparc22Reloc
    (
    void *	 pAdrs,		/* relocation address */
    Elf32_Rela * pRelocCmd,	/* points to a relocation structure */
    SYM_INFO_TBL symInfoTbl	/* array of absolute symbol values */
    )
    {
    unsigned int value, tmpVal;

    value = (UINT32)symInfoTbl [ELF32_R_SYM (pRelocCmd->r_info)].pAddr +
            pRelocCmd->r_addend;

    tmpVal = *((UINT32 *) pAdrs);

    tmpVal &= (unsigned int)0xffc00000;
    tmpVal |= (unsigned int)value;

    *((UINT32 *)pAdrs) = (UINT32) tmpVal;

    return (OK);
    }

/*******************************************************************************
*
* elfSparcUaddr16Reloc - perform the R_PPC_UADDR16 relocation
* 
* This routine handles the R_PPC_UADDR16 relocation (half16, S + A).
*
* RETURNS : OK or ERROR if computed value can't be written down in target
*	    memory.
*/

LOCAL STATUS elfSparc13Reloc
    (
    void *	 pAdrs,		/* relocation address */
    Elf32_Rela * pRelocCmd,	/* points to a relocation structure */
    SYM_INFO_TBL symInfoTbl	/* array of absolute symbol values */
    )
    {
    unsigned int value, tmpVal;

    value = (UINT32)symInfoTbl [ELF32_R_SYM (pRelocCmd->r_info)].pAddr +
            pRelocCmd->r_addend;

    tmpVal = *((UINT32 *) pAdrs);

    tmpVal &= (unsigned int)0xfffe0000;
    tmpVal |= (unsigned int)value;

    *((UINT32 *)pAdrs) = (UINT32) tmpVal;

    return (OK);
    }

/*******************************************************************************
*
* elfSparcRel32Reloc - perform the R_PPC_REL32 relocation
* 
* This routine handles the R_PPC_REL32 relocation (word32, S + A - P).
*
* RETURNS : OK or ERROR if computed value can't be written down in target
*	    memory.
*/

LOCAL STATUS elfSparcLo10Reloc
    (
    void *	 pAdrs,		/* relocation address */
    Elf32_Rela * pRelocCmd,	/* points to a relocation structure */
    SYM_INFO_TBL symInfoTbl	/* array of absolute symbol values */
    )
    {
    unsigned int value, tmpVal;

    value = ((UINT32)symInfoTbl [ELF32_R_SYM (pRelocCmd->r_info)].pAddr +
            pRelocCmd->r_addend) & 0x3ff;

    tmpVal = *((UINT32 *) pAdrs);

    tmpVal &= (unsigned int)0xfffffc00;        
    tmpVal |= (unsigned int)value;

    *((UINT32 *)pAdrs) = tmpVal;

    return (OK);
    }

/*******************************************************************************
*
* elfSparcEmbNaddr32Reloc - perform the R_PPC_EMB_NADDR32 relocation
* 
* This routine handles the R_PPC_EMB_NADDR32 relocation (uword32, A - S).
*
* RETURNS : OK or ERROR if computed value can't be written down in target
*	    memory.
*/

LOCAL STATUS elfSparcPc10Reloc
    (
    void *	 pAdrs,		/* relocation address */
    Elf32_Rela * pRelocCmd,	/* points to a relocation structure */
    SYM_INFO_TBL symInfoTbl	/* array of absolute symbol values */
    )
    {
    unsigned int value, tmpVal;

    value = ((UINT32)symInfoTbl [ELF32_R_SYM (pRelocCmd->r_info)].pAddr +
            pRelocCmd->r_addend - ((UINT32)pAdrs))&0x3ff;

    tmpVal = *((UINT32 *) pAdrs);

    tmpVal &= (unsigned int)0xfffffc00;        
    tmpVal |= (unsigned int)value;

    *((UINT32 *)pAdrs) = (UINT32) tmpVal;

    return (OK);
    }

/*******************************************************************************
*
* elfSparcEmbNaddr16Reloc - perform the R_PPC_EMB_NADDR16 relocation
* 
* This routine handles the R_PPC_EMB_NADDR16 relocation (uhalf16, A - S).
*
* RETURNS : OK or ERROR if computed value can't be written down in target
*	    memory.
*/

LOCAL STATUS elfSparcPc22Reloc
    (
    void *	 pAdrs,		/* relocation address */
    Elf32_Rela * pRelocCmd,	/* points to a relocation structure */
    SYM_INFO_TBL symInfoTbl	/* array of absolute symbol values */
    )
    {
    unsigned int value, tmpVal;

    value = ((UINT32)symInfoTbl [ELF32_R_SYM (pRelocCmd->r_info)].pAddr +
            pRelocCmd->r_addend - ((UINT32)pAdrs))>>10;

    tmpVal = *((UINT32 *) pAdrs);

    tmpVal &= (unsigned int)0xffc00000;
    tmpVal |= (unsigned int)value;

    *((UINT32 *)pAdrs) = (UINT32) tmpVal;

    return (OK);
    }

/*******************************************************************************
*
* elfSparcEmbNaddr16LoReloc - perform the R_PPC_EMB_NADDR16_LO relocation
* 
* This routine handles the R_PPC_EMB_NADDR16_LO relocation (uhalf16,
* #lo (A - S)).
*
* RETURNS : OK or ERROR if computed value can't be written down in target
*	    memory.
*/

LOCAL STATUS elfSparcUa32Reloc
    (
    void *	 pAdrs,		/* relocation address */
    Elf32_Rela * pRelocCmd,	/* points to a relocation structure */
    SYM_INFO_TBL symInfoTbl	/* array of absolute symbol values */
    )
    {
    unsigned int value;

    value = (UINT32)symInfoTbl [ELF32_R_SYM (pRelocCmd->r_info)].pAddr +
            pRelocCmd->r_addend;

    *((UINT32 *)pAdrs) = (UINT32) value;

    return (OK);
    }

#endif	/* (CPU_FAMILY == SIMSPARCSOLARIS) */

#if	(CPU_FAMILY == COLDFIRE)
/*******************************************************************************
*
* elfM68kSegReloc - perform relocation for the M68k family
*
* This routine reads the specified relocation command segment and performs
* all the relocations specified therein. Only relocation command from sections
* with section type SHT_RELA are considered here.
*
* Absolute symbol addresses are looked up in the 'externals' table.
*
* RETURNS: OK or ERROR
*/

STATUS elfM68kSegReloc
    (
    int		 fd,		/* file to read in */
    MODULE_ID	 moduleId,	/* module id */
    int		 loadFlag,	/* not used */
    int		 posCurRelocCmd,/* position of current relocation command */
    Elf32_Shdr * pScnHdrTbl,	/* pointer to section header table */
    Elf32_Shdr * pRelHdr,	/* pointer to relocation section header */
    SCN_ADRS *	 pScnAddr,	/* section address once loaded */
    SYM_INFO_TBL symInfoTbl,	/* array of absolute symbol values and types */
    Elf32_Sym *	 pSymsArray,	/* pointer to symbols array */
    SYMTAB_ID 	 symTbl,	/* not used */
    SEG_INFO *	 pSeg		/* section addresses and sizes */
    )
    {
    Elf32_Shdr * pScnHdr;	/* section to which the relocations apply */
    Elf32_Rela   relocCmd;	/* relocation structure */
    UINT32	 relocNum;	/* number of reloc entries in section */
    UINT32	 relocIdx;	/* index of the reloc entry being processed */
    void *	 pAdrs;		/* relocation address */
    Elf32_Sym *	 pSym;		/* pointer to an external symbol */
    STATUS	 status = OK;	/* whether or not the relocation was ok */

    pScnHdr = pScnHdrTbl + pRelHdr->sh_info;

    /* Some sanity checking */

    if (pRelHdr->sh_type != SHT_RELA)
	{
	errnoSet (S_loadElfLib_RELA_SECTION);
	printErr ("Relocation sections of type %d are not supported.\n",
		     pRelHdr->sh_type);
	return (OK);
	}

    if (pRelHdr->sh_entsize != sizeof (Elf32_Rela))
	{
	printErr ("Wrong relocation entry size.\n");
	errnoSet (S_loadElfLib_RELA_SECTION);
	return (ERROR);
	}

    /* Relocation loop */

    relocNum = pRelHdr->sh_size / pRelHdr->sh_entsize;

    for (relocIdx = 0; relocIdx < relocNum; relocIdx++)
	{
	/* read relocation command */

	posCurRelocCmd = elfRelocRelaEntryRd (fd, posCurRelocCmd, &relocCmd);

	/*
	 * Calculate actual remote address that needs relocation, and
	 * perform external or section relative relocation.
	 */

	pAdrs = (void *)((Elf32_Addr)*pScnAddr + relocCmd.r_offset);

	pSym = pSymsArray + ELF32_R_SYM (relocCmd.r_info);

	/*
	 * System V ABI defines the following notations:
	 *
	 * A - the addend used to compute the value of the relocatable field.
	 * S - the value (address) of the symbol whose index resides in the
	 *     relocation entry. Note that this function uses the value stored
	 *     in the external symbol value array instead of the symbol's
	 *     st_value field.
	 * P - the place (section offset or address) of the storage unit being
	 *     relocated (computed using r_offset) prior to the relocation.
	 */

        switch (ELF32_R_TYPE (relocCmd.r_info) & 0x7f)
            {
	    case R_68K_32:
		if (elfM68k32Reloc (pAdrs, &relocCmd, symInfoTbl) != OK)
		    status = ERROR;
		break;

	    case R_68K_16:
		if (elfM68k16Reloc (pAdrs, &relocCmd, symInfoTbl) != OK)
		    status = ERROR;
		break;

	    case R_68K_PC32:
		if (elfM68kDisp32Reloc (pAdrs, &relocCmd, symInfoTbl) != OK)
		    status = ERROR;
		break;

	    case R_68K_PC16:
		if (elfM68kDisp16Reloc (pAdrs, &relocCmd, symInfoTbl) != OK)
		    status = ERROR;
		break;

	    default:
        	printErr ("Unsupported relocation type %d\n",
			    ELF32_R_TYPE (relocCmd.r_info));
        	status = ERROR;
        	break;
	    }
	}

    return (status);
    }

/******************************************************************************
*
* elfM68k32Reloc - perform the R_68K_32 relocation
* 
* This routine handles the R_68K_32 relocation (S + A).
*
* RETURNS : OK or ERROR if computed value can't be written down in target
*	    memory.
*/

LOCAL STATUS elfM68k32Reloc
    (
    void *	 pAdrs,		/* relocation address */
    Elf32_Rela * pRelocCmd,	/* points to a relocation structure */
    SYM_INFO_TBL symInfoTbl	/* array of absolute symbol values */
    )
    {
    UINT32	 value;		/* relocation value */

    value = (UINT32)symInfoTbl [ELF32_R_SYM (pRelocCmd->r_info)].pAddr +
	    pRelocCmd->r_addend;

    *((UINT32 *)pAdrs) = (UINT32) value;

    return (OK);
    }

/******************************************************************************
*
* elfM68k16Reloc - perform the R_68K_16 relocation
* 
* This routine handles the R_68K_16 relocation (S + A).
*
* RETURNS : OK or ERROR if computed value can't be written down in target
*	    memory.
*/

LOCAL STATUS elfM68k16Reloc
    (
    void *	 pAdrs,		/* relocation address */
    Elf32_Rela * pRelocCmd,	/* points to a relocation structure */
    SYM_INFO_TBL symInfoTbl	/* array of absolute symbol values */
    )
    {
    UINT32	 value;		/* relocation value */

    value = (UINT32)symInfoTbl [ELF32_R_SYM (pRelocCmd->r_info)].pAddr +
	    pRelocCmd->r_addend;

    *((UINT16 *)pAdrs) = (UINT16) value;

    return (OK);
    }

/*******************************************************************************
*
* elfM68kDisp32Reloc - perform the R_68K_PC32 relocation
* 
* This routine handles the R_68K_PC32 relocation (S + A - P).
*
* RETURNS : OK or ERROR if computed value can't be written down in target
*	    memory.
*/

LOCAL STATUS elfM68kDisp32Reloc
    (
    void *	 pAdrs,		/* relocation address */
    Elf32_Rela * pRelocCmd,	/* points to a relocation structure */
    SYM_INFO_TBL symInfoTbl	/* array of absolute symbol values */
    )
    {
    UINT32	 value;		/* relocation value */

    value = (UINT32) symInfoTbl [ELF32_R_SYM (pRelocCmd->r_info)].pAddr +
	    pRelocCmd->r_addend - ((UINT32) pAdrs);

    *((UINT32 *)pAdrs) = value;

    return (OK);
    }

/*******************************************************************************
*
* elfM68kDisp16Reloc - perform the R_68K_PC16 relocation
* 
* This routine handles the R_68K_PC16 relocation (S + A - P).
*
* RETURNS : OK or ERROR if computed value can't be written down in target
*	    memory.
*/

LOCAL STATUS elfM68kDisp16Reloc
    (
    void *	 pAdrs,		/* relocation address */
    Elf32_Rela * pRelocCmd,	/* points to a relocation structure */
    SYM_INFO_TBL symInfoTbl	/* array of absolute symbol values */
    )
    {
    UINT32	 value;		/* relocation value */

    value = (UINT32) symInfoTbl [ELF32_R_SYM (pRelocCmd->r_info)].pAddr +
	    pRelocCmd->r_addend - ((UINT32) pAdrs);

    *((UINT16 *)pAdrs) = (UINT16) value;

    return (OK);
    }
#endif	/* (CPU_FAMILY == COLDFIRE) */

#if (CPU_FAMILY == SH)
/*******************************************************************************
*
* elfShSegReloc - perform relocation for the Super Hitachi family
*
* This routine reads the specified relocation command segment and performs
* all the relocations specified therein. Only relocation command from sections
* with section type SHT_RELA are considered here.
*
* Absolute symbol addresses are looked up in the 'externals' table.
*
* This function handles the following types of relocation commands
* for the SH processor:
*
*  R_SH_NONE
*  R_SH_DIR32
*  R_SH_REL32
*  R_SH_DIR8WPN
*  R_SH_IND12W
*  R_SH_DIR8WPL
*  R_SH_DIR8WPZ
*  R_SH_DIR8BP
*  R_SH_DIR8W
*  R_SH_DIR8L
*
*  The remaining relocs are a GNU extension used for relaxation.  We
*  use the same constants as COFF uses, not that it really matters.  
*
*  R_SH_SWITCH16
*  R_SH_SWITCH32
*  R_SH_USES
*  R_SH_COUNT
*  R_SH_ALIGN
*  R_SH_CODE
*  R_SH_DATA
*  R_SH_LABEL
*
*
* RETURNS: OK or ERROR
*/

LOCAL STATUS elfShSegReloc
    (
    int		 fd,		/* file to read in */
    MODULE_ID	 moduleId,	/* module id */
    int		 loadFlag,	/* not used */
    int	 	 posCurRelocCmd,/* position of current relocation command */
    Elf32_Shdr * pScnHdrTbl,	/* not used */
    Elf32_Shdr * pRelHdr,	/* pointer to relocation section header */
    SCN_ADRS *	 pScnAddr,	/* section address once loaded */
    SYM_INFO_TBL symInfoTbl,	/* array of absolute symbol values and types */
    Elf32_Sym *	 pSymsArray,	/* pointer to symbols array */
    SYMTAB_ID 	 symTbl,	/* not used */
    SEG_INFO *	 pSeg		/* section addresses and sizes */
    )
    {
    Elf32_Rela   relocCmd;	/* relocation structure */
    UINT32	 relocNum;	/* number of reloc entries in section */
    UINT32	 relocIdx;	/* index of the reloc entry being processed */
    void *	 pAdrs;		/* relocation address */
    Elf32_Sym *	 pSym;		/* pointer to an external symbol */
    STATUS	 status = OK;	/* whether or not the relocation was ok */

    /* Some sanity checking */
    if (pRelHdr->sh_type != SHT_RELA)
	{
        errnoSet (S_loadElfLib_RELA_SECTION);
	return (OK);
	}

    if (pRelHdr->sh_entsize != sizeof (Elf32_Rela))
	{
        errnoSet (S_loadElfLib_RELA_SECTION);
	return (ERROR);
	}

    /* Relocation loop */

    relocNum = pRelHdr->sh_size / pRelHdr->sh_entsize;

    for (relocIdx = 0; relocIdx < relocNum; relocIdx++)
	{
	/* read relocation command */

	if ((posCurRelocCmd = 
	     elfRelocRelaEntryRd (fd, posCurRelocCmd, &relocCmd)) == ERROR)
	    {
	    errnoSet (S_loadElfLib_READ_SECTIONS);
	    return (ERROR);
	    }

	/*
	 * Calculate actual remote address that needs relocation, and
	 * perform external or section relative relocation.
	 */

	pAdrs = (void *)((Elf32_Addr)*pScnAddr + relocCmd.r_offset);

	pSym = pSymsArray + ELF32_R_SYM (relocCmd.r_info);

	/*
	 * System V ABI defines the following notations:
	 *
	 * A - the addend used to compute the value of the relocatable field.
	 * S - the value (address) of the symbol whose index resides in the
	 *     relocation entry. Note that this function uses the value stored
	 *     in the external symbol value array instead of the symbol's
	 *     st_value field.
	 * P - the place (section offset or address) of the storage unit being
	 *     relocated (computed using r_offset) prior to the relocation.
	 */

        switch (ELF32_R_TYPE (relocCmd.r_info))
            {
	    case (R_SH_NONE):				/* none */
		break;

	    case (R_SH_DIR32):

		if (elfShDir32Reloc (pAdrs, &relocCmd, symInfoTbl,
				       moduleId) != OK)
		    status = ERROR;
		break;

	    default:
        	printErr ("Unsupported relocation type %d\n",
			      ELF32_R_TYPE (relocCmd.r_info));
        	status = ERROR;
        	break;
	    }
	}

    return (status);
    }

/*******************************************************************************
*
* elfShDir32Reloc - perform the R_SH_DIR32 relocation
* 
* This routine handles the R_SH_DIR32 relocation (word32, S + A).
*
* RETURNS : OK or ERROR if computed value can't be written down in target
*	    memory.
*/

LOCAL STATUS elfShDir32Reloc
    (
    void *	 pAdrs,		/* relocation address */
    Elf32_Rela * pRelocCmd,	/* points to a relocation structure */
    SYM_INFO_TBL symInfoTbl,	/* array of absolute symbol values and types */
    MODULE_ID	 moduleId	/* module id */
    )
    {
    UINT32	 value;		/* relocation value */
    UINT32	 oldValue;


    oldValue = *(UINT32 *)pAdrs;

    value = (UINT32)symInfoTbl [ELF32_R_SYM (pRelocCmd->r_info)].pAddr +
	    pRelocCmd->r_addend + oldValue;


    *((UINT32 *)pAdrs) = value;

    return (OK);
    }

#endif /* CPU_FAMILY == SH */

/*******************************************************************************
*
* loadElfInit - initialize the system for ELF load modules
*
* This routine initializes VxWorks to use the ELF (executable and linking format)
* object module format for loading modules.
*
* RETURNS: OK (always)
*
* SEE ALSO: loadModuleAt()
*/

STATUS loadElfInit (void)
    {
    loadRoutine = (FUNCPTR)loadElfFmtManage;

#if (CPU_FAMILY == I80X86)
    elfI86Init (&pElfModuleVerifyRtn, &pElfSegRelRtn);
#endif /* CPU_FAMILY */

#if (CPU_FAMILY == ARM)
    elfArmInit (&pElfModuleVerifyRtn, &pElfSegRelRtn);
#endif /* CPU_FAMILY */

#if (CPU_FAMILY == MIPS)
    elfMipsInit (&pElfModuleVerifyRtn, &pElfSegRelRtn);
#endif /* CPU_FAMILY */

#ifdef INCLUDE_SDA
    /* Create new memory partitions for the SDA areas */

    if ((EM_ARCH_MACHINE == EM_PPC) || ((EM_ARCH_MACHINE == EM_SH))
    	if (loadElfSdaCreate() == ERROR)
	    return ERROR;
#endif /* INCLUDE_SDA */

    return OK;
    }

/******************************************************************************
*
* loadElfModuleIsOk - check the object module format
*
* This routine contains the heuristic required to determine if the object
* file belongs to the OMF handled by this OMF reader, with care for the target
* architecture.
* It is the underlying routine for loadElfFmtCheck().
*
* RETURNS: TRUE or FALSE if something prevent the loader to handle the module.
*/

LOCAL BOOL loadElfModuleIsOk
    (
    Elf32_Ehdr *	pHdr		/* ELF module header */
    )
    {
    BOOL        moduleIsForTarget = FALSE;      /* TRUE if for target*/
    UINT16      targetMachine;          	/* target of module */
    BOOL	dummy = FALSE;

    /* Check that the module begins with "\0x7fELF" */

    if (strncmp ((char *) pHdr->e_ident, (char *) ELFMAG, SELFMAG) != 0)
	{
	printErr ("Unknown object module format \n");
	return FALSE;
	}

    /*
     * Get the target machine identification and checks that the object
     * module is appropriate for the architecture.
     * Note that the switch statement below should disappear when all
     * reloction units will offer an initialization routine.
     */

    targetMachine = pHdr->e_machine;

    if (pElfModuleVerifyRtn != NULL)
	{
#ifdef INCLUDE_SDA
	moduleIsForTarget = pElfModuleVerifyRtn (targetMachine, &sdaIsRequired);
#else
	moduleIsForTarget = pElfModuleVerifyRtn (targetMachine, &dummy);
#endif	/* INCLUDE_SDA */
	}
    else if (targetMachine == EM_ARCH_MACHINE)
	{
	moduleIsForTarget = TRUE;
#if (EM_ARCH_MACHINE == EM_PPC) && defined(INCLUDE_SDA)
	sdaIsRequired = TRUE;
#endif	/* INCLUDE_SDA */
	}

    return moduleIsForTarget;
    }

/******************************************************************************
*
* loadElfMdlHdrCheck - check the module header
*
* This routine checks the validity of a file header structure.
*
* RETURNS : OK, or ERROR if the header is not correct.
*/

LOCAL STATUS loadElfMdlHdrCheck
    (
    Elf32_Ehdr *	pHdr		/* ptr on header structure to check */
    )
    {
    if (pHdr->e_ehsize != sizeof (*pHdr))
	{
	printErr ("Incorrect ELF header size: %d\n", pHdr->e_ehsize);
	return (ERROR);
	}

    if ((pHdr->e_type != ET_REL) && (pHdr->e_type != ET_EXEC))
	{
	printErr ("Incorrect module type: %d\n", pHdr->e_type);
	return (ERROR);
	}

    if ((pHdr->e_type == ET_EXEC) && (pHdr->e_phnum == 0))
	{
	printErr ("No program header table in executable module.\n");
	return (ERROR);
	}

    if ((pHdr->e_phnum != 0) && (pHdr->e_phoff == 0))
	{
	printErr ("Null offset to program header table.\n");
	return (ERROR);
	}

    if ((pHdr->e_type == ET_EXEC) && (pHdr->e_phentsize != sizeof (Elf32_Phdr)))
	{
	printErr ("Incorrect program header size: %d\n", pHdr->e_phentsize);
	return (ERROR);
	}

    if (pHdr->e_shentsize != sizeof (Elf32_Shdr))
	{
	printErr ("Incorrect section header size: %d\n", pHdr->e_shentsize);
	return (ERROR);
	}

    if ((pHdr->e_shnum != 0) && (pHdr->e_shoff == 0))
	{
	printErr ("Null offset to section header table.\n");
	return (ERROR);
	}

    return (OK);
    }

/******************************************************************************
*
* loadElfMdlHdrRd - "Read in" the module header
*
* This routine fills a file header structure with information from the object
* file.
*
* RETURNS : OK, or ERROR if the header is not correct.
*/

LOCAL STATUS loadElfMdlHdrRd
    (
    int 		fd,		/* file to read in */
    Elf32_Ehdr *	pHdr		/* ptr on header structure to fill */
    )
    {
    ioctl(fd, FIOSEEK, 0);

    if (fioRead (fd, (char *) pHdr, sizeof (Elf32_Ehdr)) 
				!= sizeof (Elf32_Ehdr))
    	return (ERROR);

    if (loadElfMdlHdrCheck (pHdr) != OK)
	return (ERROR);

    return (OK);
    }

/******************************************************************************
*
* loadElfProgHdrCheck - check a program header
*
* This routine checks the validity of a program header structure.
*
* RETURNS : OK, or ERROR if the header is not correct.
*/

LOCAL STATUS loadElfProgHdrCheck
    (
    Elf32_Phdr *	pProgHdr,	/* pointer to a program header */
    int			progHdrNum	/* number of the program header */
    )
    {
    if (!CHECK_2_ALIGN (pProgHdr->p_align))	/* align must be power of 2 */
	{
        errnoSet (S_loadElfLib_HDR_ERROR);
	return (ERROR);
	}

    return (OK);
    }

/******************************************************************************
*
* loadElfProgHdrTblRd - "Read in" the program header table
*
* This routine fills a program header structure with information from the
* object file.
* Each program header in the program header table is processed in turn.
*
* RETURNS : OK, or ERROR if a program header is not correct.
*/

LOCAL STATUS loadElfProgHdrTblRd
    (
    int          	fd,           	/* file to read in */
    int          	posProgHdrField,/* positon of first program header */
    Elf32_Phdr *	pProgHdrTbl,	/* pointer to program header table */
    int                 progHdrNumber  	/* number of header in table */
    )
    {
    int			progHdrIndex;	/* loop counter */
    Elf32_Phdr *	pProgHdr;	/* pointer to a program header */

    /* Read all the programs header */

    ioctl (fd, FIOSEEK, posProgHdrField);

    if (fioRead (fd, (char *) pProgHdrTbl, progHdrNumber * sizeof (Elf32_Phdr)) 
					!= progHdrNumber * sizeof (Elf32_Phdr))
    	{
    	errnoSet (S_loadElfLib_HDR_ERROR);
    	return (ERROR);
    	}

    /* loop thru all the program headers */

    for (progHdrIndex = 0; progHdrIndex < progHdrNumber; progHdrIndex++)
	{
	pProgHdr = pProgHdrTbl + progHdrIndex;

        /* Check the program header */

	if (loadElfProgHdrCheck (pProgHdr, progHdrIndex) != OK)
	    return (ERROR);
	}

    return (OK);
    }

/******************************************************************************
*
* loadElfScnHdrCheck - check a section header
*
* This routine checks the validity of a section header structure.
*
* RETURNS : OK, or ERROR if the header is not correct.
*/

LOCAL STATUS loadElfScnHdrCheck
    (
    Elf32_Shdr *	pScnHdr,	/* pointer to a section header */
    int			scnHdrNum	/* number of the section header */
    )
    {
    if (!CHECK_2_ALIGN (pScnHdr->sh_addralign))	/* align must be power of 2 */
	{
        errnoSet (S_loadElfLib_SHDR_READ);
	return (ERROR);
	}

    return (OK);
    }

/******************************************************************************
*
* loadElfScnHdrIdxDispatch - dispatch the section header index
*
* This routine dispatches the section header into various "databases" depending
* on the section type. Only the section header index in the section header
* table is registred in the "databases".
*
* RETURNS : OK, or ERROR
*/

LOCAL STATUS loadElfScnHdrIdxDispatch
    (
    Elf32_Shdr * pScnHdrTbl,		/* pointer to section header table */
    int		 scnHdrIdx,		/* index of current section header */
    IDX_TBLS   * pIndexTables		/* pointer to section index tables */
    )
    {
    Elf32_Shdr * pScnHdr;		/* pointer to a section header */
    Elf32_Shdr * pAllocScnHdr;		/* ptr to header of allocatable scn */
    static int	 loadSections = 0;	/* counter of text, data, literal or */
					/* bss section headers */
    static int	 symTableSections = 0;	/* counter of symbol table section */
					/* headers */
    static int	 relocSections = 0;	/* counter of relocation information */
					/* section headers */
    static int	 strTableSections = 0;	/* counter of string table section */
					/* headers */

    /* Reinitialization of counters */

    if (pScnHdrTbl == NULL)
	{
	loadSections = 0;
	symTableSections = 0;
	relocSections = 0;
	strTableSections = 0;

	return (OK);
	}

    /* Get current section header */

    pScnHdr = pScnHdrTbl + scnHdrIdx;

    /* Dispatch the section headers index among the "databases" */

    switch (pScnHdr->sh_type)
        {
        case (SHT_PROGBITS):		/* text, data, literal sections */
        case (SHT_NOBITS):		/* bss sections */

	    /*
	     * Just consider the sections that will be allocated in the
	     * target memory. Thus debug sections are discarded.
	     */

	    if (pScnHdr->sh_flags & SHF_ALLOC)
	        pIndexTables->pLoadScnHdrIdxs [loadSections++] = scnHdrIdx;
	    break;

        case (SHT_SYMTAB):		/* symbol table sections */
	    pIndexTables->pSymTabScnHdrIdxs [symTableSections++] = scnHdrIdx;
	    break;

        case (SHT_DYNSYM):		/* minimal symbol table */
	    printErr ("Section type SHT_DYNSYM not supported.\n");
	    break;

        case (SHT_REL):		/* relocation info sections */
        case (SHT_RELA):	/* relocation with addend info section*/

	    /*
	     * We only keep the relocation sections related to an
	     * allocatable section. Relocation sections related to debug
	     * sections are then discarded.
	     */

	    pAllocScnHdr = pScnHdrTbl + pScnHdr->sh_info;
	    if ((pAllocScnHdr->sh_flags & SHF_ALLOC) &&
	        (pAllocScnHdr->sh_type == SHT_PROGBITS))
	        pIndexTables->pRelScnHdrIdxs [relocSections++] = scnHdrIdx;
	    break;

        case (SHT_STRTAB):		/* string table sections */
	    pIndexTables->pStrTabScnHdrIdxs [strTableSections++] = scnHdrIdx;
	    break;
        }

    return (OK);
    }

/******************************************************************************
*
* loadElfScnHdrRd - "Read in" the sections header
*
* This routine fills a section header structure with information from the
* object file in memory.
* Each section header in the section header table is processed in turn.
*
* RETURNS : OK or ERROR if a section header is not correct.
*/

LOCAL STATUS loadElfScnHdrRd
    (
    int		 fd,			/* file to read in */
    int 	 posScnHdrField,        /* positon of first section header */
    Elf32_Shdr * pScnHdrTbl,		/* section header table */
    int          sectionNumber,         /* number of sections */
    IDX_TBLS   * pIndexTables		/* pointer to section index tables */
    )
    {
    int		 scnHdrIdx;		/* loop counter */
    Elf32_Shdr * pScnHdr;		/* pointer to a section header */

    /* Initialization */

    loadElfScnHdrIdxDispatch (NULL, 0, NULL);

    /* Read all the sections header */

    ioctl (fd, FIOSEEK, posScnHdrField);

    if ((fioRead (fd, (char *) pScnHdrTbl, sectionNumber * sizeof(Elf32_Shdr))) 
		!= sectionNumber * sizeof(Elf32_Shdr))
 	    {
            errnoSet (S_loadElfLib_SHDR_READ);
	    return (ERROR);
	    }

    /* loop thru all the sections headers */

    for (scnHdrIdx = 0; scnHdrIdx < sectionNumber; scnHdrIdx++)
	{
	pScnHdr = pScnHdrTbl + scnHdrIdx;

        /* Check the section header */

	if (loadElfScnHdrCheck (pScnHdr, scnHdrIdx) != OK)
	    return (ERROR);

	/* Record the indexes of the headers of various section types */

	loadElfScnHdrIdxDispatch (pScnHdrTbl, scnHdrIdx, pIndexTables);
        }

    return (OK);
    }

/*******************************************************************************
*
* loadElfSegSizeGet - determine segment sizes from section headers
*
* This function fills in the size fields in the SEG_INFO structure when the
* module is relocatable.
*
* RETURNS : N/A
*/

LOCAL void loadElfSegSizeGet
    (
    char *       pScnStrTbl,        	/* ptr to section name string  table */
    UINT32 *	 pLoadScnHdrIdxs,	/* index of loadable section hdrs */
    Elf32_Shdr * pScnHdrTbl,		/* pointer to section header table */
    SEG_INFO *	 pSeg			/* section addresses and sizes */
    )
    {
    int			index;		/* loop counter */
    int                 nbytes;         /* bytes required for alignment */
    Elf32_Shdr *	pScnHdr;	/* pointer to a section header */
    int			textAlign;	/* Alignment for text segment */
    int			dataAlign;	/* Alignment for data segment */
    int			bssAlign;	/* Alignment for bss segment */
    SDA_SCN_TYPE        sdaScnVal;      /* type of SDA section */
    SDA_INFO *          pSda = NULL;    /* SDA section addresses and sizes */

    pSeg->sizeText = 0;
    pSeg->sizeData = 0;
    pSeg->sizeBss = 0;

#ifdef INCLUDE_SDA
    /* ELF modules for PowerPC may have additional specific sections */

    if (pSeg->pAdnlInfo != NULL)
        {
        pSda = (SDA_INFO *) pSeg->pAdnlInfo;
        pSda->sizeSdata = 0;
        pSda->sizeSbss = 0;
        pSda->sizeSdata2 = 0;
        pSda->sizeSbss2 = 0;
        }
#endif /* INCLUDE_SDA */

    /* Set minimum alignments for segments */			

    textAlign = _ALLOC_ALIGN_SIZE;	
    dataAlign = _ALLOC_ALIGN_SIZE;	
    bssAlign  = _ALLOC_ALIGN_SIZE;	

    /* loop thru all loadable sections */

    for (index = 0; pLoadScnHdrIdxs[index] != 0; index++)
	{
	pScnHdr = pScnHdrTbl + pLoadScnHdrIdxs [index];

        /* Determine if section is sdata/sbss, sdata2/sbss2 or anything else */
#ifdef INCLUDE_SDA
        if (pSda != NULL)
            sdaScnVal = loadElfSdaScnDetermine (pScnStrTbl, pScnHdr, NULL);
        else
#endif /* INCLUDE_SDA */
            sdaScnVal = NOT_SDA_SCN;

        /*
	 * To follow the three-segment model, all sections of same type are
	 * loaded following each others within the area allocated for the
	 * segment. But, sections must be loaded at addresses that fit with
	 * the alignment requested or, by default, with the target
	 * architecture's alignment requirement. So, the segment size is the
	 * total size of the sections integrated in this segment plus the
	 * number of bytes that create the required offset to align the next
	 * sections :
	 * 
	 *     +-------------+ <- segment base address (aligned thanks to the
	 *     |:::::::::::::|    memory manager). This is also the load 
	 *     |::::Text:::::|    address of the first section.
	 *     |:Section:1:::|
	 *     |:::::::::::::|
	 *     |:::::::::::::|
	 *     +-------------+ <- end of first section.
	 *     |/////////////| <- offset needed to reach next aligned address.
	 *     +-------------+ <- aligned load address of next section within
	 *     |:::::::::::::|    the segment.
	 *     |:::::::::::::|
	 *     |:::::::::::::|
	 *     |::::Text:::::|
	 *     |:Section:2:::|
	 *     |:::::::::::::|
	 *     |:::::::::::::|
	 *     |:::::::::::::|
	 *     |:::::::::::::|
	 *     +-------------+
	 *     |             |
	 *
	 * The principle here is to determine, for a given section type (text,
	 * data or bss), how much padding bytes should be added to the previous
	 * section in order to be sure that the current section will be
	 * correctly aligned. This means that the first section of each type is
	 * considered as "de facto" aligned. Note that this assumption is
	 * correct only if each segment is allocated separately (since
	 * tgtMemMalloc() returns an aligned address). If however only one
	 * memory area is allocated for the three segments (as do
	 * loadSegmentsAllocate()), an other round of alignment computation
	 * must be done between the three segments.
	 */

	switch (pScnHdr->sh_type)
	    {
	    case SHT_PROGBITS:	/* data and text sections */

		/* 
		 * Previously, if the SHF_ALLOC and SHF_WRITE flags were set,
		 * thesection was determined to be a data section. Hence text
		 * sections were expected to be read only. This logic causes
		 * problems for fully linked files such as the VxWorks image
		 * if the linker's -N option is used. This option forces the
		 * toolchain to disregard the ELF standard and generate text
		 * sections with the SHF_WRITE flag set (among other things).
		 * Let's be a bit more flexible and check the absence of the
		 * SHF_EXECINSTR flag to determine a data section...
		 */

		if ((pScnHdr->sh_flags & SHF_ALLOC) &&
		    (pScnHdr->sh_flags & SHF_WRITE) &&
		    !(pScnHdr->sh_flags & SHF_EXECINSTR))
		    {
                    /*
                     * This is data section. Note that SDA data sections are
                     * stored in dedicated areas in the target's memory. They
                     * are then handled separately.
                     *
                     * XXX PAD this is not enough to differentiate data
                     * sections from Global Offset Table sections.
                     */

                    switch (sdaScnVal)
                        {
                        case NOT_SDA_SCN:
                            nbytes = loadElfAlignGet (pScnHdr->sh_addralign,
                                                     (void *) pSeg->sizeData);
                            pSeg->sizeData += pScnHdr->sh_size + nbytes;

			    /* 
			     * Record segment alignment.  The alignment of 
			     * each segment should be the maximum of the 
			     * alignments required by all sections in the 
			     * segment, not just the alignment of the first 
			     * section.  
			     */

			    if (pScnHdr->sh_addralign > dataAlign)
				dataAlign = pScnHdr->sh_addralign;

                            break;

                        case SDA_SCN:
                            nbytes = loadElfAlignGet (pScnHdr->sh_addralign,
                                                     (void *)pScnHdr->sh_size);
                            pSda->sizeSdata = pScnHdr->sh_size + nbytes;
                            break;

                        case SDA2_SCN:
                            nbytes = loadElfAlignGet (pScnHdr->sh_addralign,
                                                     (void *)pScnHdr->sh_size);
                            pSda->sizeSdata2 = pScnHdr->sh_size + nbytes;
                            break;
                        }
                    }
		else if ((pScnHdr->sh_flags & SHF_ALLOC) &&
			 (pScnHdr->sh_flags & SHF_EXECINSTR))
		    {
                    /*
                     * This is a text section.
                     *
                     * XXX PAD this is not enough to differentiate text
                     * sections from Procedure Linkage Table sections on
                     * processors other than the PowerPC.
                     */

                    nbytes = loadElfAlignGet (pScnHdr->sh_addralign,
                                             (void *) pSeg->sizeText);
                    pSeg->sizeText += pScnHdr->sh_size + nbytes;

		    /* 
		     * Record segment alignment.  The alignment of each segment
		     * should be the maximum of the alignments required by all 
		     * sections in the segment, not just the alignment of the 
		     * first section.   
		     */

		    if (pScnHdr->sh_addralign > textAlign)
			textAlign = pScnHdr->sh_addralign;
        	    
		    }
		else if (pScnHdr->sh_flags & SHF_ALLOC)
		    {
                    if (sdaScnVal == SDA2_SCN)
                        {
                        /*
                         * This is unfortunate but .sdata2 sections may be not
                         * writable. How strange...
                         */

                        nbytes = loadElfAlignGet (pScnHdr->sh_addralign,
                                                 (void *)pScnHdr->sh_size);
                        pSda->sizeSdata2 = pScnHdr->sh_size + nbytes;
                        }
                    else
                        {
                        /*
                         * This is a read only data section. Since these
                         * sections hold constant data, their contents is
                         * considered as "text" by the loader (see
                         * loadElfScnRd()). So, the size of the read only data
                         * sections is added to the size of the text sections.
                         */

                        nbytes = loadElfAlignGet (pScnHdr->sh_addralign,
                                                 (void *) pSeg->sizeText);
                        pSeg->sizeText += pScnHdr->sh_size + nbytes;

			/* 
			 * Record segment alignment.  The alignment of each 
			 * segment should be the maximum of the alignments 
			 * required by all sections in the segment, not just 
			 * the alignment of the first section. 
			 */

			if (pScnHdr->sh_addralign > textAlign)
			    textAlign = pScnHdr->sh_addralign;
                        }
		    }

		break;

	    case SHT_NOBITS:	/* bss sections */

		if ((pScnHdr->sh_flags & SHF_ALLOC) &&
		    (pScnHdr->sh_flags & SHF_WRITE))
		    {
                    /*
                     * This is bss section. Note that SDA bss sections are
                     * stored in dedicated areas in the target's memory. They
                     * are then handled separately.
                     *
                     * XXX PAD this is not enough to differentiate bss
                     * sections from Procedure Linkage Table sections on
                     * PowerPC processors.
                     */

                    switch (sdaScnVal)
                        {
                        case NOT_SDA_SCN:
                            nbytes = loadElfAlignGet (pScnHdr->sh_addralign,
                                                     (void *) pSeg->sizeBss);
                            pSeg->sizeBss += pScnHdr->sh_size + nbytes;

			    /* 
			     * Record segment alignment.  The alignment of 
			     * each segment should be the maximum of the 
			     * alignments required by all sections in the 
			     * segment, not just the alignment of the first 
			     * section.  
			     */

			    if (pScnHdr->sh_addralign > bssAlign)
				bssAlign = pScnHdr->sh_addralign;

                            break;

                        case SDA_SCN:
                            nbytes = loadElfAlignGet (pScnHdr->sh_addralign,
                                                     (void *)pScnHdr->sh_size);
                            pSda->sizeSbss = pScnHdr->sh_size + nbytes;
                            break;

                        case SDA2_SCN:
                            nbytes = loadElfAlignGet (pScnHdr->sh_addralign,
                                                     (void *)pScnHdr->sh_size);
                            pSda->sizeSbss2 = pScnHdr->sh_size + nbytes;
                            break;
                        }
		    }
		else
		    printErr ("Section SHT_NOBITS ignored (wrong flags).\n");

		break;
	    }
	}

    /* 
     * SPR #21836: pSeg->flagsText, pSeg->flagsData, pSeg->flagsBss are used
     * to save the alignment requirement for each segment. These fields of pSeg
     * are only used on output, thus a temporary use is allowed.
     */
    
    pSeg->flagsText = textAlign;
    pSeg->flagsData = dataAlign;
    pSeg->flagsBss  = bssAlign;

    /* 
     * Alloc alignment may be less than what is required by
     * these particular sections.  Adjust sizes to allow for
     * alignment after loadSegmentsAllocate().
     */

    if ((textAlign > _ALLOC_ALIGN_SIZE) && (pSeg->sizeText > 0))
    	pSeg->sizeText += textAlign - _ALLOC_ALIGN_SIZE;
    if ((dataAlign > _ALLOC_ALIGN_SIZE) && (pSeg->sizeData > 0))
       	pSeg->sizeData += dataAlign - _ALLOC_ALIGN_SIZE;
    if ((bssAlign > _ALLOC_ALIGN_SIZE) && (pSeg->sizeBss > 0))
   	pSeg->sizeBss += bssAlign - _ALLOC_ALIGN_SIZE;

    /*
     * If only one memory area is to be allocated for the three segments,
     * takes care of the alignment between each three segments.
     */

    if ((pSeg->addrText == LD_NO_ADDRESS) &&
	(pSeg->addrData == LD_NO_ADDRESS) &&
	(pSeg->addrBss  == LD_NO_ADDRESS))
	{
	if (pSeg->sizeData > 0)
            pSeg->sizeText += loadElfAlignGet (dataAlign,
                                              (void *) pSeg->sizeText);
	if (pSeg->sizeBss > 0)
	    {
	    if (pSeg->sizeData > 0)
		pSeg->sizeData += loadElfAlignGet (bssAlign,
				(void *) (pSeg->sizeText + pSeg->sizeData));
	    else
		pSeg->sizeText += loadElfAlignGet (bssAlign,
				(void *) pSeg->sizeText);
	    }
	}
    }

/*******************************************************************************
*
* loadElfScnRd - "read" sections into the target memory
*
* This routine actually copies the sections contents into the target memory.
* All the sections of the same type are coalesced so that we end up with a 
* three segment model.
*
* RETURNS : OK or ERROR if a section couldn't be read in.
*/

LOCAL STATUS loadElfScnRd
    (
    int		 fd,			/* file to read in */
    char *       pScnStrTbl,         /* ptr to section name string  table */
    UINT32 *	 pLoadScnHdrIdxs,	/* index of loadable section hdrs */
    Elf32_Shdr * pScnHdrTbl,		/* points to table of section headers */
    SCN_ADRS_TBL sectionAdrsTbl,	/* points to table of section addr */
    SEG_INFO *	 pSeg			/* segments information */
    )
    {
    int		 index;			/* loop counter */
    Elf32_Shdr * pScnHdr;		/* pointer to a section header */
    SCN_ADRS *	 pScnAddr;		/* pointer to address of section */
    void *	 pTgtLoadAddr;		/* target address to load data at */
    INT32	 scnSize;		/* section size */
    void *	 pTextCurAddr;		/* current addr where text is loaded */
    void *	 pDataCurAddr;		/* current addr where data are loaded */
    void *	 pBssCurAddr;		/* current addr where bss is "loaded" */
    int          nbytes;                /* bytes required for alignment */
    void *       pTgtSdaLoadAddr;       /* target address to load SDA scn at */
    SDA_SCN_TYPE sdaScnVal;             /* type of SDA section */
    SDA_INFO *   pSda = NULL;           /* SDA section addresses and sizes */

    pTextCurAddr = pSeg->addrText;
    pDataCurAddr = pSeg->addrData;
    pBssCurAddr = pSeg->addrBss;

#ifdef INCLUDE_SDA
    /* ELF modules for PowerPC may have additional specific sections */
    if (pSeg->pAdnlInfo != NULL)
        pSda = (SDA_INFO *) pSeg->pAdnlInfo;
#endif 	/* INCLUDE_SDA */

    /* Loop thru all loadable sections */

    for (index = 0; pLoadScnHdrIdxs [index] != 0; index++)
	{
	pScnHdr = pScnHdrTbl + pLoadScnHdrIdxs [index];
	pScnAddr = sectionAdrsTbl + pLoadScnHdrIdxs [index];

        pTgtLoadAddr = NULL;
	scnSize = pScnHdr->sh_size;
        pTgtSdaLoadAddr = NULL;

        /* Determine if section is sdata/sbss, sdata2/sbss2 or anything else */

#ifdef	INCLUDE_SDA
        if (pSda != NULL)
            sdaScnVal = loadElfSdaScnDetermine (pScnStrTbl, pScnHdr, NULL);
        else
#endif	/* INCLUDE_SDA */
            sdaScnVal = NOT_SDA_SCN;

	/*
	 * Only sections of type SHT_PROGBITS and SHT_NOBITS with flag
	 * SHF_ALLOC are considered here.
	 *
	 * About the section alignment, see explanations and diagram in
	 * loadElfSegSizeGet().
	 */

	switch (pScnHdr->sh_type)
	    {
	    case (SHT_PROGBITS):	/* data and text sections */
		
		if ((pScnHdr->sh_flags & SHF_ALLOC) &&
		    (pScnHdr->sh_flags & SHF_EXECINSTR))
		    {
		    /* This is text section */

		    pTgtLoadAddr = pTextCurAddr;
                    nbytes = loadElfAlignGet(pScnHdr->sh_addralign,
							pTgtLoadAddr) ;
                    pTgtLoadAddr = (UINT8 *)pTgtLoadAddr + nbytes;
                    pTextCurAddr = (UINT8 *)pTgtLoadAddr + scnSize;

		    }
		else if ((pScnHdr->sh_flags & SHF_ALLOC) &&
			 (pScnHdr->sh_flags & SHF_WRITE))
		    {
		    /* This is data section */

                    switch (sdaScnVal)
                        {
                        case NOT_SDA_SCN:
		    	    pTgtLoadAddr = pDataCurAddr;
                            nbytes = loadElfAlignGet (pScnHdr->sh_addralign,
                                                     pTgtLoadAddr);
                            pTgtLoadAddr = (UINT8 *)pTgtLoadAddr + nbytes;
                            pDataCurAddr = (UINT8 *)pTgtLoadAddr + scnSize;
                            break;

                        case SDA_SCN:
                            pTgtSdaLoadAddr = pSda->pAddrSdata;
                            nbytes = loadElfAlignGet (pScnHdr->sh_addralign,
                                                     pTgtSdaLoadAddr);
                            pTgtSdaLoadAddr = (UINT8 *)pTgtSdaLoadAddr 
			    				+ nbytes ;
                            break;

                        case SDA2_SCN:
                            pTgtSdaLoadAddr = pSda->pAddrSdata2;
                            nbytes = loadElfAlignGet (pScnHdr->sh_addralign,
                                                     pTgtSdaLoadAddr);
                            pTgtSdaLoadAddr = (UINT8 *)pTgtSdaLoadAddr 
			    				+ nbytes ;
                            break;
                        }
                    }
	        else if (pScnHdr->sh_flags & SHF_ALLOC)
		    {
                    if (sdaScnVal == SDA2_SCN)
                        {
                        /*
                         * This is unfortunate but .sdata2 sections may be not
                         * writable. How strange...
                         */

                        pTgtSdaLoadAddr = pSda->pAddrSdata2;
                        nbytes = loadElfAlignGet (pScnHdr->sh_addralign,
                                                 pTgtSdaLoadAddr);
                        pTgtSdaLoadAddr = (UINT8 *)pTgtSdaLoadAddr + nbytes;
                        }
		    else
			{
		    	/*
		     	* This is a read only data section. Since these sections
		     	* hold constant data, their contents is considered as
		     	* "text" by the loader. So, the size of the read only 
			* data sections is added to the size of the text 
			* sections.
		     	*/

		    	pTgtLoadAddr = pTextCurAddr;
                    	nbytes = loadElfAlignGet(pScnHdr->sh_addralign,
		    				pTgtLoadAddr) ;
                    	pTgtLoadAddr = (UINT8 *)pTgtLoadAddr + nbytes;
                    	pTextCurAddr = (UINT8 *)pTgtLoadAddr + scnSize;

		    	}
		    }
		else
		    printErr("Section SHT_PROGBITS ignored (flags: %d).\n",
				  pScnHdr->sh_flags);
	    break;

	    case (SHT_NOBITS):		/* bss sections */

		if ((pScnHdr->sh_flags & SHF_ALLOC) &&
		    (pScnHdr->sh_flags & SHF_WRITE))
		    {
		    /* 
		     * Bss sections. Such sections must not be downloaded since
		     * they don't actually exist in the object module. However
		     * for the relocation purpose, we need to know where they
		     * are located in the target memory.
		     */
                    switch (sdaScnVal)
                        {
                        case NOT_SDA_SCN:
		    	    pTgtLoadAddr = pBssCurAddr;
                            nbytes = loadElfAlignGet (pScnHdr->sh_addralign,
                                                     pTgtLoadAddr);
                            pTgtLoadAddr = (UINT8 *)pTgtLoadAddr + nbytes;
                            pBssCurAddr = (UINT8 *)pTgtLoadAddr + scnSize;
                            break;

                        case SDA_SCN:
                            pTgtSdaLoadAddr = pSda->pAddrSbss;
                            nbytes = loadElfAlignGet (pScnHdr->sh_addralign,
                                                     pTgtSdaLoadAddr);
                            pTgtSdaLoadAddr = (UINT8 *)pTgtSdaLoadAddr 
							+ nbytes ;
                            break;

                        case SDA2_SCN:
                            pTgtSdaLoadAddr = pSda->pAddrSbss2;
                            nbytes = loadElfAlignGet (pScnHdr->sh_addralign,
                                                     pTgtSdaLoadAddr);
                            pTgtSdaLoadAddr = (UINT8 *)pTgtSdaLoadAddr 
							+ nbytes ;
                            break;
                        }
		    }
		else
		    printErr ("Section SHT_NOBITS ignored (flags: %d).\n",
				   pScnHdr->sh_flags);
		break;

	    default:			/* we should not get here */
		printErr("Section of type %d ignored.\n",pScnHdr->sh_type);
		break;
	    }

        /*
	 * Advance to position in file and copy the section into the target
	 * memory.
         */

	if (pScnHdr->sh_type != SHT_NOBITS)
	    {
	    ioctl(fd, FIOSEEK, pScnHdr->sh_offset);

            if (sdaScnVal == NOT_SDA_SCN)
                {
	    	if (fioRead (fd, pTgtLoadAddr, scnSize) != scnSize)
		    {
            	    errnoSet (S_loadElfLib_SCN_READ);
		    return (ERROR);
		    }
                }
            else
                {
	    	if (fioRead (fd, pTgtSdaLoadAddr, scnSize) != scnSize)
		    {
            	    errnoSet (S_loadElfLib_SCN_READ);
		    return (ERROR);
		    }
                }
	    }

	/* record the load address of each section */

        if (sdaScnVal == NOT_SDA_SCN)
            *pScnAddr = pTgtLoadAddr;
        else
            *pScnAddr = pTgtSdaLoadAddr;
	}

    return (OK);
    }

/*******************************************************************************
*
* loadElfSymEntryRd - "read in" a symbol entry
*
* This routine fills a symbol structure with information from the object
* module in memory.
*
* RETURNS : the next symbol entry position in the module file.
*
*/

LOCAL int loadElfSymEntryRd
    (
    int		fd,		/* file to read in */
    int		symEntry,	/* position of symbol info in file */
    Elf32_Sym *	pSymbol		/* points to structure to fill with info */
    )
    {
    int nbytes;
   
    nbytes = sizeof(Elf32_Sym);

    ioctl(fd, FIOSEEK, symEntry);

    if (fioRead (fd, (char *) pSymbol, nbytes) != nbytes)
	return (ERROR);

    return (symEntry + nbytes);
    }

/*******************************************************************************
*
* loadElfSymTabRd - read and process the symbol table sections
* 
* This routine searches and reads in the module's current symbol table section.
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS loadElfSymTabRd
    (
    int 	 fd,			/* file to read in */
    int 	 nextSym,		/* position of symbol info in file */
    UINT32	 nSyms,			/* number of symbols in section */
    Elf32_Sym *	 pSymsArray		/* array of symbol entries */
    )
    {
    UINT32	 symIndex;		/* loop counter */
    Elf32_Sym *  pSym;			/* pointer to symbol entry */

    /* Loop thru all the symbol in the current symbol table. */

    for (symIndex = 0; symIndex < nSyms; symIndex++)
	{
	pSym = pSymsArray + symIndex;

	/* read in next entry */

	if ((nextSym = loadElfSymEntryRd (fd, nextSym, pSym)) == ERROR)
	    return (ERROR);
	}

    return (OK);
    }

/*******************************************************************************
*
* loadElfSymTablesHandle - handle all symbol tables in module
* 
* This routine allocates
*
* RETURNS: the number of symbol tables, or -1 if something went wrong.
*/

LOCAL int loadElfSymTablesHandle
    (
    UINT32 *	   pSymTabScnHdrIdxs,	/* index of sym table section hdrs */
    Elf32_Shdr *   pScnHdrTbl,		/* pointer to section header table */
    int 	   fd,			/* file to read in */
    SYMTBL_REFS *  pSymTblRefs,		/* array for module's symbols */
    SYMINFO_REFS * pSymsAdrsRefs	/* adrs of ptr to tbl of sym addr tbl */
    )
    {
    UINT32	 index;			/* loop counter */
    int		 symtabNb;		/* number of symbol tables in module */
    Elf32_Shdr * pSymTabHdr;		/* points to a symbol table header */
    UINT32	 nSyms;			/* number of syms in current symtab */

    /*
     * In the future, ELF objects file may have more than one symbol table.
     * Then get this number.
     */

    for (symtabNb = 0; pSymTabScnHdrIdxs [symtabNb] != 0; symtabNb++);

    /*
     * Allocate first an array of pointers to symbol tables...
     *
     *                +---+---+---+---+...
     * symTblRefs ->  |   |   |   |   |
     *                +-|-+-|-+-|-+-|-+...
     *                  V   V   V   V
     * Sym tbl 1: +-------+
     *            |symbol |
     *            |entry 1|
     *            +-------+
     *            |symbol |
     *            |entry 2|
     *            +-------+
     *            :       :
     *            :       :
     *
     * Then allocate an array of pointers to symbol address tables...
     */

    if ((*pSymTblRefs = (SYMTBL_REFS) calloc (symtabNb ,
					      sizeof (Elf32_Sym *))) == NULL)
	return (-1);

    if ((*pSymsAdrsRefs =
		(SYMINFO_REFS) calloc (symtabNb ,
					 sizeof (SYM_INFO_TBL))) == NULL)
	return (-1);

    /*
     * ...then we allocate here the required room for the symbols in all
     * symbol tables and read in the symbols.
     */

    for (index = 0; pSymTabScnHdrIdxs [index] != 0; index++)
	{
	pSymTabHdr = pScnHdrTbl + pSymTabScnHdrIdxs [index];

        if (pSymTabHdr->sh_entsize != sizeof (Elf32_Sym))
	    {
            errnoSet (S_loadElfLib_SYMTAB);
	    return (-1);
	    }

	nSyms = pSymTabHdr->sh_size / pSymTabHdr->sh_entsize;

	if ((*pSymTblRefs [index] = (Elf32_Sym *) malloc (pSymTabHdr->sh_size))
	    == NULL)
	    return (-1);

	/* Read in current symbol table contents */

	if (loadElfSymTabRd (fd, pSymTabHdr->sh_offset, nSyms,
			     *pSymTblRefs [index]) == ERROR)
	    return (-1);

	/*
	 * Create the external undefined symbol address table (for relocations)
	 * The size of this table is determined from the number of symbols
	 * in the current symbol table. This allow us to store the 
	 * external undefined symbols indexed by their number.
	 */

	if ((*pSymsAdrsRefs [index] =
		(SYM_INFO_TBL) malloc (nSyms * sizeof (SYM_INFO))) == NULL)
	    return (-1);
	}

    return (symtabNb);
    }

/*******************************************************************************
*
* loadElfSymTypeGet - determine the symbol's type
*
* This routine determines the type of a symbol, that is whether a given
* symbol belongs to a text section, a data section, a bss section, is absolute
* or undefined.
*
* RETURNS: the symbol's type.
*/

LOCAL SYM_TYPE loadElfSymTypeGet
    (
    Elf32_Sym *	 pSymbol,	/* pointer to symbol entry */
    Elf32_Shdr * pScnHdrTbl,	/* section header table */
    char *       pScnStrTbl         /* ptr to section name string table */
    )
    {
#ifdef INCLUDE_SDA
    SDA_SCN_TYPE sdaScnVal;     	/* type of SDA section */
#endif /* INCLUDE_SDA */
    SYM_TYPE	 symType = SYM_UNDF;	/* internal value for symbol type */
    Elf32_Shdr * pScnHdr;		/* pointer to sect header of */
					/* the section related to the symbol */

    pScnHdr = pScnHdrTbl + pSymbol->st_shndx;

    /*
     * For absolute symbols, don't consider the section type and flags.
     * Otherwise, the type and flags of the section to which belongs the
     * symbol are tested to determined the type of the symbol.
     */

    if (pSymbol->st_shndx == SHN_ABS)
	symType = SYM_ABS;
    else
	{
#ifdef INCLUDE_SDA
	if (sdaIsRequired)
	    {
	    /* Determine if the symbol is in the SDA or SDA2 areas.  */
            
   	    sdaScnVal = loadElfSdaScnDetermine (pScnStrTbl, pScnHdr, NULL);

            switch (sdaScnVal)
            	{
            	case SDA_SCN:
                    symType |= SYM_SDA;
                    break;
            	case SDA2_SCN:
                    symType |= SYM_SDA2;
                    break;
            	}
	    }
#endif /* INCLUDE_SDA */

	switch (pScnHdr->sh_type)
	    {
	    case (SHT_PROGBITS):
		if ((pScnHdr->sh_flags & SHF_ALLOC) &&
		    (pScnHdr->sh_flags & SHF_EXECINSTR))
		    {
		    /* This is text section */

		    symType |= SYM_TEXT;
		    }
		else if ((pScnHdr->sh_flags & SHF_ALLOC) &&
			 (pScnHdr->sh_flags & SHF_WRITE))
		    {
		    /* This is data section */

		    symType |= SYM_DATA;
		    }
		else if (pScnHdr->sh_flags & SHF_ALLOC)
#ifdef INCLUDE_SDA
                    if (sdaIsRequired && (sdaScnVal == SDA2_SCN))
                        symType |= SYM_DATA;
		    else
#endif /* INCLUDE_SDA */
		    	/*
		     	* This is a read only data section.
			* XXX PAD - these sections hold constant data, but
			* since we don't have an appropriate symbol type their
			* contents is considered as "data" by the loader so
			* that tools, such as the shell, behave properly.
		     	*/
		    	symType |= SYM_DATA;
		break;

	    case (SHT_NOBITS):
		if (pScnHdr->sh_flags & SHF_ALLOC)
		    {
		    /* This is a bss section */

		    symType |= SYM_BSS;
		    }
		break;

	    default:
		/*
		 * Well, we don't handle this type of symbol.
		 * Give them type SYM_UNDF.
		 */

		symType = SYM_UNDF;
		break;
	    }
	}

    return (symType);
    }

/*******************************************************************************
*
* loadElfSymIsVisible - check whether a symbol must be added to the symtable
*
* This routine determines whether or not a symbol must be added to the Target's
* symbol table, based on the symbol's information and the load flags.
*
* RETURNS: TRUE if the symbol must be added, FALSE otherwise.
*/

LOCAL BOOL loadElfSymIsVisible
    (
    UINT32	symAssoc,	/* type of entity associated to the symbol */
    UINT32	symBinding,	/* symbol visibility and behavior */
    int		loadFlag	/* control of loader's behavior */
    )
    {
    /* Only consider symbols related to functions and variables (objects) */

    /* 
     * XXX The STT_ARM_TFUNC type is used by the gnu compiler to mark 
     * Thumb functions.  It is not part of the ARM ABI or EABI.
     *
     * We also now recognize STT_ARM_16BIT - this is the thumb equivalent
     * of STT_OBJECT. (SPR 73992)
     */

    if (! ((symAssoc == STT_OBJECT)  ||
           (symAssoc == STT_FUNC)
#if (CPU_FAMILY == ARM)
	   || (symAssoc == STT_ARM_TFUNC) ||
	   (symAssoc == STT_ARM_16BIT)
#endif
	   ))
        return (FALSE);

    /* Depending on the load flags we add globals and locals or only globals */

    if (! (((loadFlag & LOAD_LOCAL_SYMBOLS) && (symBinding == STB_LOCAL)) ||
	   ((loadFlag & LOAD_GLOBAL_SYMBOLS) &&
	   ((symBinding == STB_GLOBAL) || (symBinding == STB_WEAK)))))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* loadElfSymTabProcess - process an object module symbol table
*
* This is passed a pointer to a ELF symbol table and processes
* each of the external symbols defined therein.  This processing performs
* two functions:

*
*  1) Defined symbols are entered in the target system symbol table as
*     specified by the "loadFlag" argument:
*	LOAD_ALL_SYMBOLS    = all defined symbols (LOCAL and GLOBAL) are added,
*	LOAD_GLOBAL_SYMBOLS = only external (GLOBAL) symbols are added,
*	LOAD_NO_SYMBOLS	    = no symbols are added;
*
*  2) Any undefined externals are looked up in the target system symbol table
*     to determine their values and, if found, entered in an array. This
*     array is indexed by the symbol number (position in the symbol table).
*     Note that all symbol values, not just undefined externals, are entered
*     into this array.  The values are used to perform relocations.
*
*     Note that "common" symbols have type undefined external - the value
*     field of the symbol will be non-zero for type common, indicating
*     the size of the object.
*
* If an undefined external cannot be found in the target symbol table,
* a warning message is printed, the noUndefSym field of the module is set
* to FALSE, and the name of the symbol is added to the list in the module.
* Note that this is not considered as an error since this allows all undefined
* externals to be looked up.
*
* RETURNS: OK or ERROR
*/

LOCAL STATUS loadElfSymTabProcess
    (
    MODULE_ID	 moduleId,		/* module id */
    int		 loadFlag,		/* control of loader's behavior */
    Elf32_Sym *	 pSymsArray,		/* pointer to symbols array */
    SCN_ADRS_TBL sectionAdrsTbl,	/* table of sections addresses */
    SYM_INFO_TBL symsAdrsTbl,		/* table to fill with syms address */
    char * 	 pStringTable,		/* pointer on current string table */
    SYMTAB_ID	 symTbl,		/* symbol table to feed */
    UINT32	 symNumber,		/* number of syms in current symtab */
    Elf32_Shdr * pScnHdrTbl,		/* section header table */
    char *       pScnStrTbl,         	/* ptr to section name string  table */
    SEG_INFO *   pSeg                   /* info about loaded segments */
    )
    {
    char * 	 name;			/* symbol name (plus EOS) */
    Elf32_Sym *	 pSymbol;		/* current symbol being processed */
    SYM_TYPE	 symType;		/* internal value for symbol type */
    UINT32	 symIndex;		/* symbol index */
    SYM_ADRS	 adrs		= NULL;	/* table resident symbol address */
    SYM_ADRS	 bias		= NULL;	/* section relative address */
    SCN_ADRS *	 pScnAddr	= NULL;	/* addr where section is loaded */
    STATUS	 status		= OK;	/* ERROR if something is wrong */
    UINT32	 symBinding;		/* symbol visibility and behavior */
    UINT32	 symAssoc;		/* type of entity associated to sym */
    UINT32	 commonSize;		/* size of common symbol when aligned */
    UINT32	 commonAlignment;	/* alignment of common symbol */

    /* Loop thru all symbol table entries in object file. */

    for (symIndex = 0; symIndex < symNumber; symIndex++)
	{
	pSymbol = pSymsArray + symIndex;

	/* Determine the type of processing based on the symbol type */

	symBinding = ELF32_ST_BIND (pSymbol->st_info);
	symAssoc = ELF32_ST_TYPE (pSymbol->st_info);

	/*
	 * Get the symbol name. In ELF, this name is held by the string table
	 * associated to the symbol table in which the symbol entry is
	 * registred.
	 */

	name = pStringTable + pSymbol->st_name;

	/* we throw away "gcc2_compiled." and "___gnu_compiled_c*" symbols */
	/* and __gnu_compiled_c for mips */
	/* XXX this sould be put in a resource file somewhere */

	if ((!strcmp (name, "gcc2_compiled.")) ||
	    (!strncmp (name, "___gnu_compiled_c",17)) ||
	    (!strncmp (name, "__gnu_compiled_c",16)))
	    continue;

	if ((pSymbol->st_shndx != SHN_UNDEF) &&
	    (pSymbol->st_shndx != SHN_COMMON))
            {
            /* Symbol is neither an undefined external, nor a common symbol */

            /*
	     * Bias is not needed either when symbol is absolute.
             */
	    if (pSymbol->st_shndx == SHN_ABS)
		bias = 0;
	    else
		{
	    	/* 
	     	* Compute the bias. In ELF is it the base address of the
	     	* section when loaded.
	     	*/

	    	pScnAddr = sectionAdrsTbl + pSymbol->st_shndx;
	    	bias = *pScnAddr;
		}

	    /* Determine the symbol type */

	    symType = loadElfSymTypeGet (pSymbol, pScnHdrTbl, pScnStrTbl);

	    /*
	     * The STT_ARM_TFUNC type is used by the gnu compiler to mark
	     * Thumb functions.  It is not part of the ARM ABI or EABI.
	     *
	     * The SYM_THUMB flag only needs to be set for 
	     * function (STT_ARM_TFUNC) symbols, not data symbols, since
	     * it's currently only used when doing a relocation, to switch
	     * the low bit of the address of the function to 1 to signal
	     * that the processor should run the code in THUMB mode. 
	     * 
	     * Therefore it does not need to be set for COMMON symbols or 
	     * STT_ARM_16BIT symbols.
	     *
	     * XXX check may need changing if full interworking is
	     * eventually implemented. 
	     * XXX PAD - also we'll try to move this code in an arch 
	     * specific file in the future.  
	     */

#if (CPU_FAMILY == ARM)
	    /* check if it's a Thumb function */

	    if (symAssoc == STT_ARM_TFUNC)
	        symType |= SYM_THUMB;

#endif

            /* Determine if symbol should be put into symbol table. */

            if (loadElfSymIsVisible (symAssoc, symBinding, loadFlag))
                {
                if ((symBinding == STB_GLOBAL) || (symBinding == STB_WEAK))
		    symType |= SYM_GLOBAL;

                /* Add symbol to target's symbol table. */

		if (symSAdd (symTbl, name, (char *)(pSymbol->st_value +
			    (INT32) bias), symType, moduleId->group) != OK)
		    {
		    printErr ( "Can't add '%s' to symbol table\n", name);
		    status = ERROR;
		    }
                }

		/*
		 * For ELF, we add all symbol addresses into an symbol address
		 * table, not only those symbols added in the target symbol
		 * table. This is required by the relocation process.
		 * XXX PAD I think it is better to use an additionnal array
		 * rather than than modifying the symbol's st_value field since
		 * we may need the original field contents in the future.
		 */

		symsAdrsTbl [symIndex].pAddr = (SYM_ADRS)(pSymbol->st_value +
						    (INT32) bias);
                symsAdrsTbl [symIndex].type = symType;
	    }
	else
	    {
            /*
             * Undefined external symbol or "common" symbol.
             * A common symbol type is denoted by a special section index.
             */

            if (pSymbol->st_shndx == SHN_COMMON)
		{
		/* Follow common symbol management policy. */

                commonSize = pSymbol->st_size;

		commonAlignment = pSymbol->st_value;

		if (loadCommonManage (commonSize, commonAlignment, 
				      name, symTbl, &adrs, &symType, loadFlag,
                                      pSeg,  moduleId->group) != OK)
		    status = ERROR;
		}
	    else
		{
		/* We get rid of the symbol if it is an ELF debug symbol */

		if ((symBinding == STB_LOCAL) && (symAssoc == STT_NOTYPE))
		    continue;

		/* Look up undefined external symbol in symbol table */

		if (symFindByNameAndType (symTbl, name, (char **) &adrs,
					       &symType, SYM_GLOBAL, 
					       SYM_GLOBAL) != OK)
  		    {
		    /* symbol not found in symbol table */
		    adrs = NULL;

	      	    printErr ("Undefined symbol: %s (binding %d type %d)\n",
						  name, symBinding, symAssoc);

		    /* 
		     * SPR # 30588 - loader should return NULL when there
		     * are unresolved symbols. 
		     */

		    status = ERROR;
		    }
		}

            /* add symbol address to externals table */

            symsAdrsTbl [symIndex].pAddr = adrs;
            symsAdrsTbl [symIndex].type = symType;
	    }
	}

    return (status);
    }
	
/*******************************************************************************
*
* loadElfSymTableBuild - build the target's symbol table
*
* This routine updates the target's symbol table. It processes in turn
* all the symbol table sections found in the module.
* 
* RETURNS: OK or ERROR
*/

LOCAL STATUS loadElfSymTableBuild
    (
    MODULE_ID	 moduleId,		/* module id */
    int		 loadFlag,		/* control of loader's behavior */
    SYMTBL_REFS  symTblRefs,		/* pointer to symbols array */
    SCN_ADRS_TBL sectionAdrsTbl,	/* table of sections addresses */
    SYMINFO_REFS symsAdrsRefs,		/* ptr to tbl to fill with syms addr */
    IDX_TBLS *	 pIndexTables,		/* pointer to section index tables */
    SYMTAB_ID	 symTbl,		/* symbol table to feed */
    int 	 fd,			/* file to read in */
    Elf32_Shdr * pScnHdrTbl,		/* section header table */
    char *       pScnStrTbl,         	/* ptr to section name string  table */
    SEG_INFO *   pSeg                   /* info about loaded segments */
    )
    {
    UINT32	 index;			/* loop counter */
    Elf32_Shdr * pSymTabHdr;		/* points to a symbol table header */
    UINT32	 nSyms;			/* number of syms in current symtab */
    Elf32_Shdr * pStrTabHdr;		/* pointer to a string table header */
    char * 	 pStringTable;		/* pointer on current string table */
    STATUS       status = OK;           /* ERROR if undefined symbols found */

    /*
     * The module symbol tables are now analysed so that every defined symbol
     * is added to the target symbol table.  Undefined external symbols are
     * looked up in the target symbol table and their addresses are stored
     * in the external symbol array (for relocation purpose).
     * Note that the target symbol table is built when a core file is
     * processed.
     */

    for (index = 0; pIndexTables->pSymTabScnHdrIdxs [index] != 0; index++)
	{
	pSymTabHdr = pScnHdrTbl + pIndexTables->pSymTabScnHdrIdxs [index];

	nSyms = pSymTabHdr->sh_size / pSymTabHdr->sh_entsize;

 	/* get current string table */

	pStrTabHdr = pScnHdrTbl + pSymTabHdr->sh_link;
    
	if ((pStringTable = (char *) malloc(pStrTabHdr->sh_size)) == NULL)
	    return ERROR;

	if (ioctl(fd, FIOSEEK, pStrTabHdr->sh_offset) == ERROR ||
            fioRead (fd, pStringTable, pStrTabHdr->sh_size) !=
                           pStrTabHdr->sh_size)
            {
            errnoSet (S_loadElfLib_READ_SECTIONS);
    	    loadElfBufferFree ((void **) &pStringTable);
            return (ERROR);
	    }

	/* build the target symbol table */

	/* 
	 * SPR # 30588 - loader should return NULL when there are unresolved 
	 * symbols.
	 */

	if (loadElfSymTabProcess (moduleId, loadFlag, symTblRefs [index],
				  sectionAdrsTbl, symsAdrsRefs [index],
				  pStringTable, symTbl, nSyms,
				  pScnHdrTbl, pScnStrTbl, pSeg) != OK)
	    {
	    /* 
	     * status is used to record whether _any_ of the calls to
	     * loadElfSymTabProcess has returned ERROR.  Therefore we
	     * set it only in the error case rather than always setting
	     * it to the return value of loadElfSymTabProcess.
	     */

	    status = ERROR;

	    /* 
	     * If the only error is that some symbols were not found, 
	     * continue building the symbol table.  This allows all 
	     * unresolved symbols to be reported in one pass.  
	     * If any other error was encountered, abort the process.
	     */

	    if (errno != S_symLib_SYMBOL_NOT_FOUND)
	        {	
		loadElfBufferFree ((void **) &pStringTable);
		return (ERROR);
		}
	    }
	}

    loadElfBufferFree ((void **) &pStringTable);

    return status;
    }

/******************************************************************************
*
* loadElfRelSegRtnGet - return the appropriate relocation routine
*
* This routine returns the appropriate relocation routine for the
* specified CPU.
*
* Warning: this routine is replaced by the relocation unit's initialization
*          routines. It is kept here until all the relocation units are
*	   standardized. It will be removed afterward.
*
* RETURNS: the address of a relocation routine, or NULL if none is found.
*
* NOMANUAL
*/

LOCAL FUNCPTR loadElfRelSegRtnGet (void)
    {
    #if (CPU_FAMILY == PPC)
	return ((FUNCPTR) &elfPpcSegReloc);
    #elif (CPU_FAMILY == SPARC || CPU_FAMILY==SIMSPARCSOLARIS)
	return ((FUNCPTR) &elfSparcSegReloc);
    #elif (CPU_FAMILY == COLDFIRE)
	return ((FUNCPTR) &elfM68kSegReloc);
    #elif (CPU_FAMILY == SH)
       	return ((FUNCPTR) &elfShSegReloc);
    #else
   	printErr ("No relocation routine for this CPU\n");
    #endif /* CPU_FAMILY == PPC */

    return (NULL);
    }

/******************************************************************************
*
* loadElfSegReloc - relocate the text and data segments
*
* This routine relocates the text and data segments stored in the target 
* memory, using the appropriate relocation routine for the target
* architecture.
*
* Even if the symbol table had missing symbols, we continue with relocation
* of those symbols that were found. Although the resulting module might not
* be usable this allows for an easier incremental testing and this also let
* the user know of all the missing symbols in the system.
*
* Note: the relocation is for adapting the code (text and data) of the
*       loaded module to its installation address and make it executable.
*       The places in the segments where references to text or data symbols
*       are made are modified so that they can access the routine or the
*       variable this symbol represents. This modification is done accordingly
*       with the specified relocation command (see the relocation unit for the
*       the architecture). BSS is uninitialized data, so it is never relocated.
*
* RETURNS: OK, or ERROR
*/

LOCAL STATUS loadElfSegReloc
    (
    int 	 fd,			/* file to read in */
    int		 loadFlag,		/* control of loader's behavior */
    MODULE_ID	 moduleId,		/* module id */
    Elf32_Ehdr * pHdr,                  /* pointer to module header */
    IDX_TBLS   * pIndexTables,		/* pointer to section index tables */
    Elf32_Shdr * pScnHdrTbl,		/* pointer to section headers table */
    SCN_ADRS_TBL sectionAdrsTbl,	/* table of sections addresses */
    SYMTBL_REFS  symTblRefs,		/* pointer to symbols array */
    SYMINFO_REFS symsAdrsRefs,		/* ptr to tbl to fill with syms addr */
    SYMTAB_ID	 symTbl,		/* symbol table to feed */
    SEG_INFO *   pSeg                  	/* info about loaded segments */
    )
    {
    Elf32_Shdr * pSymTabHdr;		/* points to a symbol table header */
    Elf32_Shdr * pStrTabHdr;		/* pointer to a string table header */
    Elf32_Shdr * pRelHdr;		/* points to a reloc section header */
    UINT32	 index;			/* loop counter */
    UINT32	 arrayIdx;		/* loop counter */

    /*
     * Get the appropriate relocation routine for the architecture.
     * See also the warning not in loadElfRelSegRtnGet().
     */

    if (pElfSegRelRtn == NULL)
	if ((pElfSegRelRtn = loadElfRelSegRtnGet ()) == NULL)
	    return ERROR;

    /* Loop thru the relocation section headers */

    for (index = 0; pIndexTables->pRelScnHdrIdxs [index] != 0; index++)
	{
	pRelHdr = pScnHdrTbl + pIndexTables->pRelScnHdrIdxs [index];

	/*
	 * Since there is only one symbol table allowed in ELF object
	 * modules for now, lets just do a loop to find the index of the
	 * required arrays among the external symbol arrays and the symbol
	 * arrays. In the future, it would be better to index these arrays
	 * on the section header indeces rather than on the order of the
	 * symbol tables.
	 */

	for (arrayIdx = 0;
	     pIndexTables->pSymTabScnHdrIdxs [arrayIdx] != pRelHdr->sh_link;
 	      arrayIdx++);

	/* get current string table */

	pSymTabHdr = pScnHdrTbl + pRelHdr->sh_link;
	pStrTabHdr = pScnHdrTbl + pSymTabHdr->sh_link;

	if ((* pElfSegRelRtn) (fd, moduleId, loadFlag,
			       pRelHdr->sh_offset,
			       pScnHdrTbl, pRelHdr,
			       sectionAdrsTbl + pRelHdr->sh_info,
			       symsAdrsRefs [arrayIdx],
			       symTblRefs [arrayIdx], symTbl, pSeg) != OK)
	    return ERROR;
	}

    return OK;
    }

/******************************************************************************
*
* loadElfTablesAlloc - allocate memory for the tables used by the ELF loader
*
* RETURNS: OK or ERROR if the target does not have enough memory available.
*/

LOCAL STATUS loadElfTablesAlloc
    (
    Elf32_Ehdr * pHdr,		/* pointer to module header */
    Elf32_Phdr ** ppProgHdrTbl,	/* where to store the adrs of prog hdrs table */
    Elf32_Shdr ** ppScnHdrTbl,	/* where to store the adrs of sect hdrs table */
    IDX_TBLS   * pIndexTables	/* pointer to section index tables */
    )
    {
    /*
     * Allocate memory for the program and section header tables and also for
     * section header index tables. The amount of slots allocated for the 
     * index tables is equal to the total number of sections in the module,
     * this save us to loop thru the section header table to know how much
     * sections of each kind actually exist in the module.
     * Note that the index table contents are initialized to zero, which is
     * interpreted as a stop flag when the tables are walked.
     */

    if ((*ppScnHdrTbl =
	    (Elf32_Shdr *) malloc (pHdr->e_shnum * pHdr->e_shentsize)) == NULL)
	return (ERROR);

    if ((*ppProgHdrTbl =
	    (Elf32_Phdr *) malloc (pHdr->e_phnum * pHdr->e_phentsize)) == NULL)
	return (ERROR);

    if ((pIndexTables->pLoadScnHdrIdxs =
		(UINT32 *) calloc (pHdr->e_shnum, sizeof (UINT32))) == NULL)
	return (ERROR);

    if ((pIndexTables->pSymTabScnHdrIdxs =
		(UINT32 *) calloc (pHdr->e_shnum, sizeof (UINT32))) == NULL)
	return (ERROR);

    if ((pIndexTables->pRelScnHdrIdxs =
		(UINT32 *)calloc (pHdr->e_shnum, sizeof (UINT32))) == NULL)
	return (ERROR);

    if ((pIndexTables->pStrTabScnHdrIdxs =
		(UINT32 *) calloc (pHdr->e_shnum, sizeof (UINT32))) == NULL)
	return (ERROR);

    return (OK);
    }

/******************************************************************************
*
* loadElfRelocMod - store relocatable module's segments 
*
* This routine stores any relocatable module's segments.
*
* RETURNS: OK or ERROR if the segment contents can't be store in target memory.
*/

LOCAL STATUS loadElfRelocMod
    (
    SEG_INFO *     pSeg,                /* info about loaded segments */
    int		   fd, 			/* file to read in  */
    char *         pScnStrTbl,      	/* ptr to section string  table */
    IDX_TBLS *     pIndexTables,        /* pointer to section index tables */
    Elf32_Ehdr *   pHdr,                /* pointer to module header */
    Elf32_Shdr *   pScnHdrTbl,          /* pointer to section headers table */
    SCN_ADRS_TBL * pSectionAdrsTbl      /* table of section addr when loaded */
    )
    {
   /* allocate memory for the section adress table */

    if ((*pSectionAdrsTbl = (SCN_ADRS_TBL) malloc (pHdr->e_shnum *
                                                   sizeof (SCN_ADRS))) == NULL)
            return (ERROR);

    /*
     * Read in the section contents to assemble the segment: section's contents
     * are coalesced so that we end up with a three-segment model in memory:
     * text, data, bss
     */

    if (loadElfScnRd (fd, pScnStrTbl, pIndexTables->pLoadScnHdrIdxs,
                      pScnHdrTbl, *pSectionAdrsTbl, pSeg) != OK)
        return (ERROR);

    return (OK);
    }

/******************************************************************************
*
* loadElfSegStore - store the module's segments in target memory
*
* This routine stores the module's segments in target memory.
* linked or core file).
*
* RETURNS: OK or ERROR if the segment contents can't be store in target memory.
*/

LOCAL STATUS loadElfSegStore
    (
    SEG_INFO *	   pSeg,		/* info about loaded segments */
    int		   loadFlag,		/* control of loader's behavior */
    int 	   fd,			/* file to read in */
    char *         pScnStrTbl,      	/* ptr to section string  table */
    IDX_TBLS *	   pIndexTables,	/* pointer to section index tables */
    Elf32_Ehdr *   pHdr,		/* pointer to module header */
    Elf32_Shdr *   pScnHdrTbl,		/* pointer to section headers table */
    Elf32_Phdr *   pProgHdrTbl,		/* pointer to program headers table */
    SCN_ADRS_TBL * pSectionAdrsTbl	/* table of section addr when loaded */
    )
    {

    if (loadElfRelocMod (pSeg, fd, pScnStrTbl, pIndexTables, pHdr, 
				pScnHdrTbl, pSectionAdrsTbl) != OK)
        return (ERROR);
    else
        return (OK);
    }

#ifdef INCLUDE_SDA
/******************************************************************************
*
* loadElfSdaScnDetermine - determine the type of SDA section
*
* This routine determines to which SDA area belongs a section. It also returns
* a pointer to the section's name if the parameter pointer is not null.
*
* RETURNS : NOT_SDA_SCN, SDA_SCN or SDA2_SCN
*/

LOCAL SDA_SCN_TYPE loadElfSdaScnDetermine
    (
    char *	 pScnStrTbl,	/* ptr to section name string  table */
    Elf32_Shdr * pScnHdr,      	/* pointer to current section header */
    char *       sectionName 	/* where to return the section name */
    )

    {
    char * 	scnName;	/* section name (plus EOS) */

    scnName = pScnStrTbl + pScnHdr->sh_name; 

    if (sectionName != NULL)
        sectionName = scnName;

    if ((strcmp (scnName, ".sdata") == 0) ||
    	(strcmp (scnName, ".sbss") == 0))
    	return (SDA_SCN);

    if ((strcmp (scnName, ".sdata2") == 0) ||
        (strcmp (scnName, ".sbss2") == 0))
        return (SDA2_SCN);

    return (NOT_SDA_SCN);
    }

/******************************************************************************
*
* loadElfSdaCreate - Create new memory partitions for the SDA areas
*
* This routine allows for handling the SDA and SDA2 areas by creating two new
* memory partitions.
*
* RETURNS: OK, or ERROR if partitions can't be created.
*/

LOCAL STATUS loadElfSdaCreate
    (
    void
    )
    {
    /* Create new memory partition for SDA and SDA2 areas */

    if ((sdaMemPartId = memPartCreate ((char *) SDA_BASE, SDA_SIZE)) == NULL)
        {
        printErr ("Can't create a memory partition for the SDA area.\n");
        return (ERROR);
        }

    if ((sda2MemPartId = memPartCreate ((char *) SDA2_BASE, SDA2_SIZE))== NULL)
        {
        printErr ("Can't create a memory partition for the SDA2 area.\n");
        return (ERROR);
        }

    sdaBaseAddr = (void *) SDA_BASE;
    sda2BaseAddr = (void *) SDA2_BASE;

    sdaAreaSize = (int ) SDA_SIZE;
    sda2AreaSize = (int ) SDA2_SIZE;

    return (OK);
    }

/******************************************************************************
*
* loadElfSdaAllocate - allocate memory in SDA and SDA2 memory partitions
*
* This routine allocates memory from the SDA and SDA2 memory partitions for,
* respectively, the module's sdata and sbss sections, and the module's sdata2
* and sbss2 sections.
*
* RETURNS: OK, or ERROR if memory can't be allocated.
*/

LOCAL STATUS loadElfSdaAllocate
    (
    SDA_INFO *   pSda           /* SDA section addresses and sizes */
    )
    {
    void *      pBlock = NULL;  /* start address of allocated mem in SDA area */
    int         nBytes;         /* total size of SDA/SDA2 sections */

    nBytes = pSda->sizeSdata + pSda->sizeSbss;
    if ((nBytes) && ((pBlock = memPartAlloc (pSda->sdaMemPartId,
                                                   nBytes)) == NULL))
        {
        errnoSet (S_loadElfLib_SDA_MALLOC);
        return (ERROR);
        }

    if (pSda->sizeSdata != 0)
        {
        pSda->pAddrSdata = pBlock;
        pSda->flagsSdata = SEG_FREE_MEMORY;
        }
    if (pSda->sizeSbss != 0)
        {
        pSda->pAddrSbss = (void *)((char *)pBlock + pSda->sizeSdata);
        if (pSda->sizeSdata == 0)
            pSda->flagsSbss = SEG_FREE_MEMORY;
        }

    nBytes = pSda->sizeSdata2 + pSda->sizeSbss2;
    if ((nBytes) && ((pBlock = memPartAlloc (pSda->sda2MemPartId,
                                                   nBytes)) == NULL))
        {
        errnoSet (S_loadElfLib_SDA_MALLOC);
        return (ERROR);
        }

    if (pSda->sizeSdata2 != 0)
        {
        pSda->pAddrSdata2 = pBlock;
        pSda->flagsSdata2 = SEG_FREE_MEMORY;
        }
    if (pSda->sizeSbss2 != 0)
        {
        pSda->pAddrSbss2 = (void *)((char *)pBlock + pSda->sizeSdata2);
        if (pSda->sizeSdata2 == 0)
            pSda->flagsSbss2 = SEG_FREE_MEMORY;
        }

    return (OK);
    }
#endif /* INCLUDE_SDA */

/*******************************************************************************
*
* loadElfScnNaneStrTblRd - read the section name string table
*
* This routine reads the section name string table.
*
* RETURNS: N/A
*/

LOCAL char * loadElfScnStrTblRd
    (
    FAST int   	   fd,      	/* fd from which to read module */
    Elf32_Shdr *   pScnHdrTbl,	/* pointer to section headers table */
    Elf32_Ehdr *   pHdr		/* pointer to module header */
    )
    {
    char * pScnStrTbl;		/* pointer to the section strng  table */
 
    if ((pScnStrTbl = malloc(pScnHdrTbl[pHdr->e_shstrndx].sh_size))
			== NULL)
	return (NULL);

    ioctl(fd, FIOSEEK, pScnHdrTbl[pHdr->e_shstrndx].sh_offset);

    if (fioRead(fd, pScnStrTbl, pScnHdrTbl[pHdr->e_shstrndx].sh_size)
				!= pScnHdrTbl[pHdr->e_shstrndx].sh_size)
	return (NULL);

    return (pScnStrTbl);
    }


/*******************************************************************************
*
* loadElfBufferFree - free a buffer previously allocated
*
* This routine frees the memory used by a buffer. The buffer pointer is set to
* NULL afterward.
*
* RETURNS: N/A
*/

LOCAL void loadElfBufferFree
    (
    void ** ppBuf
    )
    {
    if (*ppBuf != NULL)
        {
        free (*ppBuf);
        *ppBuf = NULL;
        }

    return;
    }

#ifdef	INCLUDE_SDA
/******************************************************************************
*
* segElfFindByType - find a segment whith its TYPE
*
* This routine finds a segment information in a module whith its
* type.
*
* RETURNS: FALSE if it is the good segment
*	   TRUE if it isn't.
*/

LOCAL BOOL segElfFindByType
    (
    SEGMENT_ID     segmentId,   /* The segment to examine */
    MODULE_ID      moduleId,    /* The associated module */
    int            type     	/* type of the segment to find */
    )
    {
    if (segmentId->type == type)
 	return (FALSE);
    return (TRUE);
    }

/******************************************************************************
*
* moduleElfSegAdd - add a segment to a module for ELF format
*
* This routine adds segment information to an object module descriptor.  The
* specific information recorded depends on the format of the object module.
*
* The <type> parameter is one of the following:
* .iP SEGMENT_TEXT 25
* Segment holds text (code) or literal (constant data).
* .iP SEGMENT_DATA
* Segment holds initialized data
* .iP SEGMENT_BSS
* Segment holds non initialized data
*
* RETURNS: OK, or ERROR.
*/

LOCAL STATUS moduleElfSegAdd
    (
    MODULE_ID   moduleId,       /* module to add segment to*/
    int         type,           /* segment type */
    void *      location,       /* segment address */
    int         length,         /* segment length */
    int         flags,          /* segment flags */
    PART_ID     memPartId       /* ID of mem. partition holding the segment */
    )
    {
    SEGMENT_ID segmentId;
	
    if (moduleSegAdd(moduleId, type, location, length, flags) == ERROR)
	return (ERROR);
 
    /*
     * We can't have two segments with the same type in a same module.
     * So, we use this type to find the segment we have added
     */
    if ((segmentId = moduleSegEach(moduleId, (FUNCPTR) segElfFindByType, 
			type)) == NULL)
	return (ERROR);
   
    segmentId->memPartId = memPartId; 

    return (OK);
    }
#endif	/* INCLUDE_SDA */

/******************************************************************************
*
*
* loadElfAlignGet - determine required alignment for an address or a size
*
* This routine determines, from a proposed address or size, how many bytes
* should be added to get a target-compliant aligned address or size.
*
* RETURNS:
* the number of bytes to be added to the address or to the size to obtain the
* correct alignment.
*
* SEE ALSO
* .I Tornado API User's Guide: Target Server
*/

LOCAL UINT32 loadElfAlignGet
    (
    UINT32      alignment,      /* value of alignment */
    void *      pAddrOrSize     /* address or size to check */
    )
    {
    UINT32      nbytes;         /* # bytes for alignment */

    /* sections may have a null alignment if their size is null */

    if (alignment > 0)
        nbytes = (UINT32) pAddrOrSize % alignment;
    else
        nbytes = 0;

    /* compute fitting if not on correct boundary */

    if (nbytes != 0)
        nbytes = alignment - nbytes;

    return (nbytes);
    }

/******************************************************************************
*
* loadElfFmtManage - process object module
*
* This routine is the underlying routine to loadModuleAt().  This interface
* allows for specification of the symbol table used to resolve undefined
* external references and to which to add new symbols.
*
* One kind of files can be handled : - relocatable files
*
* For relocatable files, addresses of segments may or may not be specified.
* Memory is allocated on the target if required (nothing specified).
* 
* RETURNS: OK, or ERROR if can't process file or not enough memory or illegal
*          file format
*
* NOMANUAL
*/

LOCAL MODULE_ID loadElfFmtManage
    (
    FAST int 	fd,        	/* fd from which to read module */
    int		loadFlag,	/* control of loader's behavior */
    void **	ppText,		/* load text segment at addr pointed to by */
				/* this ptr, return load addr via this ptr */
    void **	ppData,		/* load data segment at addr pointed to by */
				/* this ptr, return load addr via this ptr */
    void **	ppBss,		/* load bss segment at addr pointed to by */
				/* this ptr, return load addr via this ptr */
    SYMTAB_ID	symTbl		/* symbol table to use */
    )
    {
    char         fileName[255];		/* name of object file */
    Elf32_Ehdr	 hdr;			/* module header */
    Elf32_Phdr * pProgHdrTbl	= NULL;	/* program headers table */
    Elf32_Shdr * pScnHdrTbl	= NULL;	/* section headers table */
    SEG_INFO     seg;            	/* segment info struct */
    MODULE_ID    moduleId;		/* molule identifier */
    SYMTBL_REFS	 symTblRefs	= NULL;	/* table of pointers to symbol tables */
    IDX_TBLS	 indexTables;		/* section index tables */
    SCN_ADRS_TBL sectionAdrsTbl = NULL;	/* table of section addr when loaded */
    UINT32	 index;			/* loop counter */
    SYMINFO_REFS symsAdrsRefs	= NULL;	/* table of ptrs to sym adrs tables */
    int		 symtabNb       = 0;	/* number of symbol tables in module */
    BOOL         modSegAddSucceeded;	/* TRUE if moduleSegAdd() succeeds */
#ifdef INCLUDE_SDA
    SDA_INFO *   pSda           = NULL; /* SDA section addresses and sizes */
#endif /* INCLUDE_SDA */
    char *       pScnStrTbl	= NULL;	/* ptr to section name string  table */
    STATUS       status         = OK;   /* ERROR indicates unresolved symbols */
   
    /* initialization */

    memset ((void *)&seg, 0, sizeof (seg));
    memset ((void *)&indexTables, 0, sizeof (indexTables));

    /* Set up the module */

    /*
     * XXX - JN - The return values of all these calls to ioctl should 
     * be checked.
     */

    ioctl (fd, FIOGETNAME, (int) fileName);

    moduleId = loadModuleGet (fileName, MODULE_ELF, &loadFlag);

    if (moduleId == NULL)
        return (NULL);

    /* Read object module header */

    if (loadElfMdlHdrRd (fd, &hdr) != OK)
        {
        errnoSet (S_loadElfLib_HDR_READ);
	goto error;
        }

    /*
     * First, we check if the module is really elf, and if it is intended
     * for the appropriate target architecture.
     * Note that we may consider the beginning of the file as an ELF header
     * since loadElfModuleIsOk() only checks the e_ident field.
     */

    if (!loadElfModuleIsOk (&hdr))
	{
        errnoSet (S_loadElfLib_HDR_READ);
	goto error;
	}

    /* Allocate memory for the various tables */

    if (loadElfTablesAlloc(&hdr, &pProgHdrTbl, &pScnHdrTbl, 
					&indexTables) != OK)
	goto error;

    /*
     * Initialization of additional info related to the Small Data Area on
     * PowerPC architecture.
     */

#ifdef INCLUDE_SDA
    if (sdaIsRequired)
        {
        if (pSda = (SDA_INFO *) calloc (1, sizeof (SDA_INFO)) == NULL)
	    goto error;
        pSda->pAddrSdata = LD_NO_ADDRESS;
        pSda->pAddrSbss = LD_NO_ADDRESS;
        pSda->pAddrSdata2 = LD_NO_ADDRESS;
        pSda->pAddrSbss2 = LD_NO_ADDRESS;

        /*
         * Record the SDAs memory partition IDs and base addresses. These value
         * are NULL when the core file is being processed but set afterward.
         */

        pSda->sdaMemPartId = sdaMemPartId;
        pSda->sda2MemPartId = sda2MemPartId;
        pSda->sdaBaseAddr = sdaBaseAddr;
        pSda->sda2BaseAddr = sda2BaseAddr;
        pSda->sdaAreaSize = sdaAreaSize;
        pSda->sda2AreaSize = sda2AreaSize;

        seg.pAdnlInfo = (void *) pSda;
        }
#endif /* INCLUDE_SDA */

    /* Read program header table if any */

    if ((hdr.e_phnum != 0) &&
	(loadElfProgHdrTblRd (fd, hdr.e_phoff, pProgHdrTbl,
			      hdr.e_phnum) != OK))
	goto error;

    /* Read in section headers and record various section header indexes */

    if ((hdr.e_shnum != 0) &&
	(loadElfScnHdrRd (fd, hdr.e_shoff, pScnHdrTbl,  
				hdr.e_shnum, &indexTables) != OK))
	goto error;

    pScnStrTbl = loadElfScnStrTblRd(fd, pScnHdrTbl, &hdr); 

    if (pScnStrTbl == NULL)
 	goto error;

    /* Replace a null address by a dedicated flag (LD_NO_ADDRESS) */

    seg.addrText = (ppText == NULL) ? LD_NO_ADDRESS : *ppText;
    seg.addrData = (ppData == NULL) ? LD_NO_ADDRESS : *ppData;
    seg.addrBss  = (ppBss  == NULL) ? LD_NO_ADDRESS : *ppBss;

    /* Determine segment sizes */

    /* 
     * XXX seg.flagsXxx mustn't be used between coffSegSizes() and 
     * loadSegmentsAllocate().
     */

    loadElfSegSizeGet (pScnStrTbl,indexTables.pLoadScnHdrIdxs, pScnHdrTbl, 
						&seg);

    /* Handles all the symbol table sections in the module */

    if ((symtabNb = loadElfSymTablesHandle (indexTables.pSymTabScnHdrIdxs,
					    pScnHdrTbl, fd, &symTblRefs,
					    &symsAdrsRefs)) == -1)
	goto error;
    /*
     * Take in account the SDA areas (this must be done after the call to
     * loadElfSymTablesHandle() when required (PowerPC).
     *
     * Allocate the segments accordingly with user's requirements.
     */

    /* 
     * SPR #21836: loadSegmentsAllocate() allocate memory aligned on 
     * the max value of sections alignement saved in seg.flagsText,
     * seg.flagsData, seg.flagsBss.
     */

    if (loadSegmentsAllocate((SEG_INFO *) &seg) != OK)
        {
        printErr ("could not allocate text and data segments\n");
	goto error;
	}

#ifdef INCLUDE_SDA
    /*
     * When required (PowerPC), allocate memory from the SDA memory partitions.
     */

    if (sdaIsRequired && (loadElfSdaAllocate (pSda) != OK))
        goto error;
#endif /* INCLUDE_SDA */

    /* We are now about to store the segment's contents in the target memory */

    if (loadElfSegStore (&seg, loadFlag, fd, pScnStrTbl, &indexTables, &hdr,
			 pScnHdrTbl, pProgHdrTbl, &sectionAdrsTbl) != OK)
	goto error;

    /*
     * Build / update the target's symbol table with symbols found
     * in module's symbol tables.
     */

    /* 
     * SPR # 30588 - loader should return NULL when there are unresolved 
     * symbols.  However, we finish the load before returning.
     */

    if (loadElfSymTableBuild (moduleId, loadFlag, symTblRefs, 
			      sectionAdrsTbl, symsAdrsRefs, 
			      &indexTables, symTbl, fd, pScnHdrTbl, 
			      pScnStrTbl, &seg) != OK) 
	{
	if (errno != S_symLib_SYMBOL_NOT_FOUND)
	    goto error;
	else 
	    status = ERROR;
	}

    /* Relocate text and data segments (if not already linked) */

    if ((loadElfSegReloc (fd, loadFlag, moduleId, &hdr, &indexTables,
			  pScnHdrTbl, sectionAdrsTbl, symTblRefs,
			  symsAdrsRefs, symTbl, &seg) != OK))
	goto error;

    /* clean up dynamically allocated temporary buffers */

    if (symTblRefs != NULL)
	{
	for (index = 0; index < symtabNb; index++)
	    loadElfBufferFree ((void **) &(symTblRefs [index]));
	loadElfBufferFree((void **)&symTblRefs);
	}

    if (symsAdrsRefs != NULL)
	{
	for (index = 0; index < symtabNb; index++)
	    loadElfBufferFree ((void **) &(symsAdrsRefs [index]));
	loadElfBufferFree ((void **) &symsAdrsRefs);
	}

    loadElfBufferFree ((void **) &pProgHdrTbl);
    loadElfBufferFree ((void **) &pScnHdrTbl);
    loadElfBufferFree ((void **) &(indexTables.pLoadScnHdrIdxs));
    loadElfBufferFree ((void **) &(indexTables.pSymTabScnHdrIdxs));
    loadElfBufferFree ((void **) &(indexTables.pRelScnHdrIdxs));
    loadElfBufferFree ((void **) &(indexTables.pStrTabScnHdrIdxs));
    loadElfBufferFree ((void **) &sectionAdrsTbl);
    loadElfBufferFree ((void **) &pScnStrTbl);

    /* return load addresses, where called for */

    if (ppText != NULL)
        *ppText = seg.addrText;

    if (ppData != NULL)
        *ppData = seg.addrData;

    if (ppBss != NULL)
        *ppBss = seg.addrBss;

    /* write protect the text if text segment protection is turned on */

#if (CPU_FAMILY != SIMSPARCSOLARIS) 
    if (((seg.flagsText & SEG_WRITE_PROTECTION)) &&
	(VM_STATE_SET (NULL, seg.addrText, seg.sizeProtectedText,
	 		VM_STATE_MASK_WRITABLE,
	 		VM_STATE_WRITABLE_NOT) == ERROR))
	goto error;
#endif /*(CPU_FAMILY != SIMSPARCSOLARIS) */

    /* Initialize bss sections */

    if (seg.sizeBss != 0)	/* zero out bss */
	memset (seg.addrBss, 0, seg.sizeBss);

#ifdef INCLUDE_SDA    
    if (sdaIsRequired)
	{
	if (pSda->sizeSbss != 0)	/* zero out sbss */
	    memset (pSda->pAddrSbss, 0, pSda->sizeSbss);

	if (pSda->sizeSbss2 != 0)	/* zero out sbss2 */
	    memset (pSda->pAddrSbss2, 0, pSda->sizeSbss2);
	}
#endif  /* INCLUDE_SDA */

#ifdef INCLUDE_SDA
    /* Just for moduleShow */
    if (sdaIsRequired)
        {
        seg.sizeData += pSda->sizeSdata + pSda->sizeSdata2;
        seg.sizeBss += pSda->sizeSbss + pSda->sizeSbss2;

        if ((seg.addrData == LD_NO_ADDRESS) &&
            ((seg.addrData = pSda->pAddrSdata) == LD_NO_ADDRESS))
            seg.addrData = pSda->pAddrSdata2;

        if ((seg.addrBss == LD_NO_ADDRESS) &&
            ((seg.addrBss = pSda->pAddrSbss) == LD_NO_ADDRESS))
            seg.addrBss = pSda->pAddrSbss2;
        }
#endif	/* INCLUDE_SDA */

    /* If the module was empty or not managable, just give up now */

    if ((seg.addrText == LD_NO_ADDRESS) &&
        (seg.addrData == LD_NO_ADDRESS) &&
        (seg.addrBss == LD_NO_ADDRESS))
        {
        printErr ("Object module not loaded\n");
        goto error;
        }

    /* flush stub to memory, flush any i-cache inconsistancies */

    if (seg.sizeText != 0)
#ifdef  _CACHE_SUPPORT
	cacheTextUpdate (seg.addrText, seg.sizeText);
#endif  /* _CACHE_SUPPORT */

    /*
     * Add the segments to the module information.  This has to happen after
     * the relocation gets done. If the relocation happens first, the
     * checksums won't be correct.  Note that the module information is
     * updated even if the relocation went wrong so that we can more simply
     * reclaim any target memory with moduleDelete().
     */

    modSegAddSucceeded = (moduleSegAdd (moduleId, SEGMENT_TEXT, 
			seg.addrText, seg.sizeText, seg.flagsText) == OK);
    modSegAddSucceeded |= (moduleSegAdd (moduleId, SEGMENT_DATA, 
			seg.addrData, seg.sizeData, seg.flagsData) == OK);
    modSegAddSucceeded |= (moduleSegAdd (moduleId, SEGMENT_BSS, 
			seg.addrBss, seg.sizeBss, seg.flagsBss) == OK);

#ifdef INCLUDE_SDA
    if (seg.pAdnlInfo != NULL)
        {
        modSegAddSucceeded |= (moduleElfSegAdd (moduleId, SEGMENT_SDATA,
                        ((SDA_INFO *)seg.pAdnlInfo)->pAddrSdata,
                        ((SDA_INFO *)seg.pAdnlInfo)->sizeSdata,
                        ((SDA_INFO *)seg.pAdnlInfo)->flagsSdata,
			((SDA_INFO *)seg.pAdnlInfo)->sdaMemPartId) == OK);
        modSegAddSucceeded |= (moduleElfSegAdd (moduleId, SEGMENT_SBSS,
                        ((SDA_INFO *)seg.pAdnlInfo)->pAddrSbss,
                        ((SDA_INFO *)seg.pAdnlInfo)->sizeSbss,
                        ((SDA_INFO *)seg.pAdnlInfo)->flagsSbss,
			((SDA_INFO *)seg.pAdnlInfo)->sdaMemPartId) == OK);
        modSegAddSucceeded |= (moduleElfSegAdd (moduleId, SEGMENT_SDATA2,
                        ((SDA_INFO *)seg.pAdnlInfo)->pAddrSdata2,
                        ((SDA_INFO *)seg.pAdnlInfo)->sizeSdata2,
                        ((SDA_INFO *)seg.pAdnlInfo)->flagsSdata2,
			((SDA_INFO *)seg.pAdnlInfo)->sda2MemPartId) == OK);
        modSegAddSucceeded |= (moduleElfSegAdd (moduleId, SEGMENT_SBSS2,
                        ((SDA_INFO *)seg.pAdnlInfo)->pAddrSbss2,
                        ((SDA_INFO *)seg.pAdnlInfo)->sizeSbss2,
                        ((SDA_INFO *)seg.pAdnlInfo)->flagsSbss2,
			((SDA_INFO *)seg.pAdnlInfo)->sda2MemPartId) == OK);
        }
#endif /* INCLUDE_SDA */

 
    if (!modSegAddSucceeded)          	/* Something failed! */
   	{
	printErr ("Object module load failed\n");
	goto error;
    	}

#ifdef 	INCLUDE_SDA
    loadElfBufferFree((void **) &seg.pAdnlInfo);
#endif 	/* INCLUDE_SDA */

    if (status == OK)
        return (moduleId);
    else                                /* Unresolved symbols were found */
        return NULL;

    /*
     * error:
     * free target memory cache nodes, clean up dynamically allocated
     * temporary buffers and return ERROR.
     */

error:
#ifdef  _CACHE_SUPPORT
    /* flush stub to memory, flush any i-cache inconsistancies */

    if (seg.sizeText != 0)
	cacheTextUpdate (seg.addrText, seg.sizeText);
#endif  /* _CACHE_SUPPORT */

    loadElfBufferFree ((void **) &pProgHdrTbl);
    loadElfBufferFree ((void **) &pScnHdrTbl);
    loadElfBufferFree ((void **) &(indexTables.pLoadScnHdrIdxs));
    loadElfBufferFree ((void **) &(indexTables.pSymTabScnHdrIdxs));
    loadElfBufferFree ((void **) &(indexTables.pRelScnHdrIdxs));
    loadElfBufferFree ((void **) &(indexTables.pStrTabScnHdrIdxs));
    loadElfBufferFree ((void **) &sectionAdrsTbl);
#ifdef 	INCLUDE_SDA
    loadElfBufferFree ((void **) &seg.pAdnlInfo);
#endif 	/* INCLUDE_SDA */
    loadElfBufferFree ((void **) &pScnStrTbl);

    if (symTblRefs != NULL)
        {
        for (index = 0; index < symtabNb; index++)
            loadElfBufferFree ((void **) &(symTblRefs [index]));
        loadElfBufferFree((void **)&symTblRefs);
        }

    if (symsAdrsRefs != NULL)
        {
        for (index = 0; index < symtabNb; index++)
            loadElfBufferFree ((void **) &(symsAdrsRefs [index]));
        loadElfBufferFree ((void **) &symsAdrsRefs);
        }

    moduleDelete (moduleId);

    return (NULL);
    }

#endif /* (CPU_FAMILY == (MIPS || PPC || SIMSPARCSOLARIS || 
	                  SH || I80X86 || ARM)) */

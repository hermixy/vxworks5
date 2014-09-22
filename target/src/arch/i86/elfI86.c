/* elfI86.c - ELF/Ix86 relocation unit */

/* Copyright 2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01b,21nov01,pad  Minor changes: removed unused parameters, test the returned
		 value of elfRelocRelEntryRd(), etc.
01a,12sep01,pad  Backported from TAE 3.1 to T2.2 with necessary adaptations
		 (base version: elfI86.c@@/main/tor3_x/15).
*/

/*
DESCRIPTION
This file holds the relocation unit for ELF/Intel I86 combination.

Each relocation entry is applied on the sections loaded in the target memory
image, in order to eventually get an executable program linked at the required
address.

The relocation computations handled by this relocation unit are:

	R_386_NONE		: none
	R_386_32		: word32,	S + A
	R_386_PC32		: word32,	S + A - P

With:
   A - the addend used to compute the value of the relocatable field.
   S - the value (address) of the symbol whose index resides in the
       relocation entry. Note that this function uses the value stored
       in the external symbol value array instead of the symbol's
       st_value field.
   P - the place (section offset or address) of the storage unit being
       relocated (computed using r_offset) prior to the relocation.

Per the ELF ABI for the i86 architecture the only relocation entry type
accepted by this relocation unit is Elf32_Rel.

*/

/* Defines */

/* Includes */

#include "vxWorks.h"
#include "string.h"
#include "stdio.h"
#include "elf.h"
#include "elftypes.h"
#include "errnoLib.h"
#include "moduleLib.h"
#include "loadLib.h"
#include "loadElfLib.h"
#include "private/vmLibP.h"
#include "symbol.h"
#include "symLib.h"
#include "arch/i86/elfI86.h"

/* Defines */

#define MEM_READ_32(pRelocAdrs, offset)	  offset = *((UINT32 *)pRelocAdrs);
#define MEM_WRITE_32(pRelocAdrs, value)	  *((UINT32 *) pRelocAdrs) = value;
#define MEM_WRITE_16(pRelocAdrs, value)	  *((UINT16 *)pRelocAdrs) = value;

/* Globals */

/* Externals */

IMPORT int elfRelocRelEntryRd (int fd, int posRelocEntry, Elf32_Rel * pReloc);

/* Locals */

LOCAL STATUS relocationSelect
    (
    void *		pRelocAdrs,
    Elf32_Rel  *	pRelocCmd,
    void *		pSymAdrs
    );

LOCAL STATUS elfI86Addr32Reloc
    (
    void *		pRelocAdrs,
    void *		pSymAdrs
    );

LOCAL STATUS elfI86Pc32Reloc
    (
    void *		pRelocAdrs,
    void *		pSymAdrs
    );

/*******************************************************************************
*
* elfI86SegReloc - perform relocation for the Intel i86/Pentium family
*
* This routine reads the specified relocation command section and performs
* all the relocations specified therein. Only relocation command from sections
* with section type SHT_REL are considered here.
*
* Absolute symbol addresses are looked up in the <symInfoTbl> table.
*
* RETURNS: OK or ERROR
*/

LOCAL STATUS elfI86SegReloc
    (
    int		 fd,		/* object file to read in */
    MODULE_ID	 moduleId,	/* ID of object module being relocated */
    int		 loadFlag,	/* load options (not used here) */
    int		 posCurRelocCmd,/* position of current relocation command */
    Elf32_Shdr * pScnHdrTbl,	/* ptr to section header table (unused here) */
    Elf32_Shdr * pRelHdr,	/* Pointer to relocation section header */
    SCN_ADRS *	 pScnAddr,	/* Section address once loaded */
    SYM_INFO_TBL symInfoTbl,	/* Array of absolute symbol values and types */
    Elf32_Sym *	 pSymsArray,	/* pointer to symbols array (unused here) */
    SYMTAB_ID 	 symTbl,	/* current symbol table (unused here) */
    SEG_INFO *	 pSeg		/* section addresses and sizes */
    )
    {
    Elf32_Rel    relocCmd;	/* relocation structure */
    UINT32	 relocNum;	/* number of reloc entries in section */
    UINT32	 relocIdx;	/* index of the reloc entry being processed */
    void *	 pRelocAdrs;	/* relocation address */
    void *	 pSymAdrs;	/* address of symbol involved in relocation */
    BOOL	 relocError;	/* TRUE when a relocation failed */

    /* Some sanity checking */

    if (pRelHdr->sh_type != SHT_REL)
	{
	printErr ("Relocation sections of type %d are not supported.\n",
		  pRelHdr->sh_type);
        errnoSet (S_loadElfLib_RELA_SECTION);
	return ERROR;
	}

    if (pRelHdr->sh_entsize != sizeof (Elf32_Rel) )
	{
	printErr ("Wrong relocation entry size.\n");
        errnoSet (S_loadElfLib_RELOC);
	return ERROR;
	}

    /* Get the number of relocation entries */

    relocNum = pRelHdr->sh_size / pRelHdr->sh_entsize;

    /* Relocation loop */

    relocError = FALSE;

    for (relocIdx = 0; relocIdx < relocNum; relocIdx++)
	{
	/* Read relocation command (SHT_REL type) */

	if ((posCurRelocCmd = elfRelocRelEntryRd (fd,
						  posCurRelocCmd,
						  &relocCmd)) == ERROR)
	    {
	    errnoSet (S_loadElfLib_READ_SECTIONS);
	    return ERROR;
	    }

	/*
	 * If the target symbol's address is null, then this symbol is
	 * undefined. The computation of its value can be out of range and
	 * a warning message has already be displayed for this so let's not
	 * waste our time on it.
	 *
	 * Note: ELF32_R_SYM(relocCmd.r_info) gives the index of the symbol
	 *       in the module's symbol table. This same index is used to
	 *       store the symbol information (address and type for now) in
	 *       the symInfoTbl table (see loadElfSymTabProcess()).
	 */

	if ((pSymAdrs = symInfoTbl [ELF32_R_SYM
					 (relocCmd.r_info)].pAddr) == NULL)
	    continue;

	/*
	 * Calculate actual remote address that needs relocation, and
	 * perform external or section relative relocation.
	 */

	pRelocAdrs = (void *)((Elf32_Addr)*pScnAddr + relocCmd.r_offset);

	if (relocationSelect (pRelocAdrs, &relocCmd, pSymAdrs) != OK)
	    relocError |= TRUE;
	}

    if (relocError)
	return ERROR;
    else
	return OK;
    }

/*******************************************************************************
*
* relocationSelect - select, and execute, the appropriate relocation
*
* This routine selects, then executes, the relocation computation as per the
* relocation command.
*
* NOTE 
* This routine should use two different errnos:
*  - S_loadElfLib_UNSUPPORTED: when a relocation type is not supported on
*    purpose.
*  - S_loadElfLib_UNRECOGNIZED_RELOCENTRY: when a relocation type is not taken
*    into account in this code (default case of the switch).
*
* RETURNS : OK or ERROR if computed value can't be written down in target
*           memory.
*/

LOCAL STATUS relocationSelect
    (
    void *		pRelocAdrs,	/* Addr where the relocation applies  */
    Elf32_Rel *		pRelocCmd,	/* Relocation command 		      */
    void *		pSymAdrs	/* Addr of sym involved in relocation */
    )
    {
    switch (ELF32_R_TYPE (pRelocCmd->r_info))
	{
	case (R_386_NONE):				/* none */
	    break;

	case (R_386_32):				/* word32, S + A */
	    if (elfI86Addr32Reloc (pRelocAdrs, pSymAdrs) != OK)
		return ERROR;
	    break;

	case (R_386_PC32):				/* word32, S + A - P */
	    if (elfI86Pc32Reloc (pRelocAdrs, pSymAdrs) != OK)
		return ERROR;
	    break;

	default:
	    printErr ("Unknown relocation type %d\n",
		      ELF32_R_TYPE (pRelocCmd->r_info));
	    errnoSet (S_loadElfLib_UNRECOGNIZED_RELOCENTRY);
	    return ERROR;
	    break;
	}

    return OK;
    }

/*******************************************************************************
*
* elfI86Addr32Reloc - perform the R_386_32 relocation
*
* This routine handles the R_386_32 relocation (word32, S + A).
*
* RETURNS : OK or ERROR if computed value can't be written down in target
*	    memory.
*/

LOCAL STATUS elfI86Addr32Reloc
    (
    void *	 pRelocAdrs,	/* address where relocation applies */
    void *	 pSymAdrs	/* address of symbol involved in relocation */
    )
    {
    UINT32	 value;		/* relocation value to be stored in memory */
    UINT32 	addend;		/* constant value used for relocation */

    MEM_READ_32 (pRelocAdrs, addend);	/* get addend from memory */

    value = (UINT32)pSymAdrs + addend;

    MEM_WRITE_32(pRelocAdrs, value);	/* write back relocated value */

    return OK;
    }

/*******************************************************************************
*
* elfI86Pc32Reloc - perform the R_386_PC32 relocation
*
* This routine handles the R_386_PC32 relocation (word32, S + A - P).
*
* RETURNS : OK or ERROR if computed value can't be written down in target
*	    memory.
*/

LOCAL STATUS elfI86Pc32Reloc
    (
    void *	 pRelocAdrs,	/* address where relocation applies */
    void *	 pSymAdrs	/* address of symbol involved in relocation */
    )
    {
    UINT32	 value;		/* relocation value to be stored in memory */
    UINT32	 addend;	/* constant value used for relocation */

    MEM_READ_32 (pRelocAdrs, addend);	/* get addend from memory */

    value = (UINT32)pSymAdrs  + addend  - (UINT32)pRelocAdrs;

    MEM_WRITE_32(pRelocAdrs, value);	/* write back relocated value */

    return OK;
    }

/******************************************************************************
*
* elfI86ModuleVerify - check the object module format for i86 target arch.
*
* This routine contains the heuristic required to determine if the object
* file belongs to the OMF handled by this OMF reader, with care for the target
* architecture.
* It is the underlying routine for loadElfModuleIsOk().
*
* RETURNS: TRUE or FALSE if the object module can't be handled.
*/

LOCAL BOOL elfI86ModuleVerify
    (
    UINT32	machType,	/* Module's target arch 	    */
    BOOL *	sdaIsRequired	/* TRUE if SDA are used by the arch */
    )
    {
    BOOL	moduleIsForTarget = TRUE;   /* TRUE if intended for target */

    *sdaIsRequired = FALSE;

    if (machType != EM_ARCH_MACHINE)
	{
	moduleIsForTarget = FALSE;
	errnoSet (S_loadElfLib_HDR_READ);
	}

    return moduleIsForTarget;
    }

/******************************************************************************
*
* elfI86Init - Initialize arch-dependent parts of loader
*
* This routine initializes the function pointers for module verification
* and segment relocation.
*
* RETURNS : OK
*/

STATUS elfI86Init
    (
    FUNCPTR * pElfModuleVerifyRtn,
    FUNCPTR * pElfRelSegRtn
    )
    {
    *pElfModuleVerifyRtn = &elfI86ModuleVerify;
    *pElfRelSegRtn 	 = &elfI86SegReloc;

    return OK;
    }

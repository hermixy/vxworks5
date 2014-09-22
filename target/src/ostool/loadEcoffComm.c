/* loadEcoffComm.c - UNIX extended coff object module common routines */

/* Copyright 1984-1992 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01a,23jul92,ajm  created
*/

/*
DESCRIPTION
This module contains the routines that are common to dynamic loading
and booting of ECOFF object files.  Including this module significantly
reduces the size of the standard bootroms.

SEE ALSO: loadLib, usrLib
.pG "Basic OS"
*/

#include "vxWorks.h"
#include "loadEcoffComm.h"
#include "ecoffSyms.h"
#include "ecoff.h"

LOCAL char fileTypeUnsupported [] =
    "loadEcoffComm error: File type with magic number %#x is not supported.\n";

/*******************************************************************************
*
* ecoffHdrRead - read in COFF header and swap if necessary
* 
*/

STATUS ecoffHdrRead
    (
    int fd,
    FILHDR *pHdr,
    BOOL *pSwap 
    )
    {

    if (fioRead (fd, (char *) pHdr, sizeof (*pHdr)) != sizeof (*pHdr))
	{
	return (ERROR);
	}

    switch (pHdr->f_magic)
	{
	case MIPSEBMAGIC:	/* big-endian target, same sex headers */
	    *pSwap = FALSE;
	    break;
	case SMIPSEBMAGIC:	/* big-endian target, opposite sex headers */
	    *pSwap = TRUE;
	    break;
	case MIPSELMAGIC:	/* little-endian target, same sex headers */
	case SMIPSELMAGIC:	/* little-endian target, opposite sex headers*/
	default:
	    printErr (fileTypeUnsupported, pHdr->f_magic);
	    return (ERROR);
	    break;
	}

    if (*pSwap)
	swapCoffhdr(pHdr);

    return (OK);
    }

/*******************************************************************************
*
* ecoffOpthdrRead - read in COFF optional header and swap if necessary
* 
*/

STATUS ecoffOpthdrRead
    (
    int     fd,
    AOUTHDR *pOptHdr,
    FILHDR  *pHdr,
    BOOL    swapTables 
    )
    {

    if (fioRead (fd, (char *)pOptHdr,(int) pHdr->f_opthdr) != pHdr->f_opthdr)
        {
        return (ERROR);
        }

    if (swapTables)
	swapCoffoptHdr (pOptHdr);

    return (OK);
    }

/*******************************************************************************
*
* swapCoffoptHdr - swap endianess of COFF optional header
*
*/

void swapCoffoptHdr 
    (
    AOUTHDR *pOptHdr           /* module's ECOFF optional header */
    )
    {
    AOUTHDR tempOptHdr;
    swab ((char *) &pOptHdr->magic, (char *) &tempOptHdr.magic,
        sizeof(pOptHdr->magic));
    swab ((char *) &pOptHdr->vstamp, (char *) &tempOptHdr.vstamp,
        sizeof(pOptHdr->vstamp));
    swabLong ((char *) &pOptHdr->tsize, (char *) &tempOptHdr.tsize);
    swabLong ((char *) &pOptHdr->dsize, (char *) &tempOptHdr.dsize);
    swabLong ((char *) &pOptHdr->bsize, (char *) &tempOptHdr.bsize);
    swabLong ((char *) &pOptHdr->entry, (char *) &tempOptHdr.entry);
    swabLong ((char *) &pOptHdr->text_start, (char *) &tempOptHdr.text_start);
    swabLong ((char *) &pOptHdr->data_start, (char *) &tempOptHdr.data_start);
    swabLong ((char *) &pOptHdr->bss_start, (char *) &tempOptHdr.bss_start);
    swabLong ((char *) &pOptHdr->gprmask, (char *) &tempOptHdr.gprmask);
    swabLong ((char *) &pOptHdr->cprmask[0], (char *) &tempOptHdr.cprmask[0]);
    swabLong ((char *) &pOptHdr->cprmask[1], (char *) &tempOptHdr.cprmask[1]);
    swabLong ((char *) &pOptHdr->cprmask[2], (char *) &tempOptHdr.cprmask[2]);
    swabLong ((char *) &pOptHdr->cprmask[3], (char *) &tempOptHdr.cprmask[3]);
    swabLong ((char *) &pOptHdr->gp_value, (char *) &tempOptHdr.gp_value);
    bcopy ((char *) &tempOptHdr, (char *) pOptHdr, sizeof(AOUTHDR));
    }

/*******************************************************************************
*
* swapCoffhdr - swap endianess of COFF header
* 
*/

void swapCoffhdr
    (
    FILHDR *pHdr 		/* module's ECOFF header */
    )
    {
    FILHDR tempHdr;
    swab ((char *) &pHdr->f_magic, (char *) &tempHdr.f_magic, 
	sizeof(pHdr->f_magic));
    swab ((char *) &pHdr->f_nscns, (char *) &tempHdr.f_nscns, 
	sizeof(pHdr->f_magic));
    swabLong ((char *) &pHdr->f_timdat, (char *) &tempHdr.f_timdat);
    swabLong ((char *) &pHdr->f_symptr, (char *) &tempHdr.f_symptr);
    swabLong ((char *) &pHdr->f_nsyms, (char *) &tempHdr.f_nsyms);
    swab ((char *) &pHdr->f_opthdr, (char *) &tempHdr.f_opthdr, 
	sizeof(pHdr->f_opthdr));
    swab ((char *) &pHdr->f_flags, (char *) &tempHdr.f_flags, 
	sizeof(pHdr->f_flags));
    bcopy ((char *) &tempHdr, (char *) pHdr, sizeof(FILHDR));
    }

/*******************************************************************************
*
*  swabLong - swap endianess of long word
*
*/

void swabLong 
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

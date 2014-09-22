/* bootElfLib.c - ELF module boot loader */

/* Copyright 1984-1996 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01f,11dec01,tlc  Add endianess checking to elfHdrRead routine.  SPR #66152
                 fix.
01e,09may96,p_m  ifdef'd PPC specific undefined macros to allow other arch build
01d,31may95,caf  changed EM_ARCH_MACHINE_ALT to EM_ARCH_MACH_ALT.
01c,25may95,kvk	 added EM_ARCH_MACHINE_ALT as valid arch for elfHdrRead
01b,01nov94,kdl	 added default definition of EM_ARCH_MACHINE.
01a,25oct93,cd   created from bootEcoffLib v01d
*/

/*
DESCRIPTION
This library provides an object module boot loading facility for ELF
format files.  Any ELF format file may be boot loaded into memory.
Modules may be boot loaded from any I/O stream.
INCLUDE FILE: bootElfLib.h

SEE ALSO: bootLoadLib
.pG "Basic OS"
*/

#include "vxWorks.h"
#include "stdio.h"
#include "bootLoadLib.h"
#include "bootElfLib.h"
#include "loadElfLib.h"
#include "elf.h"
#include "elftypes.h"
#include "ioLib.h"
#include "fioLib.h"
#include "cacheLib.h"
#include "errnoLib.h"
#include "stdlib.h"
#include "string.h"

#ifndef EM_ARCH_MACHINE
#define EM_ARCH_MACHINE -1		/* default */
#endif

#undef DEBUG
#ifdef DEBUG
LOCAL STATUS bootElfDebug = 1;
#define DBG(x) if (bootElfDebug) sysPrintf x
#else
#define DBG(x)
#endif


/*
 * Define this is if FIOWHERE cannot be relied upon to return the current
 * file position.  Note that defining this makes the loader non reentrant.
 */
#define FIOWHEREFAILS

/*
 * Define this if lseek does not work on all possible sources of input.
 * Note that the current implementation of lseek (vxWorks5.1)
 * uses FIOWHERE...
 */
#define LSEEKFAILS

#if defined(LSEEKFAILS)
#define lseek internalLseek
#endif
#if defined(LSEEKFAILS) && defined(FIOWHEREFAILS)
#define read internalRead
#else
#define read fioRead
#endif

/* forward static functions */
LOCAL STATUS elfHdrRead (int fd, Elf32_Ehdr *pHdr);

#if defined(LSEEKFAILS) && defined(FIOWHEREFAILS)

LOCAL int filePosition;

/*****************************************************************************
*
* internalRead - read data from load file
*
* This routine reads a chunk of data from the load file.  It will attempt to
* read <size> bytes by calling fioRead.  Because sockets do not support the
* FIOWHERE ioctl, this routine keeps a local file position variable so that
* internalLSeek can find out where it is.
*
* RETURNS: Number of bytes read.
*
* SEE ALSO: internalLSeek
*
* NOMANUAL
*/

LOCAL int internalRead
    (
    int fd,			/* file descriptor */
    char *buf,			/* data buffer */
    int size			/* number of bytes to read */
    )
    {
    int n;
    n = fioRead (fd, buf, size);
    if (n > 0)
	filePosition += n;
    DBG(("read(%d)=%d\n", size, n));
    return (n);
    }
#endif

#if defined(LSEEKFAILS)
/*****************************************************************************
*
* internalLseek - seek to position in load file
*
* This routine seeks to a given position in the load file.
* The seek is emulated by performing calls to read until the correct
* file position is reached, thus there is an implicit assumption that you will
* only ever want to seek forwards.
* If the FIOWHERE ioctl does not operate correctly on sockets (vxWorks 5.1beta)
* the file position is determined by a local variable that is updated by read.
*
* RETURNS: The new offset from the beginning of the file, or ERROR.
*
* NOMANUAL
*/

LOCAL long internalLseek
    (
    int fd,			/* file descriptor */
    long offset,		/* position to seek to */
    int whence			/* relative file position */
    )
    {
    long where, howmany;
    char buf[4096];

#if defined(FIOWHEREFAILS)
    where = filePosition;
#else
    where = ioctl (fd, FIOWHERE, 0);
#endif
    switch (whence) {
      case L_SET:
	break;
      case L_INCR:
	offset += where;
	break;
      case L_XTND:
      default:
	return (ERROR);	
	}
    if (offset < where)
	{
	DBG(("backward seek where=%d offset=%d\n", where, offset));
	return (ERROR);
	}

    for (howmany = offset - where;
	 howmany >= sizeof(buf); howmany -= sizeof(buf))
	{
	if (read (fd, buf, sizeof(buf)) != sizeof(buf))
	    {
	    DBG(("lseek read failure\n"));
	    return (ERROR);
	    }
	}
    if (howmany > 0 && (read (fd, buf, howmany) != howmany))
	{
	DBG(("lseek read failure\n"));
	return (ERROR);
	}
    DBG(("lseek returns offset\n"));
    return (offset);
    }
#endif

/*******************************************************************************
*
* bootElfModule - load an object module into memory
*
* This routine loads an object module in ELF format from the specified
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

LOCAL STATUS bootElfModule 
    (
    int fd,
    FUNCPTR *pEntry 			/* entry point of module */
    )
    {
    Elf32_Ehdr ehdr;
    Elf32_Phdr *pPhdr, *pph;
    int i;
    int segment = 0;
    unsigned int nbytes;

    if (elfHdrRead (fd, &ehdr) != OK)
	{
	errnoSet (S_loadElfLib_HDR_READ);
	return (ERROR);
	}

    /* check the program header validity */
    if (ehdr.e_phoff == 0 ||
	ehdr.e_phnum == 0 || ehdr.e_phentsize != sizeof(Elf32_Phdr))
	{
	errnoSet (S_loadElfLib_HDR_ERROR);
	return (ERROR);
	}

    /* read the program header */
    nbytes = ehdr.e_phnum * ehdr.e_phentsize;
    if ((pPhdr = (Elf32_Phdr *) malloc (nbytes)) == NULL)
	{
	errnoSet (S_loadElfLib_PHDR_MALLOC);
	return (ERROR);
	}
    if (lseek (fd, ehdr.e_phoff, L_SET) == ERROR ||
	read (fd, (char *) pPhdr, nbytes) != nbytes)
	{
	errnoSet (S_loadElfLib_PHDR_READ);
      fail:
	free (pPhdr);
	return (ERROR);
	}

    /* read each loadable segment */
    for (i = 0, pph = pPhdr; i < ehdr.e_phnum; i++, pph++)
	{
	if (pph->p_type == PT_LOAD)
	    {
	    /* This segment is to be loaded, so do it */
	    printf ("%s%ld", segment++ == 0 ? "" : " + ", pph->p_memsz);

	    /* load the bits that are present in the file */
	    if (pph->p_filesz) {
		if (lseek (fd, pph->p_offset, L_SET) != pph->p_offset ||
		    read (fd, (char *) pph->p_vaddr, pph->p_filesz) != pph->p_filesz)
		    {
		    errnoSet (S_loadElfLib_READ_SECTIONS);
		    goto fail;
		    }
		}

	    /* zap the bits that didn't appear in the file */
	    if (pph->p_filesz < pph->p_memsz)
		bzero ((char *) (pph->p_vaddr+pph->p_filesz),
		       pph->p_memsz - pph->p_filesz);
	    /* if we might end up executing this we need to flush the cache */
	    if (pph->p_flags & PF_X)
		cacheTextUpdate ((char *)pph->p_vaddr, pph->p_memsz);
	    }
	}

    printf ("\n");
    *pEntry = (FUNCPTR) ehdr.e_entry;
    return (OK);
    }
    

/*******************************************************************************
*
* elfHdrRead - read in elf header
*
* This routine read in the elf header from the specified file and verify it.
* Only relocatable and executable files are supported.
*
* RETURNS: OK or ERROR
*
* NOMANUAL
*/

LOCAL STATUS elfHdrRead
    (
    int fd,			/* file to read in */
    Elf32_Ehdr * pHdr		/* elf header */
    )
    {
    int endiannessIsOk;
    
#if defined(LSEEKFAILS) && defined(FIOWHEREFAILS)
    filePosition = 0;		/* assume we are at beginning of file */
#endif
    if (read (fd, (char *) pHdr, sizeof (*pHdr)) != sizeof (*pHdr))
        {
	printf ("Erroneous header read\n");
        return (ERROR);
        }
    /* Is it an ELF file? */

    if (strncmp ((char *) pHdr->e_ident, (char *) ELFMAG, SELFMAG) != 0)
        return (ERROR);

    /* Does the endianess match? */ 

    if (((_BYTE_ORDER == _BIG_ENDIAN) &&
         (pHdr->e_ident[EI_DATA] == ELFDATA2MSB)) ||
        ((_BYTE_ORDER == _LITTLE_ENDIAN) &&
         (pHdr->e_ident[EI_DATA] == ELFDATA2LSB)))
         endiannessIsOk = TRUE;
    else
         endiannessIsOk = FALSE;

    if (!endiannessIsOk)
        {
        printf ("Endianness is incorrect\n");
        return ERROR;
        }

    /* Is the size correct? */

    if (pHdr->e_ehsize != sizeof (*pHdr))
	{
	printf ("Size is incorrect\n");
	return (ERROR);
	}

    /* check machine type */

#if (CPU_FAMILY==PPC)
    if (pHdr->e_machine != EM_ARCH_MACHINE &&
		pHdr->e_machine != EM_ARCH_MACH_ALT)
        return (ERROR);
#endif /* (CPU_FAMILY==PPC) */

    /* Relocatable and Excecutable files supported */

    if ((pHdr->e_type == ET_REL) || (pHdr->e_type == ET_EXEC))
	{
        return (OK);
	}
    else
	return (ERROR);
    }

/********************************************************************************
* bootElfInit - initialize the system for elf load modules
*
* This routine initialises VxWorks to use ELF format for
* boot loading modules.
*
* RETURNS:
* OK, or
* ERROR if XXX
*
* SEE ALSO: loadModuleAt()
*/

STATUS bootElfInit
    (
    )
    {
    bootLoadRoutine = bootElfModule;
    return (OK);
    }

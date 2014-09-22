/* bootEcoffLib.c - UNIX extended coff object module boot loader */

/* Copyright 1984-1992 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01d,22apr93,caf  ansification: added cast to cacheTextUpdate() parameter.
01c,31jul92,dnw  changed to use cacheTextUpdate().
01b,23jul92,jmm  removed #include "loadCommonLib.h"
01a,23jul92,ajm  created
*/

/*
DESCRIPTION
This library provides an object module boot loading facility particuarly for 
the MIPS compiler environment.  Any MIPS SYSV ECOFF
format file may be boot loaded into memory.
Modules may be boot loaded from any I/O stream.
INCLUDE FILE: bootEcoffLib.h

SEE ALSO: bootLoadLib
.pG "Basic OS"
*/

#include "vxWorks.h"
#include "stdio.h"
#include "bootEcoffLib.h"
#include "ecoffSyms.h"
#include "ecoff.h"
#include "ioLib.h"
#include "fioLib.h"
#include "bootLoadLib.h"
#include "loadLib.h"
#include "memLib.h"
#include "pathLib.h"
#include "string.h"
#include "cacheLib.h"

/*******************************************************************************
*
* bootEcoffModule - load an object module into memory
*
* This routine loads an object module in ECOFF format from the specified
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

LOCAL STATUS bootEcoffModule 
    (
    int fd,
    FUNCPTR *pEntry 			/* entry point of module */
    )
    {
    FILHDR hdr;				/* module's ECOFF header */
    AOUTHDR optHdr;             	/* module's ECOFF optional header */
    int nBytes;
    char tempBuf[MAX_SCNS * SCNHSZ];
    int tablesAreLE;

    /* read object module header */

    if (ecoffHdrRead (fd, &hdr, &tablesAreLE) != OK)
	return (ERROR);

    /* read in optional header */

    if (ecoffOpthdrRead (fd, &optHdr, &hdr, tablesAreLE) != OK)
	return (ERROR);

    /* 
    *  Read till start of text, for some reason we can't do an ioctl FIOWHERE
    *  on a rcmd opened file descriptor. 
    */

    nBytes = fioRead (fd, tempBuf, (N_TXTOFF(hdr, optHdr) - FILHSZ - AOUTHSZ));
    if (nBytes < (N_TXTOFF(hdr, optHdr) - FILHSZ - AOUTHSZ))
        return (ERROR);

    printf ("%d", optHdr.tsize);
    nBytes = fioRead (fd, (char *) optHdr.text_start, (int) optHdr.tsize);
    if (nBytes < optHdr.tsize)
        return (ERROR);

    cacheTextUpdate ((void *) optHdr.text_start, optHdr.tsize);

    printf (" + %d", optHdr.dsize);
    nBytes = fioRead (fd, (char *) optHdr.data_start, (int) optHdr.dsize);
    if (nBytes < optHdr.dsize)
        return (ERROR);

    printf (" + %d\n", optHdr.bsize);
    bzero ((char *) optHdr.bss_start, (int) optHdr.bsize);

    *pEntry = (FUNCPTR) optHdr.entry;
    return (OK);
    }

/********************************************************************************
* bootEcoffInit - initialize the system for ecoff load modules
*
* This routine initialized VxWorks to use an extended coff format for
* boot loading modules.
*
* RETURNS:
* OK, or
* ERROR if XXX
*
* SEE ALSO: loadModuleAt()
*/

STATUS bootEcoffInit
    (
    )
    {
    /* XXX check for installed ? */
    bootLoadRoutine = bootEcoffModule;
    return (OK);
    }

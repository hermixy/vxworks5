/* bootAoutLib.c - UNIX a.out object module boot loader */

/* Copyright 1984-1992 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01b,09feb93,jmm  added call to cacheTextUpdate() in bootAoutModule (SPR #1932)
01a,27jul92,elh  created from loadAoutLib.c
*/

/*
DESCRIPTION

This library provides an object module boot loading facility for 
the a.out object module format. 

SEE ALSO: bootLoadLib
.pG "Basic OS"
*/

/* includes */

#include "vxWorks.h"
#include "a_out.h"
#include "fioLib.h"
#include "stdio.h"
#include "bootLoadLib.h"
#include "cacheLib.h"


/*******************************************************************************
*
* bootAoutModule - load an a.out object module into memory
*
* This routine loads an object module in a.out format from the specified
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

LOCAL STATUS bootAoutModule 
    (
    int fd,
    FUNCPTR *pEntry 			/* entry point of module */
    )
    {
    struct exec hdr;			/* a.out header */
    int nBytes;

    nBytes = fioRead (fd, (char *) &hdr, sizeof (struct exec));
    if (nBytes < sizeof (struct exec))
        return (ERROR);

    printf ("%d", hdr.a_text);

    nBytes = fioRead (fd, (char *) hdr.a_entry, (int) hdr.a_text);
    if (nBytes < hdr.a_text)
        return (ERROR);

    cacheTextUpdate ((void *) hdr.a_entry, hdr.a_text);

    printf (" + %d", hdr.a_data);

    nBytes = fioRead (fd, (char *) (hdr.a_entry + hdr.a_text),
                      (int) hdr.a_data);

    if (nBytes < hdr.a_data)
        return (ERROR);

    printf (" + %d\n", hdr.a_bss);

    *pEntry = (FUNCPTR) hdr.a_entry;
    return (OK);
    }

/********************************************************************************
* bootAoutInit - initialize the system for aout load modules
*
* This routine initialized VxWorks to use an a.out format for
* boot loading modules.
*
* RETURNS:
* OK, or
* ERROR if XXX
*
* SEE ALSO: loadModuleAt()
*/

STATUS bootAoutInit
    (
    )
    {
    /* XXX check for installed ? */
    bootLoadRoutine = bootAoutModule;
    return (OK);
    }

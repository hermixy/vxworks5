/* bootLoadLib.c - UNIX object module boot loader */

/* Copyright 1984-1992 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01b,18jul92,smb  Changed errno.h to errnoLib.h.
01a,01jun92,ajm  written: some culled from old loadLib
*/

/*
DESCRIPTION
This library provides a generic bootstrap object module loading facility.  Any
supported format files may be loaded into memory.
Modules may be loaded from any I/O stream.

EXAMPLE
.CS
    fdX = open ("/devX/objFile", O_RDONLY);
    close (fdX);
.CE
This code fragment would load the object file "objFile" located on
device "/devX/" into memory which would be allocated from the system
memory pool.

INCLUDE FILE: loadLib.h

SEE ALSO: usrLib, symLib, memLib,
.pG "Basic OS"
*/

/* LINTLIBRARY */

#include "vxWorks.h"
#include "bootLoadLib.h"
#include "errnoLib.h"

/* define generic boot loader */

FUNCPTR bootLoadRoutine = (FUNCPTR) NULL;

/******************************************************************************
*
* bootLoadModule - bootstrap load an object module into memory
*
* This routine is the underlying routine to loadModuleAt().  This interface
* allows specification of the the symbol table used to resolve undefined
* external references and to which to add new symbols.
*
* RETURNS:
* OK, or
* ERROR if can't read file or not enough memory or illegal file format
*
* NOMANUAL
*/

STATUS bootLoadModule 
    (
    FAST int fd,        /* fd from which to read module */
    FUNCPTR *pEntry     /* entry point of module */
    )
    {
    if (bootLoadRoutine == NULL)
	{
        errnoSet (S_bootLoadLib_ROUTINE_NOT_INSTALLED);
	return (ERROR);

	}
    else
	{
        return ((bootLoadRoutine) (fd, pEntry));
	}
    }

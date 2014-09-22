/* symTblLockLib.c - sym table locking mechanisms */

/* Copyright 1984-1994 Wind River Systems, Inc. */
/*
modification history
--------------------
01c,05jan94,kdl	 general cleanup.
01b,13dec93,dvs	 made NOMANUAL
01a,06apr93,smb	 created
*/

/*
DESCRIPTION
This module provides functionality for synchronizing the use of 
a symbol table created by symTblCreate(). 

INCLUDE FILES: 

SEE ALSO:

NOMANUAL
*/

#include "vxWorks.h"
#include "semLib.h"
#include "symLib.h"

/*******************************************************************************
*
* symTblLock - lock a name table.
*
* This routine provides a mechanism for locking out a symbol table so that
* it cannot be used by other tasks.
*
* RETURNS: OK, or ERROR if <symTblId> invalid
*
* NOMANUAL
*/

STATUS symTblLock
    (
    SYMTAB_ID symTblId         /* ID of symbol table to lock */
    )
    {
    if (OBJ_VERIFY (symTblId, symTblClassId) != OK)
        return (ERROR);                         /* invalid symbol table ID */

    semTake (&symTblId->symMutex, WAIT_FOREVER);

    return (OK);
    }

/*******************************************************************************
*
* symTblUnlock - Unlock a name table.
*
* This routine unlocks a symbol table that has be locked via symTblLock().
*
* RETURNS: OK, or ERROR if <symTblId> invalid
*
* NOMANUAL
*/

STATUS symTblUnlock
    (
    SYMTAB_ID symTblId         /* ID of symbol table to unlock */
    )
    {
    if (OBJ_VERIFY (symTblId, symTblClassId) != OK)
        return (ERROR);                         /* invalid symbol table ID */

    semGive (&symTblId->symMutex);          /* release mutual exclusion */

    return (OK);
    }

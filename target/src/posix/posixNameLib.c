/* posixNameLib.c - POSIX name table support */

/* Copyright 1984-1994 Wind River Systems, Inc. */

/*
modification history
--------------------
01d,01feb94,dvs  documentation changes.
01c,05jan94,kdl	 general cleanup.
01b,13dec93,dvs	 made NOMANUAL
01a,06apr93,smb	 created
*/

/*
DESCRIPTION
The library includes functions needed to initialize and manipulate 
names associated with the POSIX specific resources. 

INCLUDE FILES: posixName.h

SEE ALSO: semaphore, mqueue POSIX libraries. symLibInit()

NOMANUAL
*/

#include "vxWorks.h"
#include "posixName.h"
#include "objLib.h"
#include "memLib.h"
#include "symLib.h"

/* global variables */

SYMTAB_ID       posixNameTbl;             /* POSIX name table id */

/*******************************************************************************
*
* posixNameTblInit - initialize the POSIX name table
*
* This routine should be called to initialize the POSIX name table before
* any POSIX functionality that assigns and manipulates names and objects.
*
* This routine creates and initializes a symbol table with a hash table of a
* specified size.  The size of the hash table is specified as a power of two.
* For example, if <posixTblHashSize> is 6, a 64-entry hash table is created.
*
* If <posixTblHashSize> is equal to zero, then a default size of 
* POSIX_TBL_HASH_SIZE_LOG2 is used to create the table.
*
* RETURNS: OK
*
* NOMANUAL
*/

SYMTAB_ID posixNameTblInit 
    (
    int posixTblHashSize
    )
    {

    /* This check is needed because symLibInit is not reentrant */
    if (OBJ_VERIFY (&symTblClassId->objCore, classClassId) != OK)
	symLibInit ();                     /* initialize symbol table package */

    if (posixTblHashSize == 0)
	posixTblHashSize = POSIX_TBL_HASH_SIZE_LOG2;

    posixNameTbl = symTblCreate (posixTblHashSize, FALSE, memSysPartId);

    return (posixNameTbl);
    }


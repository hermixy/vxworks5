/* wvServer.c - WindView RPC server */

/* Copyright 1994 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01e,05apr94,smb  corrected documentation
01e,22feb94,smb  corrected Copyright date
01d,20jan94,c_s  doc cleanup.
01c,14jan94,c_s  added online doc for wvServerInit, description block.
01b,30dec93,c_s  service routines now place results in heap memory, for 
		   reentrancy.  wvServer () entry point now has arguments
		   for priority and stack size.  Since the svc module
		   is now hand coded rather than the output of rpcgen, 
		   the nonsense with main () and __main () is removed.
01a,06dec93,c_s  written.
*/

/*
DESCRIPTION

This module implements the RPC calls for the WindView command server, and
exports the routine wvServerInit to start up the RPC service.

INCLUDE FILES: wvLib.h wvRpc.h
NOMANUAL
SEE ALSO: 
*/

/* includes */

#include "vxWorks.h"
#include "symLib.h"
#include "string.h"
#include "taskLib.h"

#include "rpc/rpc.h"
#include "rpcLib.h"
#include "wvRpc.h"
#include "wvLib.h"

/* externs */

extern SYMTAB_ID sysSymTbl;

/* forward declarations */

void wvServer (void);

/*******************************************************************************
*
* wvproc_symtab_lookup_1 - RPC service routine for symbol-table lookup
*
* The named symbol is looked up in sysSymTbl.  The name is tried first
* verbatim and then with an underscore prepended.
* before kernel objects are initialized.
*
* RETURNS:  a pointer to the unsigned long result on the heap; if the
*	    symbol couldn't be found that result will be zero.  If memory
*           couldn't be allocated NULL is returned.
* SEE ALSO:
* NOMANUAL
*/

u_long *wvproc_symtab_lookup_1 
    (
    char **name
    )

    {
    SYM_TYPE type;
    u_long *value = (u_long *) malloc (sizeof (u_long));
    int status;
    char tmp_name [80];

    if (!value)
	{
	return NULL;
	}

    *tmp_name = '_';
    strcpy (tmp_name+1, *name);

    status = symFindByName (sysSymTbl, tmp_name+1, (char **) value, &type);

    if (status == ERROR)
	{
	status = symFindByName (sysSymTbl, tmp_name, (char **) value, &type);
	}
       
    if (status == ERROR)
	{
	*value = 0;
	}

    return value;
    }

/*******************************************************************************
*
* wvproc_task_spawn_1 - RPC service routine for task spawning
*
* The passed <pSpawnRec> contains the arguments for taskSpawn () (see
* wvRpc.x).
*
* RETURNS:  the return value of taskSpawn on the heap, or NULL if memory 
*           couldn't be allocated.
* SEE ALSO:
* NOMANUAL
*/

u_long *wvproc_task_spawn_1 
    (
    struct taskSpawnRec *pSpawnRec
    )

    {
    u_long *value = (u_long *) malloc (sizeof (u_long));

    if (!value)
	{
	return NULL;
	}

    *value = taskSpawn (pSpawnRec->name,
		        pSpawnRec->priority,
		        pSpawnRec->options,
		        pSpawnRec->stackSize,
		        (FUNCPTR) pSpawnRec->entry,
		        pSpawnRec->arg [0],
		        pSpawnRec->arg [1],
		        pSpawnRec->arg [2],
		        pSpawnRec->arg [3],
		        pSpawnRec->arg [4],
		        pSpawnRec->arg [5],
		        pSpawnRec->arg [6],
		        pSpawnRec->arg [7],
		        pSpawnRec->arg [8],
		        pSpawnRec->arg [9]);

    return value;
    }

/*******************************************************************************
*
* wvproc_call_function_1 - RPC service routine for calling a function
*
* The passed <pCallRec> contains the function address and arguments to 
* pass.  
*
* RETURNS:  the return value of function on the heap, or NULL if memory 
*           couldn't be allocated.
* SEE ALSO:
* NOMANUAL
*/

u_long *wvproc_call_function_1 
    (
    struct callFuncRec *pCallRec
    )

    {
    u_long *value = (u_long *) malloc (sizeof (u_long));
    unsigned long (*function) () = (unsigned long (*)()) pCallRec->funcPtr;

    if (!value)
	{
	return NULL;
	}

    *value = function (pCallRec->arg[0],
		       pCallRec->arg[1],
		       pCallRec->arg[2],
		       pCallRec->arg[3],
		       pCallRec->arg[4],
		       pCallRec->arg[5],
		       pCallRec->arg[6],
		       pCallRec->arg[7],
		       pCallRec->arg[8],
		       pCallRec->arg[9],
		       pCallRec->arg[10],
		       pCallRec->arg[11],
		       pCallRec->arg[12],
		       pCallRec->arg[13],
		       pCallRec->arg[14],
		       pCallRec->arg[15],
		       pCallRec->arg[16],
		       pCallRec->arg[17],
		       pCallRec->arg[18],
		       pCallRec->arg[19]);

    return value;
    }

/*******************************************************************************
*
* wvServerInit - start the WindView command server on the target (WindView)
*
* This routine spawns the RPC server task for WindView with the indicated 
* stack size and priority.
*
* This routine is invoked automatically from usrConfig.c when 
* INCLUDE_WINDVIEW is defined in configAll.h.  It should be performed
* after usrNetInit() has run.
*
* RETURNS:  
* The task ID of the server task, or ERROR if it could not be started.
*  
* SEE ALSO: wvLib
*/

STATUS wvServerInit 
    (
    int serverStackSize,	/* server task stack size */
    int serverPriority		/* server task priority */
    )

    {
    return taskSpawn ("tWvSvc", serverPriority, 0, serverStackSize,
	              (FUNCPTR) wvSvc, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    }

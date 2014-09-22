/* symSyncLib.c - host/target symbol table synchronization */

/* Copyright 1996-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01t,03may02,jhw  Added VX_FP_TASK task option to tSymSync. (SPR 66307)
01s,03apr02,j_s  correct the miscalculation of length limit of symbol name
		 in syncSymAddHook/syncSymRemoveHook (SPR 75036).
01r,30oct01,jn   use symFindSymbol for symbol lookup (SPR #7453)
01q,01oct01,c_c  Fixed SPR 62583.
01p,02apr99,jdi  corrected spelling of NOMANUAL in syncTgtSafeModCheck()
01o,14mar99,jdi  doc: removed refs to config.h and/or configAll.h (SPR 25663).
01n,01sep98,pcn  Added wtxObjModuleUndefSymAdd definition.
01m,27jul98,elp  fixed stack size for SIMSPARCSOLARIS (SPR #20150).
01l,07jul98,pcn  Added include for wtxObjModuleUndefSymAdd.
01k,24apr98,dbt  fixed WTX timeout error message (SPR #20852).
01j,14jul97,dgp  doc: edit new additions
01i,21feb96,elp	 improved timeout managements (added warning message).
01h,13feb97,elp  managed timeout errors on wtxEventGet() (SPR# 7959)
		 + network doc enhancements (SPR# 7852).
01g,06jan97,elp  fixed a typo.
01f,18dec96,jdi  doc: more enhancements and cleanup; changed title line to
		    reflect new name of module.
01e,12dec96,elp  moved hooks initialization into tSymSync
		 + added global symSyncHWtx to enable timeout modifications.
		 + added symSyncTimeout()
		 + name revision.
01d,10dec96,jdi  doc: cleanup.
01c,13nov96,elp  doc generation.
01b,24oct96,elp  added event notification when synchronization is done
		 + synchronize usual target commands (ld(), symAdd(), ...).
01a,04oct96,elp  written.
*/

/*
DESCRIPTION
This module provides host/target symbol table synchronization.
With synchronization, every module or symbol added to the run-time
system from either the target or host side can be seen by facilities
on both the target and the host. Symbol table synchronization makes it 
possible to use host tools to debug application modules loaded with the 
target loader or from a target file system.  To enable synchronization, 
two actions must be performed: 

.iP 1 6
The module is initialized by symSyncLibInit(), which is called automatically 
when the configuration macro INCLUDE_SYM_TBL_SYNC is defined.
.iP 2
The target server is launched with the `-s' option.
.LP

If synchronization is enabled, `symSyncLib' spawns a synchronization task 
on the target, `tSymSync'. This task behaves as a WTX tool and attaches 
itself to the target server.  When the task starts, it synchronizes target 
and host symbol tables so that every module loaded on the target before the 
target server was started can be seen by the host tools.  This feature is 
particularly useful if VxWorks is started with a target-based startup script 
before the target server has been launched.

The `tSymSync' task synchronizes new symbols that are added
by either the target or the host tools.  The task waits for synchronization 
events on two channels:  a WTX event from the host or a message queue additon
from the target.

The `tSymSync' task, like all WTX tools, must be able to connect to the WTX
registry. To make the WTX registry accessible from the target, do one of
the following:

.iP 1 6
Boot the target from a host on the same subnet as the registry.
.iP 2
Start the registry on the same host the target boots from.
.iP 3
Add the needed routes with routeAdd() calls, possibly in a startup script.
.LP

Neither the host tools nor the target loader wait for synchronization
completion to return.  To know when the synchronization is complete, you
can wait for the corresponding event sent by the target server, or,
if your target server was started with the `-V' option, it prints a
message indicating synchronization has completed.

The event sent by the target server is of the following format:

.tS
    SYNC_DONE <syncType> <syncObj> <syncStatus>
.tE

The following are examples of messages displayed by the target server
indicating synchronization is complete:
.CS
    Added target_modules         to target-server.....done
    Added ttTest.o.68k           to target............done
.CE

If synchronization fails, the following message is displayed:
.CS
    Added gopher.o               to target............failed
.CE

This error generally means that synchronization of the corresponding module
or symbol is no longer possible because it no longer exists in the original
symbol table.  If so, it will be followed by:
.CS
    Removed gopher.o             from target..........failed
.CE

Failure can also occur if a timeout is reached.  
Call symSyncTimeoutSet() to modify the WTX timeout between the target
synchronization task and the target server.

LIMITATIONS
Hardware:  Because the synchronization task uses the WTX protocol to
communicate with the target server, the target must include network
facilities.  Depending on how much synchronization is to be done (number
of symbols to transfer), a reasonable throughput between the target server
and target agent is required (the wdbrpc backend is recommended when large
modules are to be loaded).

Performance:  The synchronization task requires some minor overhead in
target routines msgQSend(), loadModule(), symAdd(), and symRemove(); 
however, if an application sends more than 15 synchronization events,
it will fill the message queue and then need to wait for a synchronization
event to be processed by `tSymSync'. Also, waiting for host synchronization 
events is done by polling; thus there may be some impact on performance if 
there are lower-priority tasks than `tSymSync'.  If no more synchronization 
is needed, `tSymSync' can be suspended.

Known problem:  Modules with undefined symbols that are loaded from the target
are not synchronized; however, they are synchronized if they are loaded
from the host.

SEE ALSO
`tgtsvr'
*/

/* includes */

#include "symSyncLib.h"
#include "rpcLib.h"
#include "loadLib.h"
#include "unldLib.h"
#include "symLib.h"
#include "msgQLib.h"
#include "sysLib.h"
#include "a_out.h"	/* XXX ELP to get N_TYPE definition */
#include "usrLib.h"
#include "logLib.h"

/* typedefs */

typedef struct
    {
    DL_NODE 	node;
    UINT16	tgtGroup;
    UINT32	hostGroup;
    } GROUP_NODE;

/* 
 * dummy type that concatenates arguments as only one can be passed to
 * routines called by symEach().
 */

typedef struct 
	{
	HWTX	hWtx;
	UINT16	tgtGroup;
	UINT32	hostGroup;
	} BY_GROUP_ARG_T;

/* defines */

/* #define DEBUG */

#define H_STR_TO_L(str)		(strtoul (str, NULL, 16))

#define SYNC_TGT_EVT_MAX	15		/* max number of sync rqsts */
#define OBJ_NAME_MAX		30
#define SYNC_TGT_EVT_LGTH	60		/* max event string length */
#define OBJ_LOADED_EVT		"OBJ_LOADED"
#define OBJ_UNLOADED_EVT	"OBJ_UNLOADED"
#define SYM_ADDED_EVT		"SYM_ADDED"
#define SYM_REMOVED_EVT		"SYM_REMOVED"
#define SYM_ADDED_EVT_MSG_NON_NAME_LGTH		34
#define USR_RQST_EVT		"USR_RQST"
#define TIMEOUT 		sysClkRateGet()
					/* for sending/receiving events */

#define SYNC_PRIORITY		90

/* XXX - jhw 
 * The VX_FP_TASK option is needed to prevent corruption of 
 * the floating point registers. All tasks that call
 * (directly or indirectly) C++ code that is compiled without 
 * '-fnoexceptions' with gnu 2.96 are affected.
 * See SPR 66307,70995 for additional info.
 * This module calls unldByModuleId() which calls C++ destructors.
 */

#define SYNC_OPTIONS		VX_FP_TASK

#if (CPU_FAMILY == MC680X0) 
#define SYNC_STACK_SIZE		8000
#elif (CPU_FAMILY == I960)
#define SYNC_STACK_SIZE		5000
#elif ((CPU_FAMILY == SIMSPARCSOLARIS) || (CPU_FAMILY == SPARC) || \
       (CPU_FAMILY == SIMSPARCSUNOS))
#define SYNC_STACK_SIZE		15000
#else
#define SYNC_STACK_SIZE		5000
#endif

/* globals */

MSG_Q_ID		symSyncMsgQId;	/* target synchro event container */
HWTX			symSyncHWtx;	/* symbol synchro WTX handle */

/* locals */

LOCAL const char	SYM_SYNC_TOOL_NAME[] = 	"symTblsSync";
LOCAL DL_LIST		symSyncLst;		/* sync modules list */
LOCAL DL_LIST		symSyncTmpLst;		/* tmp buffer */
LOCAL BOOL		symSyncKill;		/* to exit tSymSync loop */
LOCAL UINT32		symSyncWtxTimeout = 5000;	/* wtx timeout */

IMPORT CLASS_ID			symTblClassId;
IMPORT CLASS_ID			moduleClassId;
IMPORT SYMTAB_ID		sysSymTbl;
IMPORT struct opaque_auth	authCred;

/* forward declarations */

extern 			STATUS wtxObjModuleUndefSymAdd (HWTX, char *, UINT32 );

STATUS			symSRemove (SYMTAB_ID, char *, SYM_TYPE);

/* group list processing */

LOCAL STATUS		groupAdd (DL_LIST *, UINT16, UINT32);
LOCAL GROUP_NODE *	groupFind (DL_LIST *, UINT32, FUNCPTR);
LOCAL GROUP_NODE *	groupRemove (DL_LIST *, UINT32, FUNCPTR);
LOCAL BOOL		isInHostGroupList (DL_NODE *, int);
LOCAL BOOL		isInTgtGroupList (DL_NODE *, int);

LOCAL void		syncGroupUpdate ();
LOCAL STATUS		symSyncMain (HWTX);
LOCAL STATUS		symSyncTaskInit (char *);

/* host -> target synchronization */

LOCAL STATUS		symSyncTgtUpdate (HWTX, char *, char *);
LOCAL STATUS		syncModCTgtUpdate (HWTX, UINT32);
LOCAL STATUS		syncModDTgtUpdate (HWTX, UINT32);
LOCAL STATUS		syncSymAddTgtUpdate (HWTX, char *);
LOCAL MODULE_ID		syncTgtModInfoFill (WTX_MODULE_INFO *);

/* target -> host synchronization */

LOCAL STATUS	symSyncHostUpdate (HWTX, char *, char *);
LOCAL STATUS	syncModCHostUpdate (HWTX, DL_LIST *, MODULE_ID, BOOL *,BOOL);
LOCAL STATUS	syncModDHostUpdate (HWTX, char *, UINT16);
LOCAL STATUS	syncSymAddHostUpdate (HWTX, char *, char *, SYM_TYPE, UINT16);
LOCAL STATUS	syncSymRemHostUpdate (HWTX, char *, SYM_TYPE);
LOCAL STATUS	syncHostModInfoFill (WTX_LD_M_FILE_DESC *, MODULE_ID, BOOL);
LOCAL STATUS	syncUsrUpdate (HWTX, int, int);
LOCAL BOOL	syncTgtSafeModCheck (MODULE_ID , int);
LOCAL BOOL	syncEachSymToHost (char *, int, SYM_TYPE, int, UINT16);
LOCAL BOOL	syncSymByGrpToHost (char *, int, SYM_TYPE, int, UINT16);

LOCAL void	syncLoadHook (MODULE_ID);
LOCAL void	syncUnldHook (MODULE_ID);
LOCAL void	syncSymAddHook (char *, char *, SYM_TYPE, UINT16);
LOCAL void	syncSymRemoveHook (char *, SYM_TYPE);

/******************************************************************************
* 
* symSyncLibInit - initialize host/target symbol table synchronization
*
* This routine initializes host/target symbol table synchronization. 
* To enable synchronization, it must be called before a target server
* is started.  It is called automatically if the configuration macro
* INCLUDE_SYM_TBL_SYNC is defined.
*
* RETURNS: N/A
*/

void symSyncLibInit ()
    {

    /* create the target synchronization event folder */
    symSyncMsgQId = msgQCreate (SYNC_TGT_EVT_MAX, SYNC_TGT_EVT_LGTH,
				MSG_Q_FIFO);

    return;
    }

/******************************************************************************
*
* symSyncTimeoutSet - set WTX timeout 
*
* This routine sets the WTX timeout between target server and synchronization
* task.
*
* RETURNS: If <timeout> is 0, the current timeout, otherwise the new timeout
* value in milliseconds.
*/

UINT32 symSyncTimeoutSet
    (
    UINT32	timeout		/* WTX timeout in milliseconds */
    )
    {
    UINT32 curTimeout;

    if (wtxToolConnected (symSyncHWtx))
	{
	if (timeout == 0)
	    {
	    if (wtxTimeoutGet (symSyncHWtx, &curTimeout) == WTX_OK)
		return (curTimeout);
	    return (symSyncWtxTimeout);
	    }
	else
	    {
	    symSyncWtxTimeout = timeout;
	    wtxTimeoutSet (symSyncHWtx, symSyncWtxTimeout);
	    return (symSyncWtxTimeout);
	    }
	}
    
    if (timeout == 0)
	return (symSyncWtxTimeout);
    symSyncWtxTimeout = timeout;
    return (symSyncWtxTimeout);
    }

/******************************************************************************
*
* syncErrHandler - synchronization error handler
* 
* Some WTX errors may happen in an unpredictable way (because information of
* the synchronization task are not up to date). So these errors should be 
* ignored.
*
* RETURNS: FALSE to prevent from calling other error handler. 
*
* NOMANUAL
*/

BOOL32 syncErrHandler
    (
    HWTX	hWtx,		/* WTX API handle */
    void *	pClientData,
    void *	errCode
    )
    {
    if (((WTX_ERROR_T)errCode == WTX_OK) ||
	((WTX_ERROR_T)errCode == WTX_ERR_LOADER_OBJ_MODULE_NOT_FOUND) || 
	((WTX_ERROR_T)errCode == WTX_ERR_SYMTBL_SYMBOL_NOT_FOUND) ||
	((WTX_ERROR_T)errCode == WTX_ERR_SYMTBL_NO_SUCH_MODULE))
	{
#ifdef DEBUG 
	logMsg ("%#x ignored\n", (WTX_ERROR_T)errCode, 0, 0, 0, 0, 0);
#endif
	}
    else if (((WTX_ERROR_T)errCode == WTX_ERR_API_NOT_CONNECTED) ||
	     ((WTX_ERROR_T)errCode == WTX_ERR_API_TOOL_DISCONNECTED) ||
	     ((WTX_ERROR_T)errCode == WTX_ERR_API_REGISTRY_UNREACHABLE) ||
	     ((WTX_ERROR_T)errCode == WTX_ERROR))
	{
	if (!symSyncKill)
	    fprintf (stderr,
		     "Fatal WTX error (%#x), synchronization stopped\n",
		     (int)errCode);
	symSyncKill = TRUE; 	/* end the task */
	}
    else if ((WTX_ERROR_T)errCode == WTX_ERR_API_REQUEST_TIMED_OUT)
	{
	fprintf (stderr, "Warning: tSymSync WTX timeout\n");
	}
    else
	{
#ifdef DEBUG
	fprintf (stderr, "Unexpected WTX error %#x during synchronization\n",
		 (WTX_ERROR_T)errCode);
        taskSuspend (0);
#endif
	}

    return (FALSE);
    }

/******************************************************************************
*
* syncSymStart - start the synchronization task
*
* This routine is the entry point that is called by target server.
* It initializes registry inet and RPC credentials and then starts <tSymSync>
* task.
* Once the synchronization task is started, it sends a request to synchronize
* the symbol tables.
*
* RETURNS: tid of <tSymSync> or WTX_ERROR.
*
* NOMANUAL
*/

STATUS syncSymStart
    (
    char *	tgtSvrName,	/* target server name */
    UINT32	registryInet,	/* registry address */
    int		pCredFlavor,	/* RPC credentials information */
    void *	pCredBase,
    UINT32	pCredLength
    )
    {
    int		tid;
    char	startSyncEvt[SYNC_TGT_EVT_LGTH];

    if (OBJ_VERIFY (sysSymTbl, symTblClassId) != OK)
	return (WTX_ERROR);

    /* store the registry internet address */
    syncRegistry = registryInet;

    /* store RPC credentials */
    authCred.oa_flavor = pCredFlavor;
    authCred.oa_base = pCredBase;
    authCred.oa_length = pCredLength;

    /*
     * Initialize hooks so that module loads and symbol additions send a
     * message in the synchronization message queue.
     */
    taskLock ();
    syncLoadRtn = (FUNCPTR) syncLoadHook;
    syncUnldRtn = (FUNCPTR) syncUnldHook;
    syncSymAddRtn = (FUNCPTR) syncSymAddHook;
    syncSymRemoveRtn = (FUNCPTR) syncSymRemoveHook;
    taskUnlock ();

    tid = taskSpawn ("tSymSync", SYNC_PRIORITY, SYNC_OPTIONS, SYNC_STACK_SIZE,
		     symSyncTaskInit, (int)tgtSvrName, 0,0,0,0,0,0,0,0,0);

    /* bring unknown target symbols to host symbol table */
    sprintf (startSyncEvt, "%s 0x1 0x1", USR_RQST_EVT);
    msgQSend (symSyncMsgQId, startSyncEvt, strlen(startSyncEvt)+1, TIMEOUT, 
	      MSG_PRI_NORMAL);
    
    return (tid);
    }

/******************************************************************************
*
* symSyncTaskInit - WTX initializations
*
* This routine initializes RPC then WTX, attaches to target server and 
* registers for concerned events.
* 
* RETURNS: OK or ERROR.
*
* NOMANUAL
*/

STATUS symSyncTaskInit
    (
    char *	tgtSvrName	/* target server name */
    )
    {
    char 		regEvtStr[50];	/* registered events */
    WTX_HANDLER_T	wtxErrHId; 	/* error handler id */

    strcpy (regEvtStr, "(OBJ_(LOAD|UNLOAD)|SYM_(ADD|REMOV))ED|SYNC_STOP");

    /*
     * initialize the synchronized group list : vxWorks symbols are always
     * synchronized (group is 1 on target side and 1 on host side).
     */
    dllInit (&symSyncLst);
    dllInit (&symSyncTmpLst);
    groupAdd (&symSyncLst, 1, 1);

    /* initialize the task access to the RPC package */
    rpcTaskInit ();

    /* initialize WTX session handle */
    if (wtxInitialize (&symSyncHWtx) != WTX_OK)
	return (ERROR);
    
    wtxErrHId = wtxErrHandlerAdd (symSyncHWtx,
				  (WTX_HANDLER_FUNC)&syncErrHandler, 0);
    if (wtxErrHId == NULL)
	{
	fprintf (stderr,
		 "Unable to initialize synchronization task. Giving up.\n");
	wtxTerminate (symSyncHWtx);
	return (ERROR);
	}

    /* attach to Target Server */
    wtxToolAttach (symSyncHWtx, tgtSvrName, SYM_SYNC_TOOL_NAME);

    /* set WTX timeout if necessary */
    if (symSyncWtxTimeout != 0)
	wtxTimeoutSet (symSyncHWtx, symSyncWtxTimeout);

    /* register for events */
    wtxRegisterForEvent (symSyncHWtx, regEvtStr);

    /* synchronize symbol tables */
    symSyncMain (symSyncHWtx);

    /* detach from target server */
    wtxToolDetach (symSyncHWtx);

    /* remove error handler */
    wtxErrHandlerRemove (symSyncHWtx, wtxErrHId);

    wtxTerminate (symSyncHWtx);
    return (OK);
    }

/******************************************************************************
*
* symSyncMain - main routine
*
* This routine waits for target or host synchronization events and does the
* corresponding job.
*
* RETURNS: OK or ERROR.
*
* NOMANUAL
*/
STATUS symSyncMain 
    (
    HWTX		hWtx
    )
    {
    WTX_EVENT_DESC *	pEvtDesc;			/* event descriptor */
    char 		tgtEvtBuffer[SYNC_TGT_EVT_LGTH];
    char 		doneEvtBuffer[SYNC_TGT_EVT_LGTH];

    symSyncKill = FALSE;

    while (!symSyncKill)
	{
	if (msgQReceive (symSyncMsgQId, tgtEvtBuffer, SYNC_TGT_EVT_LGTH,
			 TIMEOUT) != ERROR)
	    {
#ifdef DEBUG
    	    logMsg ("tgt evt: %s\n", (int)tgtEvtBuffer, 0, 0, 0, 0, 0);
#endif
	    symSyncHostUpdate (hWtx, tgtEvtBuffer, doneEvtBuffer);
	    }
	else
	    {
	    if ((pEvtDesc = wtxEventGet (hWtx)) == NULL)
		continue;	/* we got a timeout, may be better next time */
	    while (strcmp (pEvtDesc->event, "") != 0)	/* no event received */
		{
		if (strcmp (pEvtDesc->event, "SYNC_STOP") == 0)	
	    	    {
#ifdef DEBUG
		    logMsg ("stop event received\n", 0, 0, 0, 0, 0, 0);
#endif
		    symSyncKill = TRUE;
		    break;
		    }
		else			/* split event string and process it */
		    {
#ifdef DEBUG 
		    logMsg ("tgtsvr evt: %s\n", (int)pEvtDesc->event,0,0,0,0,0);
#endif
		    symSyncTgtUpdate (hWtx, pEvtDesc->event, doneEvtBuffer);
		    wtxEventAdd (hWtx, doneEvtBuffer, 0, NULL);
		    }
		wtxResultFree (hWtx, pEvtDesc);
		if ((pEvtDesc = wtxEventGet (hWtx)) == NULL)
		    break;	/* we got a timeout, giving up */
		}
	    if (pEvtDesc != NULL)
		wtxResultFree (hWtx, pEvtDesc);
	    }
	}
    return (OK);
    }

/******************************************************************************
*
* groupAdd - add a group to a group list.
*
* RETURNS: OK if addition is possible, ERROR otherwise.
*
* NOMANUAL
*/

STATUS groupAdd
    (
    DL_LIST *	pList,
    UINT16	tgtGroup,
    UINT32	hostGroup
    )
    {
    GROUP_NODE *	pToAdd = (GROUP_NODE *) malloc (sizeof (GROUP_NODE));

    if (pToAdd)
	{
	pToAdd->tgtGroup = tgtGroup;
	pToAdd->hostGroup = hostGroup;
	dllAdd (pList, (DL_NODE *)pToAdd);
	return (OK);
	}
    return (ERROR);
    }

/******************************************************************************
*
* groupFind - find if a given group is in a group list
*
* This routine is used to know if a module is already synchronized. If its
* target or host module's group is in the synchronized group list there is
* nothing to do.
*
* RETURNS: a pointer on the GROUP_NODE found or NULL.
*
* NOMANUAL
*/

GROUP_NODE * groupFind
    (
    DL_LIST *	pList,
    UINT32	group,		/* module's group on host or target side */
    FUNCPTR	findRtn		
    )
    {
    GROUP_NODE *	pGroup;

    pGroup = (GROUP_NODE *) dllEach (pList, findRtn, (int)group);
    return (pGroup);
    }

/******************************************************************************
*
* groupRemove - remove a group from a list
*
* RETURNS: a pointer on the group node removed or NULL.
*
* NOMANUAL
*/

GROUP_NODE * groupRemove
    (
    DL_LIST *	pList,
    UINT32	group,
    FUNCPTR	findRtn
    )
    {
    GROUP_NODE *	pGroup = groupFind (pList, group, findRtn);

    if (pGroup)
	dllRemove (pList, (DL_NODE *)pGroup);
    return (pGroup);
    }

/******************************************************************************
*
* syncGroupUpdate - update <symSyncLst> with elements of <symSyncTmpLst>
*
* <symSyncTmpLst> is used as a buffer to indicate when we are bringing
* target symbols to host, that target module info were already synchronized
* while symbols are not. When every symbols are synchronized then modules 
* of <symSyncTmpLst> can be considered as synchronized.
*
* RETURNS: N/A.
*
* NOMANUAL
*/

void syncGroupUpdate ()
    {
    GROUP_NODE *	pToAdd = (GROUP_NODE *) dllGet (&symSyncTmpLst);

    while (pToAdd != NULL)
	{
	if (groupFind (&symSyncLst, pToAdd->tgtGroup, 
			(FUNCPTR)isInTgtGroupList) == NULL)
	    dllAdd (&symSyncLst, (DL_NODE *)pToAdd);
	else 
	    free (pToAdd);
	pToAdd = (GROUP_NODE *) DLL_NEXT (pToAdd);
	}
    }

/******************************************************************************
*
* isInHostGroupList - check a given host group is in the list.
*
* RETURNS: TRUE if we should examine next element, FALSE otherwise.
*
* NOMANUAL
*/

BOOL isInHostGroupList
    (
    DL_NODE *	pNode,
    int		arg
    )
    {
    if (((GROUP_NODE *)pNode)->hostGroup == (UINT32)arg)
	return (FALSE);
    return (TRUE);
    }

/******************************************************************************
*
* isInTgtGroupList - check a given target group is in the list.
*
* RETURNS: TRUE if we should examine next element, FALSE otherwise.
*
* NOMANUAL
*/

BOOL isInTgtGroupList
    (
    DL_NODE *	pNode,
    int		arg
    )
    {
    if (((GROUP_NODE *)pNode)->tgtGroup == arg)
	return (FALSE);
    return (TRUE);
    }

/* HOST ----> TARGET */

/******************************************************************************
*
* symSyncTgtUpdate - update target symbol table
*
* This routine parses and dispatches the target events received.
*
* RETURNS: OK or ERROR.
*
* NOMANUAL
*/

STATUS symSyncTgtUpdate
    (
    HWTX	hWtx,		/* WTX API handle */
    char *	evtString,	/* event received */
    char *	doneEvt 	/* event sent when sync is done */
    )
    {
    char *		evtType;
    char *		info[3];
    char *		pLast = NULL;		
    STATUS		status = OK;

    evtType = strtok_r (evtString, " ", &pLast);
    if (strcmp (evtType, OBJ_LOADED_EVT) == 0)
	{
	/* get the host module Id from the event string */

	info[0] = strtok_r (NULL, " ", &pLast);	/* module Id */
	info[1] = strtok_r (NULL, " ", &pLast);	/* module name */

	status = syncModCTgtUpdate (hWtx, H_STR_TO_L(info[0]));
	}

    else if (strcmp (evtType, OBJ_UNLOADED_EVT) == 0)
	{
	/* get the host module group from the event string */

	info[0] = strtok_r (NULL, " ", &pLast); /* module id */
	info[1] = strtok_r (NULL, " ", &pLast); /* module name */
	info[2] = strtok_r (NULL, " ", &pLast); /* module group */

	status = syncModDTgtUpdate (hWtx, H_STR_TO_L(info[2]));
	}

    else if (strcmp (evtType, SYM_ADDED_EVT) == 0)
	{
	info[1] = strtok_r (NULL, " ", &pLast);		/* symbol name */

	status = syncSymAddTgtUpdate (hWtx, info[1]);
	}
    else if (strcmp (evtType, SYM_REMOVED_EVT) == 0)
	{
	info[1] = strtok_r (NULL, " ", &pLast);		/* symbol name */
	info[0] = strtok_r (NULL, " ", &pLast);		/* symbol type */

	status = symSRemove (sysSymTbl, info[1], H_STR_TO_L(info[0]));
	}
    else 
	return (OK);

    if (strlen (info[1]) > OBJ_NAME_MAX)
	info[1][OBJ_NAME_MAX - 1] = '\0';
    sprintf (doneEvt, "SYNC_DONE t %s %s %d", evtType, info[1], status);
    return (status);
    }

/******************************************************************************
*
* syncModCTgtUpdate - update target info when a module is created on host
*
* RETURNS: OK or ERROR
*
* NOMANUAL
*/

STATUS syncModCTgtUpdate
    (
    HWTX	hWtx,		/* WTX API handle */
    UINT32	id		/* host module id */
    )
    {
    WTX_MODULE_INFO *	pModInfo;	/* host module informations */
    MODULE_ID		modId;		/* target module Id */
    WTX_SYM_LIST *	pSymList;	/* symbols of the module */
    WTX_SYMBOL	*	pSymInfo;
    STATUS		status = OK;

    /* Get the module info from the host and update target side */

    if ((pModInfo = wtxObjModuleInfoGet (hWtx, id)) == NULL)
	return (ERROR);

    /* check if the module was previously synchronized */
    if (groupFind (&symSyncLst, pModInfo->group, isInHostGroupList) != NULL)
	{
	wtxResultFree (hWtx, pModInfo);
	return (OK);
	}
    
    /* create the new target module */
    if ((modId = syncTgtModInfoFill (pModInfo)) == NULL)
	{
	wtxResultFree (hWtx, pModInfo);
	return (ERROR);
	}
	
    /* get symbols from the tgtsvr and add them in target symTbl */
    if ((pSymList = wtxSymListGet (hWtx, NULL, pModInfo->moduleName,
				   0, FALSE)) == NULL)
	{
	/* the module no longer exists on target */

	moduleDelete (modId);
	wtxResultFree (hWtx, pModInfo);
	return (ERROR);
	}

    pSymInfo = pSymList->pSymbol;
    while ((pSymInfo != NULL) && (status == OK))
	{
	status = symSAdd (sysSymTbl, pSymInfo->name, (char *)pSymInfo->value,
			  pSymInfo->type, modId->group);
	pSymInfo = pSymInfo->next;
	}

    /* update synchronized group list */
    groupAdd (&symSyncLst, modId->group, pModInfo->group);
	
    wtxResultFree (hWtx, pSymList);
    wtxResultFree (hWtx, pModInfo);
    return (status);
    }

/******************************************************************************
*
* syncModDTgtUpdate - update target info when a module is deleted on host
*
* RETURNS: OK or ERROR
*
* NOMANUAL
*/

STATUS syncModDTgtUpdate
    (
    HWTX	hWtx, 		/* WTX API handler */
    UINT32	group		/* group (on host) of the deleted module */
    )
    {
    STATUS	status = OK;

    MODULE_ID		modId;		/* target module Id */
    GROUP_NODE *	pGroup;		/* group node of the module */

    /* remove the module from the synchronized group list */
	
    if ((pGroup = groupRemove (&symSyncLst, group, 
			       (FUNCPTR)isInHostGroupList)) == NULL)
	return (ERROR);

    /* unload the module on target side */

    if ((modId = moduleFindByGroup (pGroup->tgtGroup)) != NULL)
	status = unldByModuleId (modId, UNLD_SYNC);

    free (pGroup);

#ifdef DEBUG
    if (status == ERROR)
	fprintf (stderr, "unload pb\n");
#endif
    return (status);
    }

/******************************************************************************
*
* syncSymAddTgtUpdate - update target symbol table when a symbol is added
*
* RETURNS: OK or ERROR
*
* NOMANUAL
*/

STATUS syncSymAddTgtUpdate
    (
    HWTX	hWtx,		/* WTX API handle */
    char *	symName		/* symbol name */
    )
    {
    GROUP_NODE *	pGroup;		/* group node of the module */
    WTX_SYMBOL *	pSymInfo;
    UINT16 		tgtGroup;
    STATUS 		status = OK;

    if ((pSymInfo = wtxSymFind (hWtx, symName, 0, TRUE, 0, 0)) == NULL)
	return (ERROR);
	
    /* 
     * get the corresponding target group 
     * the default group number (0) is the same on host and target
     */

    if ((tgtGroup = pSymInfo->group) != 0)
	{
	if ((pGroup = groupFind (&symSyncLst, pSymInfo->group,
				 (FUNCPTR)isInHostGroupList)) == NULL)
	    /* XXX ELP inexistant group set to 0 */
	    tgtGroup = 0;	
	tgtGroup = pGroup->tgtGroup;
	}

    status = symSAdd (sysSymTbl, pSymInfo->name, (char *)pSymInfo->value,
		      pSymInfo->type, tgtGroup);

    wtxResultFree (hWtx, pSymInfo);
    return (status);
    }

/******************************************************************************
*
* syncTgtModInfoFill - fill target module information
*
* From a host module descriptor we get the information to store on the target
* side.
*
* note: flag LOAD_LOADED_BY_TGTSVR is set in order we know that if the
*	target-server restarts these modules become obsolete.
* note: To prevent from displaying 0xffffffff on target moduleShow(), segment
*	address is set to 0 when the segment does not exist.
* note: The segment flag SEG_FREE_MEMORY is removed to prevent the memory from
*	being freed twice when unloading (1 by host and 1 by target).
*
* RETURNS: target module Id or NULL.
*
* NOMANUAL
*/

MODULE_ID syncTgtModInfoFill
    (
    WTX_MODULE_INFO *   pModInfo
    )
    {
    MODULE_ID	modId;
    int		format = -1;
    int		flags = 0;
    int		segType;
    int		segAddr;
    int		ix;
    STATUS	status = OK;

    /* tarnslate host format into target format */
    
    if (strcmp (pModInfo->format, "a.out") == 0)
	format = MODULE_A_OUT;
    else if (strcmp (pModInfo->format, "coff") == 0)
	format = MODULE_ECOFF;
    else if (strcmp (pModInfo->format, "elf") == 0)
	format = MODULE_ELF;

    /* translate host load flags into target host flags */

    if (pModInfo->loadFlag & 0x1)
	flags |= LOAD_NO_SYMBOLS;
    if (pModInfo->loadFlag & 0x2)
	flags |= LOAD_LOCAL_SYMBOLS;
    if (pModInfo->loadFlag & 0x4)
	flags |= LOAD_GLOBAL_SYMBOLS;
    flags |= (pModInfo->loadFlag & 0x00fffff0);
    flags |= LOAD_LOADED_BY_TGTSVR;

    /* create the module on the target */

    if ((modId = moduleCreate (pModInfo->moduleName, format, flags)) == NULL)
	return (NULL);
	
    /* add the segments */
    
    for (ix = 0; (ix < pModInfo->nSegments) && (status == OK); ix++)
	{
	switch (ix)
	    {
	    case 0: segType = SEGMENT_TEXT; break;
	    case 1: segType = SEGMENT_DATA; break; 
	    case 2: segType = SEGMENT_BSS; break;
#ifdef INCLUDE_SDA
	    case 3: segType = SEGMENT_SDATA; break;
	    case 4: segType = SEGMENT_SBSS; break;
	    case 5: segType = SEGMENT_SDATA2; break;
	    case 6: segType = SEGMENT_SBSS2; break;
#endif
	    default: segType = -1; break;
	    }
			
	segAddr = ((int)pModInfo->segment[ix].addr != -1) ?
		  (int)pModInfo->segment[ix].addr : 0;
	status = moduleSegAdd (modId, segType, (void *)segAddr,
			       pModInfo->segment[ix].length,
		      (pModInfo->segment[ix].flags & ~SEG_FREE_MEMORY));
	}

    if (status == OK)
	return (modId);
    else 
	{
	moduleDelete (modId);
	return (NULL);
	}
    }

/* TARGET ----> HOST  */

/******************************************************************************
*
* symSyncHostUpdate -  update host side depending on <evtString>
*
* This routine dispatch the WTX events received from the target server
*
* RETURNS:
*
* NOMANUAL
*/

STATUS symSyncHostUpdate
    (
    HWTX	hWtx,		/* WTX API handle */
    char *	evtString,	/* event received */
    char *	doneEvt		/* event sent when sync is done */
    )
    {
    char *	pLast = NULL;
    STATUS	status = OK;
    char *	evtType;	/* synchronization event type */
    char *	info[4];	/* store event tokens */
    BOOL	eventSend = TRUE;

    evtType = strtok_r (evtString, " ", &pLast);
    if (strcmp (evtType, OBJ_LOADED_EVT) == 0)
	{
	info[1] = strtok_r (NULL, " ", &pLast); 	/* module id */
	info[0] = strtok_r (NULL, " ", &pLast); 	/* module name */

	status = syncModCHostUpdate (hWtx, &symSyncLst,
				     (MODULE_ID)H_STR_TO_L(info[1]),
				     &eventSend, TRUE);
	}
    else if (strcmp (evtType, OBJ_UNLOADED_EVT) == 0)
	{
	info[0] = strtok_r (NULL, " ", &pLast);	/* mod name */
	info[1] = strtok_r (NULL, " ", &pLast);	/* mod group */

	status = syncModDHostUpdate (hWtx, info[0], H_STR_TO_L(info[1]));
	}
    else if (strcmp (evtType, SYM_ADDED_EVT) == 0)
	{
	info[0] = strtok_r (NULL, " ", &pLast);	/* sym name */
	info[1] = strtok_r (NULL, " ", &pLast);	/* sym val */
	info[2] = strtok_r (NULL, " ", &pLast);	/* sym type */
	info[3] = strtok_r (NULL, " ", &pLast);	/* sym group */

	status = syncSymAddHostUpdate (hWtx, info[0],
				   (char *)H_STR_TO_L(info[1]),
				   H_STR_TO_L(info[2]), H_STR_TO_L(info[3]));
	}
    else if (strcmp (evtType, SYM_REMOVED_EVT) == 0)
	{
	info[0] = strtok_r (NULL, " ", &pLast);	/* sym name */
	info[1] = strtok_r (NULL, " ", &pLast);	/* sym type */

	status = syncSymRemHostUpdate (hWtx, info[0], H_STR_TO_L(info[1]));
	}
    else if (strcmp (evtType, USR_RQST_EVT) == 0)
	{
	info[0] = "target_modules";
	info[1] = strtok_r (NULL, " ", &pLast); /* sense */
	info[2] = strtok_r (NULL, " ", &pLast);	/* starting tgtsvr? */

	status = syncUsrUpdate (hWtx, H_STR_TO_L(info[1]), H_STR_TO_L(info[2]));
	}

    else 
	{
#ifdef DEBUG
	logMsg ("unknown target event\n", 0, 0, 0, 0, 0, 0);
#endif
	return (OK);
	}

    if (eventSend)
	{
	if (strlen (info[0]) > OBJ_NAME_MAX)
	    info[0][OBJ_NAME_MAX - 1] = '\0';
	sprintf (doneEvt, "SYNC_DONE h %s %s %d", evtType, info[0], status);
	wtxEventAdd (hWtx, doneEvt, 0, NULL);
	}

    return (status);
    }

/******************************************************************************
*
* syncModCHostUpdate - update host side when a module is created on target
*
* This routine creates a module on the host with the same information than on
* the target (except group number). If <addSym> is TRUE the routine looks for
* the symbol of that module and add them in the host symbol table, otherwise
* symbols are not added here.
*
* RETURNS:
*
* NOMANUAL
*/

STATUS syncModCHostUpdate
    (
    HWTX	hWtx,		/* WTX API handle */
    DL_LIST *	pList,		/* where to add the synchronized module */
    MODULE_ID	modId,		/* target module Id */
    BOOL *	pEventSend,	/* send back an event when synchro is done */
    BOOL	addSym		/* add module's symbol or not */
    )
    {
    STATUS			status = OK;	/* synchro status */
    WTX_LD_M_FILE_DESC		in;		/* host mod creation input */
    WTX_LD_M_FILE_DESC *	pObjInfo;	/* object file info */
    WTX_MODULE_INFO *		pModInfo;	/* host module info */
    BY_GROUP_ARG_T 		modFind;
    MODULE			sMod;		/* stored module */

    /* store module information to get them safely later */
    taskLock();
    if (OBJ_VERIFY (modId, moduleClassId) == OK)
	memcpy (&sMod, modId, sizeof(MODULE));
    else 
	{
	taskUnlock ();
	return (ERROR);
	}
    taskUnlock();

    /* check if the module was previously synchronized */
    
    if (groupFind (&symSyncLst, sMod.group, isInTgtGroupList) != NULL)
	{
	/* we do not send an event to tgtsvr because no work is performed */

	*pEventSend = FALSE;
	return (OK);
	}

    /* prepare host module creation */

    if (syncHostModInfoFill (&in, &sMod, TRUE) == ERROR)
	return (ERROR);

    /* create the host module (flag LOAD_MODULE_INFO_ONLY is set) */

    if ((pObjInfo = wtxObjModuleLoad (hWtx, &in)) == NULL)
	{
	free (in.filename);
	free (in.section);
	return (ERROR);
	}
    free (in.filename);
    free (in.section);

    /* get newly created host module information */
    
    if ((pModInfo = wtxObjModuleInfoGet (hWtx, pObjInfo->moduleId)) == NULL)
	{
	wtxResultFree (hWtx, pObjInfo);
	return (ERROR);
	}
    
    /* add the module in the synchronized group list */
    
    groupAdd (pList, sMod.group, pModInfo->group);

    if (addSym)
	{
	/* add the symbols of the new module */
    /* 
     * XXX ELP there is no routine that returns a symbol list as tgtsvr does.
     * The solution is to go through the whole symTbl and add the concerned 
     * symbols one by one. Building a list and sending it once should be faster.
     */

    	modFind.hWtx = hWtx;
	modFind.tgtGroup = sMod.group;
	modFind.hostGroup = pModInfo->group;

	if (symEach (sysSymTbl, (FUNCPTR)syncSymByGrpToHost, (int)&modFind)
	    != NULL)
	    /* because of an error we did not go through the all list */
	    status = ERROR;
	}

    wtxResultFree (hWtx, pObjInfo);
    wtxResultFree (hWtx, pModInfo);

    return (status);
    }

/******************************************************************************
*
* syncModDHostUpdate - upadte host side when a module is deleted on target
*
* RETURNS:
*
* NOMANUAL
*/

STATUS syncModDHostUpdate
    (
    HWTX	hWtx,		/* WTX API handle */
    char *	modName,	/* name of the deleted module */
    UINT16	modGroup	/* group of the deleted module */
    )
    {
    GROUP_NODE *	pGroup;
    UINT32		moduleId = 0;

    /* remove the module from the synchronized group list */

    if ((pGroup = groupRemove (&symSyncLst, modGroup , isInTgtGroupList))
	== NULL)
	return (OK);
    free (pGroup);

    /* check to see if the module should be unloaded on host side */

    moduleId = wtxObjModuleFindId (hWtx, modName);

    if (moduleId)
	wtxObjModuleUnload (hWtx, moduleId);
    
    return (OK);
    }

/******************************************************************************
*
* syncSymAddHostUpdate - update host symTbl when a symbol is added on target
*
* RETURNS: 
*
* NOMANUAL
*/

STATUS syncSymAddHostUpdate
    (
    HWTX	hWtx,		/* WTX API handle */
    char *	name,		/* symbol name */
    char *	val,		/* symbol value */
    SYM_TYPE	type,		/* symbol type */
    UINT16	group		/* symbol group */
    )
    {
    GROUP_NODE *	pGroup;
    UINT32		hGroup;
    MODULE_ID		modId;
    STATUS		status = OK;
    BOOL 		eventSend = TRUE;
    char 		buf[SYNC_TGT_EVT_LGTH];
    char 		doneEvt[SYNC_TGT_EVT_LGTH];

    /* find the corresponding host group */

    if (group == symGroupDefault)
	hGroup = 0; 	
    else if ((pGroup = groupFind (&symSyncLst, group,
				  (FUNCPTR)isInTgtGroupList)) != NULL)
	hGroup = pGroup->hostGroup;
    else
	{
	/* symbol's group is unknown on host side */

	if ((modId = moduleFindByGroup (group)) == NULL)
	    hGroup = 0;		/* XXX ELP */
	else 
	    {
	    /*
	     * it seems very hard to come here but... we answer we failed
	     * and try to load the unsynchronized module.
	     */
	     
	    memset (buf, 0, SYNC_TGT_EVT_LGTH);
	    taskLock();
	    if (OBJ_VERIFY (modId, moduleClassId) == OK)
		{
		strncpy (buf, modId->name, SYNC_TGT_EVT_LGTH - 1);
		buf[SYNC_TGT_EVT_LGTH - 1] = EOS;
		}
	    taskUnlock();

	    status = syncModCHostUpdate (hWtx, &symSyncLst, modId, &eventSend,
					 FALSE);
	    if (eventSend)
		{
		if (strlen (buf) > OBJ_NAME_MAX)
		    buf [OBJ_NAME_MAX - 1] = '\0';
		sprintf (doneEvt, "SYNC_DONE h OBJ_LOADED %s %d", buf,
			 status);
		wtxEventAdd (hWtx, doneEvt, 0, NULL);
		}
	    return (status);
	    }
	}

    if (wtxSymAddWithGroup (hWtx, name, (TGT_ADDR_T)val, type, hGroup)
	== WTX_ERROR)
	status = ERROR;

    return (status);
    }

/******************************************************************************
*
* syncSymRemHostUpdate - update host symTbl when a target symbol is removed
* 
* This routine is called if the target remove a symbol.
*
* RETURNS: OK or ERROR.
*
* NOMANUAL
*/

STATUS syncSymRemHostUpdate
    (
    HWTX	hWtx,		/* WTX API handle */
    char *	symName,	/* symbol name */
    SYM_TYPE	symType		/* symbol type */
    )
    {
    STATUS		status = OK;
    if (wtxSymRemove (hWtx, symName, symType) == WTX_ERROR)
	status = ERROR;

    return (status);
    }

/******************************************************************************
*
* syncUsrUpdate - update target from host and host from target
*
* This routine is called when the user asks for it.
* if <sense> is 1 then synchronization is done from target to host.
*
* the following is unused
* if <sense> is 2 then synchronization is done from host to target. 
* if <sense> is 0 then both synchronization are performed. 
* <toolId> is used to identify the tool that requested for synchronization
*
* note: host symbols that don't belong to a module are not synchronized.
*
* RETURNS: OK or ERROR.
*
* NOMANUAL
*/

STATUS syncUsrUpdate
    (
    HWTX	hWtx,		/* WTX API handle */
    int		sense,		/* synchronization sense */
    int		start		/* TRUE if the target server is starting */
    )
    {
    WTX_MODULE_LIST *	pModList;
    STATUS		status = OK;
    int			ix;
    char		evtBuf[60];

    if ((sense == 0) || (sense == 1))
    	{
	/*
	 * Check every modules that will be synchronized are target modules.
	 * Otherwise the target server may have restarted, thus it considers
	 * segments are no more allocated!
	 */
	if (start)
	    {
	    moduleEach (syncTgtSafeModCheck, (int) hWtx);
	    }

	/* process each symbol of the target symbol table */

	if (OBJ_VERIFY (sysSymTbl, symTblClassId) == OK)
	    {
	    if (symEach (sysSymTbl, (FUNCPTR)syncEachSymToHost,
			 (int)hWtx) != NULL)
		return (ERROR);

	    /* add modules that was not yet in the sync group list */
	    syncGroupUpdate ();
	    }
	}

    if ((sense == 0) || (sense == 2))
	{
	pModList = wtxObjModuleList (hWtx);
	for (ix = 0;
	     (pModList != NULL) && (status == OK) && (ix < pModList->numObjMod);
	     ix++)
	    status = syncModCTgtUpdate (hWtx, pModList->modIdList[ix]);
	wtxResultFree (hWtx, pModList);

	sprintf (evtBuf, "SYNC_DONE t USR_RQST target_modules %d", status);
	wtxEventAdd (hWtx, evtBuf, 0, NULL);
	}

    return (status);
    }

/******************************************************************************
*
* syncTgtSafeModCheck - check if a target module can be safely used
*
* If synchronization is running, modules loaded by the target-server are added
* to the target side. Then, if the target-server restarts (without rebooting)
* these modules seem to be loaded on target side while memory is no more 
* allocated. This routine unloads such modules (it is not possible to
* automatically reload them because we do not have the path of the file). 
*
* RETURNS: TRUE to go through the whole module list, FALSE if an error occured.
*
* NOMANUAL
*/

BOOL syncTgtSafeModCheck
    (
    MODULE_ID	moduleId,	
    int		arg			/* WTX API handle */
    )
    {
    /*
     * if the module is loaded by target server and it is no more in
     * synchronization list then it is a "zombie" module.
     */

    if ((moduleId->flags & LOAD_LOADED_BY_TGTSVR) &&
	(groupFind (&symSyncLst, moduleId->group, isInTgtGroupList) == NULL))
	unldByModuleId (moduleId, UNLD_SYNC);

    return (TRUE);
    }

/******************************************************************************
*
* syncEachSymToHost - update the host symbol table if needed
*
* This routine examine a symbol to know if it should be added in host symTbl.
*
* RETURNS: TRUE if we should go to next symbol, FALSE otherwise.
*
* NOMANUAL
*/

BOOL syncEachSymToHost
    (
    char *      name,   /* entry name */
    int         val,    /* value associated with entry */
    SYM_TYPE    type,   /* entry type */
    int         arg,    /* wtx API handle */
    UINT16      group   /* group number (on target side) */
    )
    {
    HWTX                        hWtx = (HWTX)arg;	/* wtx API handle */
    MODULE_ID                   modId;			/* target module Id */
    GROUP_NODE *                pGroup;
    UINT32			hGroup;
    WTX_SYMBOL *		pSymbol;
    BOOL			dummy;

    if (groupFind (&symSyncLst, group, (FUNCPTR)isInTgtGroupList) != NULL)
	/* this module is already synchronized */
	return TRUE;

    if (group == symGroupDefault)
	{
	/* XXX ELP, 2 possibilities 
	 * 1. we do not synchronize these symbols in forced mode since we do
	 *    not know if it was already done before. 
	 * 2. we examine host symbol table to see if the symbol is already
	 *    existing. if so we return, otherwise we add it.
	 * choose 2
	 */
	hGroup = 0;
	if ((pSymbol = wtxSymFind (hWtx, name, 0, TRUE, type, N_TYPE))
	    != NULL)
	    {
	    if ((pSymbol->value == val) && (pSymbol->group == hGroup))
		{
		wtxResultFree (hWtx, pSymbol);
		return (TRUE);
		}
	    wtxResultFree (hWtx, pSymbol);
	    }
	}
    else if ((pGroup = groupFind (&symSyncTmpLst, group, 
				  (FUNCPTR)isInTgtGroupList)) == NULL)
	{
	/* this module is unknown on host, we must add it */

	if ((modId = moduleFindByGroup (group)) == NULL)
	    return (FALSE);
	if (syncModCHostUpdate (hWtx, &symSyncTmpLst, modId, &dummy, FALSE)
	    == ERROR)
	    return (FALSE);
	if ((pGroup = groupFind (&symSyncTmpLst, group,
				 (FUNCPTR)isInTgtGroupList)) == NULL)
	    return (FALSE);
	hGroup = pGroup->hostGroup;
	}
    else
	hGroup = pGroup->hostGroup;

    /* add the symbol */
    
    if (wtxSymAddWithGroup (hWtx, name, (TGT_ADDR_T)val, type, hGroup)
	!= WTX_OK)
	return (FALSE);
   
    /* if it is an undefined symbol, update the corresponding module info */

    if (type == N_UNDF)
	{
	if (wtxObjModuleUndefSymAdd (hWtx, name, hGroup) != WTX_OK)
	    return (FALSE);
	}
    
    return (TRUE);
    }

/******************************************************************************
*
* syncSymByGrpToHost - update host symbol table with symbols of a given group
*
* This routine examine each symbol of a given group to know if it should be
* added in host symTbl.
*
* RETURNS: TRUE if we should examine the next symbol, FALSE otherwise.
*
* NOMANUAL
*/

BOOL syncSymByGrpToHost
    (
    char *      name,   /* entry name */
    int         val,    /* value associated with entry */
    SYM_TYPE    type,   /* entry type */
    int         arg,    /* wtx API handle, host and target groups */
    UINT16      group   /* group number (on target side) */
    )
    {
    BY_GROUP_ARG_T *	pArg = (BY_GROUP_ARG_T *)arg;

    if (group != pArg->tgtGroup)
	return (TRUE);

    if (wtxSymAddWithGroup (pArg->hWtx, name, (TGT_ADDR_T)val, type,
			    pArg->hostGroup) == WTX_ERROR)
	return (FALSE);

    /* if it is an undefined symbol, update the corresponding module info */

    if (type == N_UNDF)
	{
	if (wtxObjModuleUndefSymAdd (pArg->hWtx, name, pArg->hostGroup)
	    == WTX_ERROR)
	    return (FALSE);
	}
    
    return (TRUE);
    }

/******************************************************************************
*
* syncHostModInfoFill - fill a host module descriptor from a target MODULE_ID
* 
* This routine creates a module on host side with the information provided by 
* the target module identifier.
* note:	if <synchro> is TRUE, loadFlag LOAD_MODULE_INFO_ONLY is set to indicate
*	that the module is already loaded so wtxObjModuleLoad() will not do it
*	again.
* note:	segment flag is always unset to prevent segment memory from being freed
*	twice when unloading module (1 on target and 1 on host).
*
* RETURNS: OK or ERROR
*
* NOMANUAL
*/

STATUS syncHostModInfoFill
    (
    WTX_LD_M_FILE_DESC *	pFileDesc,
    MODULE_ID			modId,
    BOOL			synchro
    )
    {
    char *		fileNameBuf = NULL;
    SEGMENT_ID		segId;
    int			ix;

    memset (pFileDesc, 0, sizeof(WTX_LD_M_FILE_DESC));
    
    if ((fileNameBuf = malloc (PATH_MAX + NAME_MAX)) == NULL)
	return (ERROR);

    sprintf (fileNameBuf, "%s/%s", modId->path, modId->name);

    pFileDesc->filename = fileNameBuf;
    pFileDesc->moduleId = 0;

    /*
     * undefined symbol list is initialized empty at first,
     * if undefined symbol are found when going through symbol table they are
     * added in this list
     */
    
    pFileDesc->undefSymList.pSymbol = NULL;

    /* translate target module flags into host module flag */

    if (modId->flags & LOAD_NO_SYMBOLS)
	pFileDesc->loadFlag |= 0x1;
    if (modId->flags & LOAD_LOCAL_SYMBOLS)
	pFileDesc->loadFlag |= 0x2;
    if (modId->flags & LOAD_GLOBAL_SYMBOLS)
	pFileDesc->loadFlag |= 0x4;
    pFileDesc->loadFlag |= (modId->flags & HIDDEN_MODULE);

    if (!synchro)
	{
	/* we want to load the file, letting target-server decide where */

	pFileDesc->nSections = 0;
	return (OK);
	}

    pFileDesc->loadFlag |= LOAD_MODULE_INFO_ONLY;
    pFileDesc->nSections = dllCount (&modId->segmentList);
    if ((pFileDesc->section = (LD_M_SECTION *)
	 malloc (pFileDesc->nSections * sizeof (LD_M_SECTION))) == NULL)
	{
	free (fileNameBuf);
	return (ERROR);
	}

    for (segId = (SEGMENT *)DLL_FIRST (&modId->segmentList);
	 segId != NULL; segId = (SEGMENT *)DLL_NEXT (segId))
	{
	switch (segId->type)
	    {
	    case SEGMENT_TEXT: ix = 0; break;
	    case SEGMENT_DATA: ix = 1; break;
	    case SEGMENT_BSS: ix = 2; break;
#ifdef INCLUDE_SDA
	    /*
	     * XXX ELP every segments are put in the list even if SDA segments
	     * are currently useless. (it will be necessary to add the
	     * memPartId info in the LD_M_SECTION structure)
	     */
	    case SEGMENT_SDATA: ix = 3; break;
	    case SEGMENT_SBSS: ix = 4; break;
	    case SEGMENT_SDATA2: ix = 5; break;
	    case SEGMENT_SBSS2: ix = 6; break;
#endif /* INCLUDE_SDA */
	    default: ix = -1; break;
	    }
	if (ix < 0)
	    continue;
	pFileDesc->section[ix].flags = segId->flags & ~SEG_FREE_MEMORY;
	pFileDesc->section[ix].addr = (segId->size == 0) ? (TGT_ADDR_T) -1 :
				      (TGT_ADDR_T)segId->address;
	pFileDesc->section[ix].length = segId->size;
	}

    return (OK);
    }

/******************************************************************************
*
* syncLoadHook - load module hook
*
* RETURNS: N/A
* 
* NOMANUAL
*/

void syncLoadHook
    (
    MODULE_ID modId
    )
    {
    char	buf[SYNC_TGT_EVT_LGTH];
    char 	name[SYNC_TGT_EVT_LGTH - 25];

    strncpy (name, modId->name, SYNC_TGT_EVT_LGTH - 26);
    name[SYNC_TGT_EVT_LGTH - 26] = EOS;
    sprintf (buf, "%s %#x %s", OBJ_LOADED_EVT, (UINT32)modId, name);
    if (msgQSend (symSyncMsgQId, buf, SYNC_TGT_EVT_LGTH, WAIT_FOREVER, 0)
	== ERROR)
	{
#ifdef DEBUG
	logMsg ("module 0x%x sync failed\n", (int)modId, 0,0,0,0,0);
#endif
	return;
	}
    return;
    }

/******************************************************************************
*
* syncUnldHook - unload module hook
*
* RETURNS: N/A
* 
* NOMANUAL
*/

void syncUnldHook
    (
    MODULE_ID	modId
    )
    {
    char	buf[SYNC_TGT_EVT_LGTH];
    char 	name[SYNC_TGT_EVT_LGTH - 25];

    strncpy (name, modId->name, SYNC_TGT_EVT_LGTH - 26);
    name[SYNC_TGT_EVT_LGTH - 26] = EOS;

    sprintf (buf, "%s %s %#x", OBJ_UNLOADED_EVT, name, modId->group);

    if (msgQSend (symSyncMsgQId, buf, SYNC_TGT_EVT_LGTH, WAIT_FOREVER, 0)
	== ERROR)
	{
#ifdef DEBUG
	logMsg ("module 0x%x deletion sync failed\n", (int)modId,
			0, 0, 0, 0, 0);
#endif
	return ;
	}
    }

/******************************************************************************
*
* syncSymAddHook - symbol addition hook
*
* RETURNS: N/A
*
* NOMANUAL
*/

void syncSymAddHook
    (
    char *	name,
    char *	value,
    SYM_TYPE	type,
    UINT16	group
    )
    {
    char	buf[SYNC_TGT_EVT_LGTH];
    char 	symName[SYNC_TGT_EVT_LGTH - SYM_ADDED_EVT_MSG_NON_NAME_LGTH];

    if (group == 1)		/* symbol table group */
	return ;

    strncpy (symName, name, 
	     SYNC_TGT_EVT_LGTH - SYM_ADDED_EVT_MSG_NON_NAME_LGTH - 1);
    symName[SYNC_TGT_EVT_LGTH - SYM_ADDED_EVT_MSG_NON_NAME_LGTH -1] = EOS;
    
    /*
     * The message for SYM_ADDED event will look as follows:
     *
     *  "SYM_ADDED <name> 0x<value> 0x<type> 0x<group>\0"
     *   |   9   | |    | |  10   | |  4   | |   6   |
     *
     * <value>: char*, takes 8 bytes at maximum in the output of sprintf;
     * <type> : SYM_TYPE, takes 2 bytes at maximum in the output of sprintf;
     * <group>: UINT16, takes 4 bytes at maximum in the output of sprintf;
     *
     * Totally, the non-name information(including white spaces and EOS) takes
     * at maximum 9+1+1+10+1+4+1+6+1 = 34. So define macro
     * SYM_ADDED_EVT_MSG_NON_NAME_LGTH to 34
     */

    sprintf (buf, "%s %s 0x%x 0x%x 0x%x", SYM_ADDED_EVT, symName, (int)value,
	     type, group);
    if (msgQSend (symSyncMsgQId, buf, SYNC_TGT_EVT_LGTH, WAIT_FOREVER, 0)
	== ERROR)
	{
#ifdef DEBUG
	logMsg ("sym %s addition sync failed\n", (int)name, 0, 0, 0, 0, 0);
#endif
	return ;
	}
    }

/******************************************************************************
*
* syncSymRemoveHook - symbol removing hook
*
* RETURNS: N/A
*
* NOMANUAL
*/

void syncSymRemoveHook
    (
    char *	name,
    SYM_TYPE	type
    )
    {
    /* SYM_ADDED_EVT_MSG_NON_NAME_LGTH is used to calculate space for
     * symbol name so syncSymRemoveHook and syncSymAdd Hook agree on
     * acceptable length of symbol name
     */

    char	buf[SYNC_TGT_EVT_LGTH];
    char 	symName[SYNC_TGT_EVT_LGTH - SYM_ADDED_EVT_MSG_NON_NAME_LGTH];

    strncpy (symName, name, 
	     SYNC_TGT_EVT_LGTH - SYM_ADDED_EVT_MSG_NON_NAME_LGTH - 1);
    symName[SYNC_TGT_EVT_LGTH - SYM_ADDED_EVT_MSG_NON_NAME_LGTH -1] = EOS;

    sprintf (buf, "%s %s 0x%x", SYM_REMOVED_EVT, symName, type);
    if (msgQSend (symSyncMsgQId, buf, SYNC_TGT_EVT_LGTH, WAIT_FOREVER, 0)
	== ERROR)
	{
#ifdef DEBUG
	logMsg ("sym %s removing sync failed\n", (int)name, 0, 0, 0, 0, 0);
#endif
	return ;
	}
    }

/*******************************************************************************
*
* symSRemove - remove a symbol from a symbol table
*
* This routine behaves like symRemove() except it does not test syncRemoveRtn
* function pointer. Thus it enables to "silently" remove a symbol.
*
* RETURNS: OK, or ERROR if the symbol is not found
* or could not be deallocated.
*
* NOMANUAL
*/

STATUS symSRemove
    (
    SYMTAB_ID symTblId,         /* symbol tbl to remove symbol from */
    char      *name,            /* name of symbol to remove */
    SYM_TYPE  type              /* type of symbol to remove */
    )
    {
    SYMBOL *pSymbol;

    if (symFindSymbol (symTblId, name, 0, type, SYM_MASK_ALL, 
		       (SYMBOL_ID *) &pSymbol) != OK)
	return (ERROR);

    /* XXX JLN symTblRemove should really take a SYMBOL_ID */

    if (symTblRemove (symTblId, pSymbol) != OK)
	return (ERROR);
    return (symFree (symTblId, pSymbol));
    }

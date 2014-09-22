/* wtx.c - WTX C library */

/* Copyright 1994-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
06k,08mar02,c_c  Reworked license management.
06j,04mar02,fle  SPR#73615 fix : wtxResultFree () may free already freed
                 memory
06i,17dec01,c_c  Fixed LM file descriptor leak problem.
06h,04dec01,sn   Added wtxDefaultDemanglerStyle
06g,05dec01,fle  added Object directory retirving to wtxCpuInfoGet ()
06f,18oct01,c_c  Updated feature name in license check.
06e,11oct01,c_c  Added license bypass.
06d,03oct01,c_c  Added license check. Added wtxTargetToolNameGet API.
06c,28sep01,fle  Fixed wtxResultFree () to not return WTX_ERROR
06b,15jun01,pch  Add new WTX_TS_INFO_GET_V2 service to handle WTX_RT_INFO
                 change (from BOOL32 hasFpp to UINT32 hasCoprocessor) without
                 breaking existing clients.
06a,30may01,kab  Fixed wtxTgtHasDsp comment
05z,09may01,dtr  Changing wtxTgtHasFpp , altivec for new format introduced by
		 hasCoprocessor.  Also adding in wtxTgtHasDsp.
05y,21mar01,dtr  Updating comments in code.
05x,06feb01,dtr  Adding check for altivec support.
05w,18jan01,dtr  Adding altivec register support.
05v,24nov99,fle  SPR#28684 fix : made wtxToolAttach() update the target server
		 key if the first adapter is not the good one
05u,22jun99,fle  made wtx compile on target side
05t,02apr99,fle  fixed a problem with wtxServiceAdd that was always allocating
		 wdb service description even if all the wdb parameters were
		 set to NULL  SPR 67367
05s,31mar99,fle  doc : specified that wtxFileOpen does not support RDONLY flag
		 under WIN32
05r,23feb99,c_c  Implemented an API to get the target IP address (SPR #25044).
05q,11feb99,c_c  Authorize accesses to target's address 0 (SPR #25049).
05p,18jan99,c_c  documented WTX_CONSOLE_LINES setting (SPR 6209).
05o,08jan99,fle  doc : added REGISTRY_NAME_CLASH to wtxRegister possible
		 errors
05n,01dec98,pcn  SPR #22867: complete the fix: in wtxObjModuleLoad(), copy the
		 address fields of the input parameter in the protocol input
		 value.
05m,30nov98,pcn  Moved a badly placed #ifdef HOST for the symbol table
		 synchronization (SPR #22867).
05l,05nov98,fle  doc : made wtxLogging been linked correctly
05k,02nov98,c_c  Modified WTX_ERR_SVR_EINVAL error explanation for Load
		 requests.
05j,16sep98,fle  Modified wtxMemDisassemble doc since its new output format
05i,22sep98,l_k  Removed wtxFindExecutable.
05h,22sep98,pcn  Change the returned error in wtxObjModuleLoadProgressReport.
05g,18sep98,l_k  Implement wtxFindExecutable.
05f,18sep98,pcn  Implement wtxObjModuleInfoAndPathGet.
05e,18sep98,pcn  Removed memory leak when asynchronous load is used.
05d,04sep98,pcn  Added WTX_EVENT_EVTPT_ADDED, WTX_EVENT_EVTPT_DELETED.
05c,10sep98,pcn  Remove wtxResultFree from wtxObjModuleLoadStart and
		 wtxObjModuleLoadProgressReport.
05b,18aug98,pcn  Use WTX_MSG_EVTPT_LIST_2 and doc updated.
05a,20jul98,pcn  wtxObjModuleLoadProgressReport doc updated.
04z,20jul98,pcn  Added evtptNum and toolId in the wtxEventpointListGet return
		 list.
04y,10jul98,fle  removed wpwrLog calls in wtxCpuInfoGet function
04x,09jul98,fle  re-added wtxCommandSend
04w,08jul98,pcn  Added undefined symbols list in
		 wtxObjModuleLoadProgressReport result.
04v,06jul98,pcn  Removed wtxObjModuleUndefSymAdd from API.
04u,02jul98,pcn  Removed memory leak from wtxObjModuleLoadStart and
		 wtxObjModuleLoadProgressReport.
04t,19jun98,pcn  Added wtxObjModuleLoadStart, wtxObjModuleLoadProgressReport,
		 wtxObjModuleLoadCancel.
04s,11jun98,pcn  Added an input parameter at wtxEventListGet.
04r,09jun98,jmp  added wtxAsyncResultFree to free memory used by a
		 wtxAsyncEventGet() call result, imported wtxAsyncEventGet()
		 from wtxAsync.c.
04q,03jun98,pcn  Added 2 requests: wtxSymListByModuleNameGet and
		 wtxSymListByModuleIdGet.
04p,25may98,pcn  Changed wtxTsLock in wtxTsTimedLock, wtxEventpointList_2 in
		 wtxEventpointListGet, wtxObjModuleUnload_2 in
		 wtxObjModuleUnloadByName.
04o,20may98,jmp  modified wtxAsyncInitialize() call, now if no user defined
		 function is given then received events are just stored in the
		 asynchronous event list, those events can be get using
		 wtxAsyncEventGet().
04n,19may98,rhp  doc: corrected wtxLogging example (supplied missing .CE)
04m,07may98,pcn	 Re-added WTX_TS_KILLED.
04l,30apr98,dbt  added wtxHwBreakpointAdd and wtxEventpointAdd.
04k,28apr98,pcn  Removed wtxCommandSend from C API.
04j,24apr98,pcn  Removed wtxEventGetThread.
04i,23apr98,fle  added CPU name retrieving to wtxCpuInfoGet
04h,23apr98,fle  added ifdef HOST around wtxCpuInfoGet
		 + added warning for HOST defined functions
04g,08apr98,fle  doc: updated and added examples to wtxMemDisassemble
		 + added wtxCpuInfoGet function.
04f,02apr98,pcn  WTX 2: added new error codes.
04e,31mar98,fle  made wtxToolAttach() always call for wtxInfo()
		 + made some history cleanup
04d,27mar98,pcn  Moved #ifdef HOST after functions parameters in order to
		 generate manual.
04c,26mar98,pcn  Changed strdup in strcpy.
04b,26mar98,pcn  WTX 2: Added an event filter for WTX logging. Added new
		 behavior of wtxObjModuleLoad_2: a file can be opened locally
		 or by the target server.
04a,24mar98,dbt  added wtxContextStatusGet.
03z,17mar98,pcn  WTX 2: wtxObjModuleChecksum: set filename to
		 WTX_ALL_MODULE_CHECK if moduleId is set to WTX_ALL_MODULE_ID.
03y,09mar98,pcn  WTX 2: added a test of <fileName> in wtxLogging.
03x,06mar98,pcn  WTX 2: changed the width test in wtxMemWidthRead/Write.
03w,05mar98,fle  got rid of wtxRegistryPing() routine
03v,03mar98,pcn  WTX 2: added fcntl.h for file constants.
03u,03mar98,pcn  Added #ifdef for HOST side.
03t,02mar98,pcn  WTX 2: added wtxAsyncNotifyEnable, wtxCacheTextUpdate,
		 wtxEventListGet, wtxEventpointList_2, wtxLogging,
		 wtxMemWidthRead, wtxMemWidthWrite, wtxObjModuleChecksum,
		 wtxObjModuleLoad_2, wtxObjModuleUnload_2,
		 wtxUnregisterForEvent, wtxThreadSigHandle. Changed
		 WTX_EVT_BREAKPOINT in WTX_EVT_HW_BP, wtxEventAdd,
		 wtxResultFree, wtxObjModuleUnload, wtxEventToStrType,
		 wtxTargetRtTypeGet, wtxTargetCpuTypeGet.
03s,29jan98,fle  made wtxToolAttach return hWtx->errCode when unable to attch
		 tool (due to implementation of wtxregdSvrPing())
		 + added wtxRegistryPing()
03r,28jan98,c_c  Packed all wtxEvtxxxStringGet routines into one.
03q,29aug97,fle  Adding the WTX_MEM_DISASSEMBLE service
		 + Updating NOTE written after the "forward declarations"
		 + made wtxSymListGet() usable by the WTX TCL API
		 + moved tsSymListFree() from tgtsvr/server/tssymlk.c in
		   symLisFree()
03p,06dec96,dgp  doc: correct spelling
03o,21nov96,dgp  doc: change Object-Module, correct italic and bold formatting
03n,20nov96,dgp  doc: correct WTX_THROW() to WTX_THROW
03m,18nov96,dgp  doc: final changes, wtxGopherEval, wtxErrExceptionFunc, 
		      wtxSymFind
03l,12nov96,c_s  remove use of strerror on sun4-sunos4
03k,11nov96,dgp  doc: "final" formatting for API Guide
03j,30sep96,elp  put in share, adapted to be compiled on target side
		 (added wtxSymAddWithGroup() and wtxObjModuleUndefSymAdd()).
03i,19sep96,p_m  fixed syntax error in wtxMemAlign() introduced by last
		 doc modification
03h,17sep96,dgp  doc: API Guide updates, particularly wtxGopherEval,
		 wtxObjectModuleLoad
03g,05sep96,p_m  Documented wtxToolIdGet() and wtxServiceAdd()
03f,05sep96,elp	 changed val in wtxMemSet() from UINT8 to UINT32 (SPR# 6894).
03e,02sep96,jmp  added wtxToolIdGet(),
		 added WTX RPC service number argument to wtxServiceAdd().
03d,30aug96,elp	 Changed wpwrVersionGet() into wtxTsVersionGet().
03c,26jul96,pad  Changed order of include files (AIX port).
03b,26jul96,dbt  fixed a memory leak. Added serverDescFree to clean servor
		 descriptor. 
03a,15jul96,dbt  supressed call to wtxExchangeDelete() and 
		 wtxExchangeTerminate() in exchange() in case of server
		 exchange handle (SPR #6862).
01a,24dec94,jcf  written.
*/

/*
DESCRIPTION
This module implements a C-language transport-neutral interface to the 
WTX messaging protocol. 

A tool must always call wtxInitialize() to initialize a handle that is
used in all further WTX calls. The tool can then attach to a target
server using a call to wtxToolAttach(). Each WTX handle can only be
connected to one server at a time.  After a successful call of
wtxToolAttach(), the handle is considered bound to the specified target
server until a call of wtxToolDetach() is made; then 
the handle can be attached to a new target server.  When the
handle is no longer required, call wtxTerminate() to release any
internal storage used by the handle. The handle must not be used after
wtxTerminate() has been called.

.CS
#include "wtx.h"

    HWTX hWtx;

    /@ initialize WTX session handle @/

    if (wtxInitialize (&hWtx) != WTX_OK)
	return (WTX_ERROR);

    /@ attach to Target Server named "soubirou" @/

    if (wtxToolAttach (hWtx, "soubirou", "wtxApp") != WTX_OK)
	return (WTX_ERROR);

    /@ register for events we want to hear about (all events here) @/

    if (wtxRegisterForEvent (hWtx, ".*") != WTX_OK)
	{
	wtxToolDetach (hWtx);
	return (WTX_ERROR);
	}

    /@ core of the WTX application @/
		   .
		   .
		   .
		   .

    /@ detach form the Target Server @/

    wtxToolDetach (hWtx);

    /@ terminate WTX session @/

    wtxTerminate (hWtx);
.CE

Most WTX calls return either a pointer value which is NULL on error or a
STATUS value which is WTX_ERROR if an error occurs.  A descriptive string
can be obtained for the last error that occurred by calling
wtxErrMsgGet().

Note that virtually all WTX calls can fail due to an error in the message
transport layer used to carry out WTX requests.  Transport errors that
are non-recoverable result in the tool being detached from the server
and an error code of WTX_ERR_API_TOOL_DISCONNECTED. If a non-fatal
error occurs, WTX_ERR_API_REQUEST_FAILED is set, except in the case of
a timeout. In that case the error is WTX_ERR_API_REQUEST_TIMED_OUT. In
the non-fatal cases, the call may be retried. In the fatal case, retrying the
call results in the error WTX_ERR_API_NOT_CONNECTED.

All API calls attempt to check the validity of the API handle provided.
The error WTX_ERR_API_INVALID_HANDLE indicates a bad handle. Other
pointer arguments are checked and WTX_ERR_API_INVALID_ARG indicates a
bad argument value such as a NULL pointer.  API calls that require the
handle to be connected to a target server generate the
WTX_ERR_API_NOT_CONNECTED error if the handle is not connected.

In addition to simple error return values, the C API allows error
handlers to be installed on a per handle basis using
wtxErrHandlerAdd().  If an error occurs, the last installed
handler is called first. If it returns a TRUE value, then any
previously installed handlers are called in reverse order, in other words,
last installed, first called.  The C API includes macros that use the
predefined error handler wtxErrExceptionFunc() to support C++ style
exception catching.  Once an API handle has been initialized using
wtxInitiliaze(), it can be used in the WTX_TRY macro to cause API
errors to be treated like exceptions. When this is done, an API call
that would normally return an error code actually causes a jump
straight to the nearest 'catch' handler as specified using a WTX_CATCH
or WTX_CATCH_ALL macro.  The previous example is shown below using this
style of error handling.

.CS
#include "wtx.h"

    HWTX hWtx;

    /@ initialize WTX session handle @/

    if (wtxInitialize (&hWtx) != WTX_OK)
	return (WTX_ERROR);

    /@ Start a block in which errors will be "caught" by a catch block @/

    WTX_TRY (hWtx)

        {

	/@ attach to Target Server named "soubirou" @/

        wtxToolAttach (hWtx, "soubirou", "wtxApp");

	/@ register for events we want to hear about (all events here) @/

	wtxRegisterForEvent (hWtx, ".*");

	/@ core of the WTX application @/
		   .
		   .
		   .
		   .

	/@ detach form the Target Server @/

	wtxToolDetach (hWtx);

	}

    /@ Catch a specific error, WTX_ERR_API_NOT_CONNECTED @/

    WTX_CATCH (hWtx, WTX_ERR_API_NOT_CONNECTED)
	    {	
	    fprintf (stderr, "Connection lost, exiting\n");
	    wtxTerminate (hWtx);
	    exit (0);
	    }

    /@ Catch any other errors in one handler and print error message @/

    WTX_CATCH_ALL (hWtx)
	fprintf (stderr, "%s\n", wtxErrMsgGet (hWtx));

    /@ Finish the try block and resume normal error handling @/

    WTX_TRY_END (hWtx);

    /@
     * Normal error handling is now restored - the WTX_TRY_END macro
     * must be executed for this to occur.
     @/
     
    /@ wtxTerminate() will also detach the tool if already attached @/

    wtxTerminate (hWtx);
.CE

In certain circumstances, it may be useful to generate a user-defined
error or to simulate an API error from within user code. This may be
done using the WTX_THROW macro, which causes execution to jump to the
nearest handler for the error thrown or to the nearest "catch all"
handler.

Many of the C API calls are very similar to the equivalent WTX
protocol calls and the user may also refer to the appropriate section
in the \f2WTX Protocol\fP reference for further information.

INCLUDE FILES: wtx.h
*/

/*
INTERNAL
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!                                                                             !
!                                   WARNING                                   !
!                                   -------                                   !
!                                                                             !
! This file is to be compiled from both host and target side. When adding new !
! functions, please care if it is target oriented or not, and try to compile  !
! the target/src/wtx directory.                                               !
!                                                                             !
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*/

/* includes */

#include <string.h>

#ifdef HOST
#    include <stdlib.h>
#    include <stdio.h>
#    include <fcntl.h>
#    ifndef WIN32
#	include <unistd.h>
#    endif

#    ifdef SUN4_SUNOS4
	extern int              sys_nerr;
	extern char *           sys_errlist;
#    endif /* SUN4_SUNOS4 */
#ifndef  _DEBUG
#include "licmgt.h"
#endif /* _DEBUG */
#endif /* HOST */

#include "private/wtxp.h"	/* Private (and public) headers */
#include "wtxexch.h"

#if (! defined WIN32) && (defined HOST)
#    include "win32ProfileLib.h"
#endif /* WIN32 */

/*
 * These five include files are here because this implementation
 * of the WTX API is based on RPC calls.  Full implementation of the 
 * "exchange" layer would remove this transport dependency
 */

#include "wtxrpc.h"		/* For handling wtxregd RPC key routines */
#include "wtxxdr.h"		/* For WTX XDR routines */

#include <errno.h>
#include <sys/stat.h>
#include <arpa/inet.h>		/* inet_ntoa */

#ifdef HOST
#    include <netdb.h>		/* gethostbyname */
#    include "wpwrutil.h"
#    include "wtxAsync.h"
#endif

#include "sllLib.h"		/* singly linked list library */
#include "pathLib.h"


/* defines */

#define STREQ(a,b) (*(a) == *(b) ? !strcmp ((a), (b)) : 0)
#define WTX_LOAD_BITMASK	(WTX_LOAD_FROM_TARGET_SERVER | \
				 WTX_LOAD_ASYNCHRONOUSLY | \
				 WTX_LOAD_PROGRESS_REPORT)
/* typedefs */

/*
 * Special dummy message type for anonymous casting and calculating field
 * alignments in wtxResultFree() 
 */

typedef struct wtx_msg_dummy	
    {
    WTX_CORE	wtxCore;
    UINT32	field;
    } WTX_MSG_DUMMY;

typedef struct wtxFreeNode		/* wtxResultFree parameters          */
    {
    SL_NODE		node;		/* node in list                      */
    void *		pToFree;	/* pointer to free                   */
    FUNCPTR		pFreeFunc;	/* function that should free pointer */
    void *		pMsgToFree;	/* WTX message containing pToFree    */
    WTX_REQUEST		svcNum;		/* number of server service to free  */
    WTX_XID		server;		/* RPC server to connect to          */
    WTX_SVR_TYPE	svrType;	/* is it a tgtsvr or registry server */
    } WTX_FREE_NODE;

/* See wtx.h and wtxp.h */

/* locals */

static const char *	WTX_EVT_NONE		= "NONE";
static const char *	WTX_EVT_OBJ_LOADED	= "OBJ_LOADED";
static const char *	WTX_EVT_CTX_EXIT	= "CTX_EXIT";
static const char *	WTX_EVT_CTX_START	= "CTX_START";
static const char *	WTX_EVT_TEXT_ACCESS	= "TEXT_ACCESS";
static const char *	WTX_EVT_OBJ_UNLOADED	= "OBJ_UNLOADED";
static const char *	WTX_EVT_TGT_RESET	= "TGT_RESET";
static const char *	WTX_EVT_SYM_ADDED	= "SYM_ADDED";
static const char *	WTX_EVT_SYM_REMOVED	= "SYM_REMOVED";
static const char *	WTX_EVT_EXCEPTION	= "EXCEPTION";
static const char *	WTX_EVT_VIO_WRITE	= "VIO_WRITE";
static const char *	WTX_EVT_TOOL_ATTACH	= "TOOL_ATTACH";
static const char *	WTX_EVT_TOOL_DETACH	= "TOOL_DETACH";
static const char *	WTX_EVT_TOOL_MSG	= "TOOL_MSG";
static const char *	WTX_EVT_DATA_ACCESS	= "DATA_ACCESS";
static const char *	WTX_EVT_CALL_RETURN	= "CALL_RETURN";
static const char *	WTX_EVT_USER		= "USER";
static const char *	WTX_EVT_HW_BP           = "HW_BP";
static const char *	WTX_EVT_OTHER		= "OTHER";
static const char *	WTX_EVT_INVALID		= "INVALID";
static const char *	WTX_EVT_UNKNOWN		= "UNKNOWN";
static const char *	WTX_EVT_TS_KILLED	= "TS_KILLED";
static const char *	WTX_EVT_EVTPT_ADDED	= "EVTPT_ADDED";
static const char *	WTX_EVT_EVTPT_DELETED	= "EVTPT_DELETED";

#if !defined (_DEBUG) && defined (NO_LICENSE)
static BOOL envVarChecked  = FALSE;	/* do we checked NO_LICENSE env. var? */
#endif	/* NO_LICENSE */

#if defined (HOST) && !defined (_DEBUG)
static BOOL	licenseCheck   = TRUE;	/* do check for license ?             */
LOCAL int	gotLicense	= 0;	/* Do we own a license ?              */
#endif /* HOST */

/* XXX : fle : put in comment as it was not used anywhere */
/* static const char * WTX_EVT_CALL_GOT_EXC=	"CALL_GOT_EXC"; */

/* Externals */

/* globals */

static BOOL	internalLoadCall;	/* states if it is an internal load */

/* forward declarations */

LOCAL WTX_ERROR_T exchange
    (
    HWTX	hWtx,
    UINT32	svcNum,
    void *	pIn,
    void *	pOut
    );

LOCAL WTX_ERROR_T registryConnect	/* establish a connection with regd */
    (
    HWTX	hWtx			/* WTX session handler              */
    );

LOCAL void registryDisconnect
    (
    HWTX hWtx
    );

STATUS wtxFreeAdd			/* adds elt in wtxResultFree() list  */
    (
    HWTX		hWtx,		/* wtx session handler               */
    void *		pToFree,	/* pointer to be freed               */
    FUNCPTR		pFreeFunc,	/* routine to use to free pToFree    */
    void *		pMsgToFree,	/* message to free if needed         */
    WTX_REQUEST		svcNum,		/* service num to free if needed     */
    WTX_XID		server,		/* RPC server to connect to          */
    WTX_SVR_TYPE	svrType		/* connected server type             */
    );

LOCAL STATUS wtxMemDisassembleFree		/* free disassembled insts.  */
    (
    WTX_DASM_INST_LIST *	pDasmInstList	/* disassembled list to free */
    );

#if 0
/* XXX : fle : for SRP#67326 */
#ifdef HOST
LOCAL STATUS wtxObjModuleLoadProgressReportFree	/* free a status report */
    (
    HWTX			hWtx,		/* WTX session handler  */
    WTX_LOAD_REPORT_INFO *	pToFree		/* pointer to free      */
    );
#endif /* HOST */
#endif /* 0 */

LOCAL BOOL wtxResultFreeListTerminate	/* empty the wtxResultFree ptrs list */
    (
    WTX_FREE_NODE *	pWtxFreeNode,	/* element in singaly linked list    */
    HWTX *		hWtx		/* WTX session handler               */
    );

LOCAL BOOL wtxResultFreeServerListTerminate	/* empty tgtsvr pointers list */
    (
    WTX_FREE_NODE *	pWtxFreeNode,	/* element in singaly linked list     */
    HWTX *		hWtx		/* WTX session handler                */
    );
#if 0
/* XXX : fle : for SRP#67326 */
#ifdef HOST
LOCAL STATUS wtxAsyncResultFree_2	/* frees async event list       */
    (
    WTX_EVENT_DESC *	eventDesc	/* pointer to structure to free */
    );
#endif /* HOST */
#endif /* 0 */

LOCAL BOOL wtxPtrMatch			/* matches a pointer value        */
    (
    WTX_FREE_NODE *	pWtxFreeNode,	/* element in singaly linked list */
    void *		pToMatch	/* pointer value to find          */
    );

LOCAL void wtxDescFree			/* free a WTX_DESC structure */
    (
    WTX_DESC *	pDescToFree		/* WTX_DESC pointer to free  */
    );

LOCAL WTX_DESC * wtxDescDuplicate	/* duplicates a WTX_DESC       */
    (
    HWTX	hWtx,			/* current WTX session handler */
    WTX_DESC *	pDesc			/* WTX_DESC to duplicate       */
    );

LOCAL void toolCleanup			/* cleanups when a tool detaches */
    (
    HWTX		hWtx		/* WTX API handler */
    );

LOCAL void serverDescFree		/* frees server description item */
    (
    HWTX		hWtx		/* WTX API handler */
    );

#ifndef HOST
LOCAL char * stringDup			/* duplicates a string */
    (
    const char *	strToDup	/* string to duplicate */
    );

#define	strdup	stringDup
#endif /* ! HOST */

/*
 * NOTE: This is a list of WTX protocol calls not accessible from the 
 *       WTX C API and is based on the service numbers in wtxmsg.h
 *
 * WTX_VIO_READ			- not implemented in server (obsolete?)
 * WTX_SYM_TBL_CREATE/DELETE	- to do
 * WTX_SYM_TBL_LIST		- to do
 * WTX_WTX_SERVICE_LIST		- not implemented in server - undefined
 * WTX_WDB_SERVICE_LIST		- not implemented in server - undefined
 * WTX_OBJ_KILL			- (currently TS kill only) Enough ?
 * WTX_OBJ_RESTART		- undefined (obsolete ?)
 */

#if defined (HOST) && !defined (_DEBUG)

/*******************************************************************************
*
* wtxLmDeauthorize - Check-in a license
*
* This routine releases a license previously checked-out by wtxLmAuthorize ().
* Typically, it is called when the process exits.
*
* RETURNS: N/A
*
* SEE ALSO: wtxLmAuthorize ()
*
* NOMANUAL
*/

void wtxLmDeauthorize (void)
    {

    /* if we own a license, release it. */

    if (gotLicense)
	{
	flexlmDeauthorize ();
	}
    }

/*******************************************************************************
*
* wtxLmAuthorize - Check-out a license
*
* This routine establishes a connection to the license server, and tries to
* Check out a license.
* Currently, we check a license upon first call to this routine. Once we get a
* license, we always return OK to any subsequent call to this routine.
* We register a handle to release the license when the tool exits.
*
* RETURNS: WTX_OK or WTX_ERROR if the check-out fails.
*
* SEE ALSO: wtxLmDeauthorize ()
*
* NOMANUAL
*/

STATUS wtxLmAuthorize (void)
    {

    /* If we already got a license return OK */
    
    if (gotLicense)
	return OK;

    /* First time: check for a valid license */

    if (flexlmAuthorize (FEATURE_TORNADO_2_2, VERSION_TORNADO_2_2, 1) != OK)
	return ERROR;

    gotLicense = 1;	/* we got one! */
    
    /* Record a handler to release the license when the process exits */

    atexit (wtxLmDeauthorize);

    return OK;	/* Return sucess */
    }
#endif

/*******************************************************************************
*
* wtxToolAttach - connect a WTX client to the target server
*
* This routine establishes a connection to the target server
* called <serverName> and announces the client as a WTX tool called
* <toolName>. If <serverName> does not contain an `@' character, it is
* used as a regular expression; if it matches more than one (registered)
* target server name, an error is returned.  If <serverName> contains 
* an `@' character then it must be an exact match for a valid target 
* server name.
*
* RETURNS: WTX_OK or WTX_ERROR if the attach fails.
*
* ERRORS:
* .iP WTX_ERR_API_ALREADY_CONNECTED 12
* The handle is already connected to a target server.
* .iP WTX_ERR_API_SERVER_NOT_FOUND 
* <serverName> does not match a target server name using the above criteria.
* .iP WTX_ERR_API_AMBIGUOUS_SERVER_NAME 
* <serverName> matches more than one target server name.
* .iP WTX_ERR_SVR_DOESNT_RESPOND
* <serverName> is dead : no RPC connection can be achieved
* .iP WTX_ERR_SVR_IS_DEAD
* <serverName> is dead : server has been found dead
* .iP WTX_ERR_SVR_INVALID_LICENSE
* No license could be checked-out for this tool.
*
* SEE ALSO: WTX_TOOL_ATTACH, wtxToolDetach(), wtxToolConnected(), wtxInfoQ()
*/

STATUS wtxToolAttach 
    (
    HWTX		hWtx,			/* WTX API handle             */
    const char *	serverName,		/* Target Server name         */
    const char *	toolName		/* tool name                  */
    )
    
    {
    WTX_MSG_TOOL_DESC	in;			/* WTX param                  */
    WTX_MSG_TOOL_DESC *	pOut = NULL;		/* WTX result                 */
    WTX_DESC_Q *	pTsDescQ = NULL;	/* Q Info about Target Server */
    WTX_ERROR_T		callStat;		/* status of WTX call         */
    WTX_ERROR_T		errCode=WTX_ERR_NONE;	/* connection init status     */
    WTX_DESC *		pSaveDesc = NULL;	/* saved server desc          */
    WTX_DESC *		pTsDesc = NULL;		/* Info about Target Server   */
    WTX_DESC *		pNewDesc = NULL;	/* for updated tgtsvr entry   */
    char *		envUser = NULL;		/* result of getenv(USER)     */
    char *		oldKey = NULL;		/* for multi adapters tgtsvrs */
    char *		tmpKey = NULL;		/* for multi adapters tgtsvrs */
    BOOL		usePMap = FALSE;	/* do we use port mapper ?    */
    char		userName [256];		/* to format "user@host"      */
    char		serverAtHost [256];	/* serverName@host            */
    char		hostNameBuf [32];	/* for gethostname ()         */
    int			ipNum = 0;		/* number of IP addr in list  */

#ifdef WIN32
    char                usrName [256];		/* holds name for Win32       */
#endif

    WTX_CHECK_HANDLE (hWtx, WTX_ERROR);

    if (wtxToolConnected (hWtx))
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_ALREADY_CONNECTED, WTX_ERROR);

    if (hWtx->pServerDesc) 
	{
	wtxResultFree (hWtx, hWtx->pServerDesc);
	hWtx->pServerDesc = NULL;
	}

    if (toolName == NULL)
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_INVALID_ARG, WTX_ERROR);

#if defined (HOST) && !defined (_DEBUG)

    /* 
     * License check (HOST side, release version only):
     * if NO_LICENSE if defined for this module, we'll look if NO_LICENSE is
     * defined in the process' environment. If this is the case, we skip
     * license check. 
     * In all cases, GDB won't be checked for license.
     */

#ifdef NO_LICENSE
    if (!envVarChecked)
	{
	licenseCheck = (wpwrGetEnv ("NO_LICENSE") != NULL)?FALSE:TRUE;
	envVarChecked = TRUE;
	}
#endif /* NO_LICENSE */

     if (licenseCheck)
	 licenseCheck = (strstr (toolName, "gdb") == NULL)?TRUE:FALSE;
	
    if ((licenseCheck) && (wtxLmAuthorize () != OK))
	{
	WTX_ERROR_RETURN (hWtx, WTX_ERR_SVR_INVALID_LICENSE, WTX_ERROR);
	}
#endif /* HOST */

    /* save current name in serverAtHost */

    strcpy (serverAtHost, serverName);

    if (!strchr (serverName, '@'))			/* name: tgtsvr */
	{
	pTsDescQ = wtxInfoQ (hWtx, (char *)serverName, "tgtsvr", NULL);

	if (pTsDescQ == NULL)
	    WTX_ERROR_RETURN (hWtx, WTX_ERR_API_SERVER_NOT_FOUND, WTX_ERROR);
	
	if (pTsDescQ->pNext != NULL)
	    {
	    wtxResultFree (hWtx, pTsDescQ);
	    WTX_ERROR_RETURN (hWtx, WTX_ERR_API_AMBIGUOUS_SERVER_NAME, 
			      WTX_ERROR);
	    }

	/*
	 * There is an @ in the target server name, let's see if it is really
	 * in registry data base.
	 */

	strcpy (serverAtHost, pTsDescQ->wpwrName);

	wtxResultFree (hWtx, pTsDescQ);
	}

    pTsDesc = wtxInfo (hWtx, (char *)serverAtHost);	/* name: tgtsvr@host */

    if (pTsDesc == NULL)
	WTX_ERROR_RETURN (hWtx, hWtx->errCode, WTX_ERROR);

    hWtx->pServerDesc = pTsDesc;

    /* allocate space to copy server information */

    if ( (pSaveDesc = (WTX_DESC *) malloc (sizeof (WTX_DESC))) == NULL)
	{
	wtxResultFree (hWtx, hWtx->pServerDesc);
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_MEMALLOC, WTX_ERROR);
	}

    /* copy server information */

    memcpy ((void *) pSaveDesc, (void *) hWtx->pServerDesc, sizeof (WTX_DESC));

    if (hWtx->pServerDesc->wpwrName != NULL)
	{
	if ((pSaveDesc->wpwrName = (char *) malloc (strlen
			    (hWtx->pServerDesc->wpwrName) + 1)) == NULL)
	    {
	    free (pSaveDesc);
	    wtxResultFree (hWtx, hWtx->pServerDesc);
	    WTX_ERROR_RETURN (hWtx, WTX_ERR_API_MEMALLOC, WTX_ERROR);
	    }
	    
        strcpy (pSaveDesc->wpwrName, hWtx->pServerDesc->wpwrName);
	}

    if (hWtx->pServerDesc->wpwrKey != NULL)
	{
	if ((pSaveDesc->wpwrKey = (char *) malloc (strlen
			    (hWtx->pServerDesc->wpwrKey) + 1)) == NULL)
	    {
	    free (pSaveDesc->wpwrName);
	    free (pSaveDesc);
	    wtxResultFree (hWtx, hWtx->pServerDesc);
	    WTX_ERROR_RETURN (hWtx, WTX_ERR_API_MEMALLOC, WTX_ERROR);
	    }
	    
        strcpy (pSaveDesc->wpwrKey, hWtx->pServerDesc->wpwrKey);
	}

    if (hWtx->pServerDesc->wpwrType != NULL)
	{
	if ((pSaveDesc->wpwrType = (char *) malloc (strlen
			    (hWtx->pServerDesc->wpwrType) + 1)) == NULL)
	    {
	    free (pSaveDesc->wpwrKey);
	    free (pSaveDesc->wpwrName);
	    free (pSaveDesc);
	    wtxResultFree (hWtx, hWtx->pServerDesc);
	    WTX_ERROR_RETURN (hWtx, WTX_ERR_API_MEMALLOC, WTX_ERROR);
	    }
	    
        strcpy (pSaveDesc->wpwrType, hWtx->pServerDesc->wpwrType);
	}

    wtxResultFree (hWtx, hWtx->pServerDesc);
    registryDisconnect (hWtx);

    hWtx->pServerDesc = pSaveDesc;

    /* first just initialise the connection */

    if (wtxExchangeInitialize (&hWtx->server) != WTX_OK ||
	wtxExchangeInstall (hWtx->server, 
			    wtxRpcExchangeCreate, 
			    wtxRpcExchangeDelete, 
			    wtxRpcExchange, 
			    wtxRpcExchangeFree, 
			    wtxRpcExchangeControl) != WTX_OK)
	{
	/* Record the error code */

	errCode = wtxExchangeErrGet ((WTX_XID) hWtx->server);

	/* free server descriptor */

	serverDescFree (hWtx);

	/* Cleanup the exchange */

	wtxExchangeTerminate ((WTX_XID) hWtx->server);
	hWtx->server = (WTX_XID) NULL;

	WTX_ERROR_RETURN (hWtx, errCode, WTX_ERROR);
	}

use_pmap:

    if (wtxExchangeCreate ((WTX_XID) hWtx->server, 
			   hWtx->pServerDesc->wpwrKey, usePMap) != WTX_OK)
	{
	/*
	 * if the error comes from the wtxExchangeCreate() routine and if
	 * it has to deal with the IP address, then let's try to see if we
	 * can change the IP adapter
	 */

	if (wtxExchangeErrGet (hWtx->server) == WTX_ERR_EXCHANGE_NO_SERVER)
	    {
	    /* if there are several adapter, let's try the next one */

	    oldKey = hWtx->pServerDesc->wpwrKey;

	    while ((tmpKey = wtxRpcKeyIPListUpdate (hWtx->pServerDesc->wpwrKey,
						    ++ipNum)) != NULL)
		{
		if (wtxExchangeCreate ((WTX_XID) hWtx->server, tmpKey, usePMap)
		    == WTX_OK)
		    {
		    HWTX	regdHWtx;	/* wtx handle for registry    */

		    /* this is the right key, save it and update registry */

		    hWtx->pServerDesc->wpwrKey = tmpKey;
		    free (oldKey);

		    /* init the registry session */

		    wtxInitialize (&regdHWtx);

		    /* update registry entry */

		    if (wtxUnregister (regdHWtx, hWtx->pServerDesc->wpwrName)
			!= WTX_OK)
			WTX_ERROR_RETURN (hWtx, regdHWtx->errCode, WTX_ERROR);

		    if ( (pNewDesc = wtxRegister (regdHWtx,
						  hWtx->pServerDesc->wpwrName,
						  hWtx->pServerDesc->wpwrType,
						  tmpKey)) == NULL)
			{
			WTX_ERROR_RETURN (hWtx, regdHWtx->errCode, WTX_ERROR);
			}

		    /*
		     * end the registry session, wtxTerminate should free the
		     * new description, no need to free it manuall.
		     */

		    wtxTerminate (regdHWtx);

		    goto updated_key;
		    }
		}

	    /* ok, now try again with the port mapper then ... */

	    if (!usePMap)
		{
		usePMap = TRUE;
		goto use_pmap;
		}
	    }

	/* Record the error code */

	errCode = wtxExchangeErrGet ((WTX_XID) hWtx->server);

	/* free server descriptor */

	serverDescFree (hWtx);

	/* Cleanup the exchange */

	wtxExchangeTerminate ((WTX_XID) hWtx->server);
	hWtx->server = (WTX_XID) NULL;

	WTX_ERROR_RETURN (hWtx, errCode, WTX_ERROR);
	}

updated_key:

    memset (&in, 0, sizeof (in));

    in.wtxToolDesc.toolName = (char *) toolName;
#ifdef HOST
#ifdef WIN32
    {
        UINT32 size = sizeof (usrName);    

        if(!GetUserName (usrName, &size))
	    envUser = NULL;
	else
	    envUser = usrName;
    }
#else
    envUser = getenv ("USER");
#endif /* WIN32 */
    gethostname (hostNameBuf, sizeof (hostNameBuf));
#else
    envUser = sysBootParams.usr;
    strcpy (hostNameBuf, sysBootParams.targetName);
#endif /* HOST */
    if (envUser == NULL)
	envUser = "unknown";
    if (hostNameBuf[0] == '\0')
	strcpy (hostNameBuf, "unknown");
    sprintf (userName, "%.24s@%.24s", envUser, hostNameBuf);

    in.wtxToolDesc.userName = userName;

    pOut = calloc (1, sizeof (WTX_MSG_TOOL_DESC));
    if (pOut == NULL)
	{
	/* Close the connection to the server */
	wtxExchangeDelete (hWtx->server);

	/* free server descriptor */
	serverDescFree (hWtx);

	/* Clean up the exchange */
	wtxExchangeTerminate (hWtx->server);
	hWtx->server = NULL;

	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_MEMALLOC, WTX_ERROR);
	}

    /* Do the attach call */
    callStat = exchange (hWtx, WTX_TOOL_ATTACH, &in, pOut);
	
    if (callStat != WTX_ERR_NONE)
	{
	/* Close the connection to the server */
	wtxExchangeDelete (hWtx->server);

	/* free server descriptor */
	serverDescFree (hWtx);

	/* Clean up the exchange */
	wtxExchangeTerminate (hWtx->server);
	hWtx->server = NULL;

	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);
	}

    /* Set the msgToolId and pToolDesc filed in the HWTX for future use */
    hWtx->pToolDesc = &pOut->wtxToolDesc;
    hWtx->msgToolId.wtxCore.objId = pOut->wtxToolDesc.id;

    if (wtxFreeAdd (hWtx, (void *) &pOut->wtxToolDesc, NULL, pOut,
		    WTX_TOOL_ATTACH, hWtx->server, WTX_SVR_SERVER) != WTX_OK)
	{
	WTX_ERROR_RETURN (hWtx, wtxErrGet (hWtx), WTX_ERROR);
	}

    return (WTX_OK);
    }


/*******************************************************************************
*
* wtxToolConnected - check to see if a tool is connected to a target server
*
* This routine checks if the tool represented by <hWtx> is currently
* connected to a target server. 
*
* NOTE: If <hWtx> is an invalid handle then FALSE is returned.
*
* RETURNS: TRUE if the tool is connected, FALSE otherwise.
*
* SEE ALSO: wtxErrClear(), wtxErrGet()
*/

BOOL32 wtxToolConnected 
    (
    HWTX	hWtx	/* WTX API handle */
    )
    {
    WTX_CHECK_HANDLE (hWtx, FALSE);

    return (hWtx->server != NULL);
    }

/*******************************************************************************
*
* wtxToolDetach - detach from the target server
*
* This routine detaches from the target server. 
* The connection status for <hWtx> is cleared and any memory
* allocated by the tool attach is freed. 
*
* CLEANUP
* This routine cleans all the tools allocated pointers. All items that have been
* allocated by `wtx' calls during the wtxToolAttach() are freed. All the
* elements are freed calling for the wtxResultFree() routine. This makes
* impossible to access data from previous `wtx' calls.
*
* NOTE: Even if the detach fails internally (for example, the server
* it is attached to has died), the API still puts the handle into a
* detached state and performs all necessary internal cleanup. In this
* case the internal error is `not' reported since the tool is no longer
* attached and the handle can subsequently be attached to another server.
*
* RETURNS: WTX_OK or WTX_ERROR.
*
* SEE ALSO: WTX_TOOL_DETACH, wtxToolAttach()
*/

STATUS wtxToolDetach 
    (
    HWTX	hWtx		/* WTX API handle */
    )
    {
    WTX_ERROR_T		callStat;	/* WTX call status */
    WTX_MSG_RESULT	out;		/* WTX result */

    WTX_CHECK_HANDLE (hWtx, WTX_ERROR);

    if (! wtxToolConnected(hWtx))
	return (WTX_OK);

#ifdef HOST
    /* close async. notif */

    wtxAsyncStop (&hWtx->asyncHandle);
    hWtx->asyncHandle = NULL;
#endif /* HOST */

    memset (&out, 0, sizeof (out));

    callStat = exchange (hWtx, WTX_TOOL_DETACH, &hWtx->msgToolId, &out);

    /* Free allocated memory and close neatly the connection to the server */

    if (callStat == WTX_ERR_NONE)
	wtxExchangeFree (hWtx->server, WTX_TOOL_DETACH, &out);

    /* 
     * free the server descriptor and the strings that were allocated in 
     * wtxToolAttach().
     */

    serverDescFree (hWtx);

    toolCleanup(hWtx);

    /* Actively ignore any errors that may occur */

    return (WTX_OK);
    }


/*******************************************************************************
*
* wtxInitialize - initialization routine to be called by the WTX client
*
* This routine allocates a handle structure for the tool's use and
* does any initialization required for use of the WTX interface.  All
* subsequent calls by the tool should use the handle returned in <phWtx>.
* If WTX_ERROR is returned and the handle <phWtx> is zero, then the
* initialization failed because the internal handle structure could
* not be allocated. Otherwise use wtxErrMsgGet() to find the cause
* of the error.
* 
* RETURNS: WTX_OK or WTX_ERROR if the initialization fails.
*
* ERRORS: 
* .iP WTX_ERR_API_INVALID_ARG 12
* The pointer <phWtx> is NULL.
* .iP WTX_ERR_API_MEMALLOC 
* The handle cannot be allocated.
*
* SEE ALSO: wtxTerminate(), wtxVerify()
*/

STATUS wtxInitialize 
    (
    HWTX * phWtx	/* RETURN: handle to use in subsequent API calls */
    )
    {
    if (phWtx == NULL)
	WTX_ERROR_RETURN (NULL, WTX_ERR_API_INVALID_ARG, WTX_ERROR);

    *phWtx = calloc (1, sizeof (struct _wtx));
    
    if (*phWtx == NULL)
	WTX_ERROR_RETURN (NULL, WTX_ERR_API_MEMALLOC, WTX_ERROR);
    
    /* Set the field that identifies this as a valid handle */
    (*phWtx)->self = *phWtx;
    (*phWtx)->pWtxFreeList = NULL;

    return (WTX_OK);
    }


/*******************************************************************************
*
* wtxTerminate - terminate the use of a WTX client handle
*
* This routine destroys the specified context handle so it may no
* longer be used in WTX API calls.  If the tool is attached to a
* target server, it is first detached. (It is forcibly detached if
* errors make a normal detach impossible.)  Any memory allocated by 
* the handle is freed and the handle is invalidated; any subsequent 
* use causes an abort.
*
* CLEANUP
* This routine frees all the `wtx' pointers that have been allocated by `wtx'
* calls during the session. Thus, it makes it impossible to address them anymore
*
* RETURNS: WTX_OK or WTX_ERROR.
*
* SEE ALSO: wtxInitialize(), wtxVerify()
*/

STATUS wtxTerminate
    (
    HWTX	hWtx		/* WTX API handle */
    )
    {
    WTX_CHECK_HANDLE (hWtx, WTX_ERROR);

    wtxToolDetach (hWtx);

    /*
     * as we exit the WTX session, all the pointers from the wtxResultFree()
     * pointers list should be freed.
     */

    if (hWtx->pWtxFreeList != NULL)
	{
	if (sllCount (hWtx->pWtxFreeList) > 0)
	    {
	    sllEach (hWtx->pWtxFreeList, wtxResultFreeListTerminate,
		     (int) &hWtx);
	    }
	sllDelete (hWtx->pWtxFreeList);
	hWtx->pWtxFreeList = NULL;
	}

    if (hWtx->pServerDesc) 
	{
	wtxResultFree (hWtx, hWtx->pServerDesc);
	}

    if (hWtx->pSelfDesc)
	{
	wtxResultFree (hWtx, hWtx->pSelfDesc);
	}

    if (hWtx->registry)
	wtxExchangeDelete (hWtx->registry);

    wtxExchangeTerminate (hWtx->server);
    wtxExchangeTerminate (hWtx->registry);

    /* Invalidate this handle in case it is used after terminate */

    hWtx->self = NULL;

    /* Now free up the handle memory */

    free (hWtx);

    return (WTX_OK);
    }


/*******************************************************************************
*
* wtxErrSet - set the error code for the handle
*
* This routine sets the error value <errCode> in the handle specified 
* by <hWtx> so that wtxErrGet() can return <errCode> as the error.
*
* NOTE: Error handlers for the handle are not called. To set the error
* code and call the registered error handlers, use wtxErrDispatch().
*
* RETURNS: WTX_OK or WTX_ERROR.
*
* SEE ALSO: wtxErrGet(), wtxErrMsgGet(), wtxErrClear(), wtxErrDispatch().
*/

STATUS wtxErrSet 
    (
    HWTX 	hWtx, 		/* WTX API handle */
    UINT32	errCode		/* error value to set */
    )
    {
    WTX_CHECK_HANDLE (hWtx, WTX_ERROR);

    /* Ensure any allocated strings are freed up */
    wtxErrClear (hWtx);

    hWtx->errCode = errCode;
    return (WTX_OK);
    }


/*******************************************************************************
*
* wtxErrGet - return the last error for a handle
*
* This routine returns the last error that occurred for the <hWtx> handle.
* The error code is only valid after an error is reported by one of the
* API calls. To check for an error after a series of API calls use 
* wtxErrClear() to clear the error status at the start and call wtxErrGet()
* at the end. 
*
* RETURNS: The last error code or WTX_ERROR if the handle is invalid.
*
* SEE ALSO: wtxErrMsgGet(), wtxErrSet(), wtxErrClear()
*/

WTX_ERROR_T wtxErrGet 
    (
    HWTX	hWtx		/* WTX API handle */
    )

    {
    WTX_CHECK_HANDLE (hWtx, WTX_ERROR);

    return (hWtx->errCode);
    }

/*******************************************************************************
*
* wtxErrHandlerAdd - add an error handler for the WTX handle
*
* This routine adds a new error handler to the list of registered handlers
* for the handle <hWtx>.  The last error handler added is the first one
* called when an error occurs. The function <pFunc> is called with three
* arguments, the handle on which the error occurred, the client data
* <pClientData>, and a call data parameter which is the error code. If the
* function returns the value TRUE then each previously registered handler
* function is called in turn until all are called or one returns the
* value FALSE.
*
* EXAMPLE
* The following is a sample error handler:
*
* .CS
*   BOOL32 errorHandler
*       (
*	HWTX   hWtx,	     /@ WTX API handle @/
*	void * pClientData,  /@ client data from wtxErrHandlerAdd() call @/
*	void * errCode       /@ error code passed from wtxErrDispatch() @/
*       )
*
*       {
*	/@ print an error message @/
*	
*	fprintf (stderr, 
*		 "Error %s (%d) from server %s\n", 
*		 wtxErrMsgGet (hWtx),
*		 (WTX_ERROR_T) errCode,		/@ or use wtxErrGet() @/
*		 wtxTsNameGet (hWtx));
*
*	/@ return TRUE allowing previously installed handlers to be called @/
*
*	return TRUE;
*	}
* .CE
*
* RETURNS: A new handler ID or NULL on failure.
*
* ERRORS:
* .iP WTX_ERR_API_MEMALLOC 12
* No memory is available to add the new handler.
*
* SEE ALSO: wtxErrHandlerRemove(), wtxErrDispatch()
*/

WTX_HANDLER_T wtxErrHandlerAdd 
    (
    HWTX		hWtx,		/* WTX API handle */
    WTX_HANDLER_FUNC	pFunc,		/* function to call on error */
    void *		pClientData	/* data to pass function */
    )
    {
    WTX_HANDLER_T descNew;

    WTX_CHECK_HANDLE (hWtx, NULL);

    descNew = calloc (1, sizeof (_WTX_HANDLER_T));

    if (descNew == NULL)
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_MEMALLOC, NULL);
    
    descNew->pFunc = pFunc;
    descNew->pClientData = pClientData;
    descNew->prev = hWtx->errHandler;
    hWtx->errHandler = descNew;

    return descNew;
    }

/*******************************************************************************
*
* wtxErrHandlerRemove - remove an error handler from the WTX handle
*
* This function removes the error handler referenced by <errHandler> from 
* the handler list for <hWtx>. The error handler ID <errHandler> must be a 
* valid error handler ID returned by a call of wtxErrHandlerAdd().
*
* NOTE: It is safe for wtxErrHandlerRemove() to be called from within an
* error handler function, even if the call is to remove itself.
*
* RETURNS: WTX_OK or WTX_ERROR.
*
* ERRORS:
* .iP WTX_ERR_API_HANDLER_NOT_FOUND 12
* <errHandler> is not a valid handler ID.
*
* SEE ALSO: wtxErrHandlerAdd(), wtxErrDispatch() 
*/

STATUS wtxErrHandlerRemove 
    (
    HWTX hWtx,			/* WTX API handle */
    WTX_HANDLER_T errHandler	/* Error handler to remove */
    )
    {
    WTX_HANDLER_T desc;
    WTX_HANDLER_T last;

    WTX_CHECK_HANDLE (hWtx, WTX_ERROR);

    last = NULL;

    for (desc = hWtx->errHandler; desc; desc = desc->prev)
	{
	if (desc == errHandler)
	    {
	    /* Got the one to remove */
	    if (last)
		last->prev = desc->prev;
	    else
		hWtx->errHandler = desc->prev;
	    free (desc);
	    return (WTX_OK);
	    }
	last = desc;
	}

    WTX_ERROR_RETURN (hWtx, WTX_ERR_API_HANDLER_NOT_FOUND, WTX_ERROR);
    }

/*******************************************************************************
*
* wtxErrDispatch - dispatch an error with supplied code for the given handle
*
* This function records the error <errCode> against the handle <hWtx> and 
* calls all the registered error handlers for it until one returns FALSE.
*
* RETURNS: WTX_OK or WTX_ERROR if the handle is invalid.
*
* SEE ALSO: wtxErrHandlerAdd()
*/

STATUS wtxErrDispatch
    (
    HWTX	hWtx,		/* WTX API handle */
    WTX_ERROR_T	errCode		/* error code to register */
    )
    {
    WTX_HANDLER_T desc;
    BOOL32 continueToDispatch;

    /* cannot use macro here as it will cause recursion via WTX_ERROR_RETURN */

    if (hWtx == NULL || hWtx->self != hWtx)
	{
	/* cannot do anything with the error */

	/* FIXME: should implement a global error handler */
	return WTX_ERROR;
	}

    /* Record the error code */
    hWtx->errCode = errCode;
    hWtx->errMsg = NULL;

    continueToDispatch = TRUE;
    desc = hWtx->errHandler; 

    /* Dispatch the error to all the error handlers */
    while ((desc != NULL) && continueToDispatch)
	{
	WTX_HANDLER_T prev;

	/* Just in case the handler removes itself! */
	prev = desc->prev;

	/* Be sure the function is non-null */
	if (desc->pFunc)
	    continueToDispatch = desc->pFunc (hWtx, 
					      desc->pClientData, 
					      (void *) errCode);
	desc = prev;
	}

    return (WTX_OK);
    }

/*******************************************************************************
* 
* wtxErrMsgGet - fetch the last network WTX API error string
*
* This routine gets a meaningful string for the last WTX API call
* that returned WTX_ERROR. The string is only valid after a WTX
* call has returned an error.
*
* NOTE: The return value is a pointer to internal data and must
* not be freed by the caller. Also the string is only valid until
* the next error occurs or wtxErrClear() is called. It must
* be copied by the caller if the value must be stored.
*
* RETURNS: A pointer to a string or NULL if an error has occurred. 
*
* SEE ALSO: wtxErrClear(), wtxErrGet()
*/

const char * wtxErrMsgGet
    (
    HWTX	hWtx	/* WTX API handle */
    )
    {
    WTX_CHECK_HANDLE (hWtx, NULL);

    /* This allows for caching of previous values but is not yet implemented */
    if (hWtx->errMsg != NULL)
	return hWtx->errMsg;
    else
	{
	hWtx->errMsg = wtxErrToMsg (hWtx, hWtx->errCode);
	return hWtx->errMsg;
	}
    }


/*******************************************************************************
*
* wtxErrToMsg - convert an WTX error code to a descriptive string
*
* This routine takes an error code which has been returned by a WTX API
* call and returns a descriptive string. The value returned is a pointer
* to a string in statically allocated memory. The string must be copied 
* if the value is to be stored and it must not be freed by the caller.
*
* RETURNS: A pointer to an error string.
*/

const char * wtxErrToMsg
    (
    HWTX hWtx, 
    WTX_ERROR_T errCode
    )
    {
    static char buffer [256];

    if (errCode == WTX_ERR_NONE)
	return "No error";

    if ((errCode > WTXERR_BASE_NUM) && (errCode < WTX_ERR_LAST))
	{
	FILE * fp;

#ifdef WIN32
	sprintf (buffer, "%s\\host\\resource\\tcl\\wtxerrdb.tcl", 
		 getenv ("WIND_BASE"));
#else
	sprintf (buffer, "%s/host/resource/tcl/wtxerrdb.tcl", 
		 getenv ("WIND_BASE"));
#endif
    
	fp = fopen (buffer, "r");
	while (fp != NULL && ! ferror (fp))
	    {
	    UINT32	errNum;
	    char	errStr[256];

	    if (fgets (buffer, sizeof (buffer), fp) == NULL)
		break;

	    if (sscanf (buffer, "set wtxError(0x%x) %s", &errNum, errStr) == 2
		&& (errNum == (UINT32)errCode))
		{
		sprintf (buffer, "%s", errStr);
		fclose (fp);
		return buffer;
		}
	    }

	if (fp)
	    fclose (fp);

	/* A WTX error we have no error text for */
	sprintf (buffer, "WTX error %#x", errCode);
	}

#ifdef SUN4_SUNOS4
    /*
     * Avoid strerror() on SUNOS4, as this will pull libiberty.a into
     * the link, making it more difficult to bind this code into a
     * shared library.
     */
    else if (errCode > 0 && errCode < sys_nerr && sys_errlist [errCode] != NULL)
        {
        /* Probably a system error */
        sprintf (buffer, "%s (%d)", sys_errlist [errCode], errCode);
        }
#else  /* !SUN4_SUNOS4 */
    else if (strerror (errCode) != NULL)
        /* Probably a system error */
        sprintf (buffer, "%s (%d)", strerror (errCode), errCode);
#endif /* SUN4_SUNOS4 */

    else
	/* Some other error we don't know about */
	sprintf (buffer, "error %d (%#x)", errCode, errCode);

    return buffer;
    }

/*******************************************************************************
*
* wtxErrClear - explicitly clear any error status for the tool
*
* This routine clears an error message already recorded. It can be
* called before a WTX routine if you want to test for an error afterwards
* by checking whether wtxErrGet() returns a non-zero value.
*
* RETURNS: WTX_OK or WTX_ERROR.
*
* SEE ALSO: wtxErrGet(), wtxErrMsgGet(), wtxErrSet()
*/

STATUS wtxErrClear
    (
    HWTX	hWtx	/* WTX API handle */
    )
    {
    WTX_CHECK_HANDLE (hWtx, WTX_ERROR);

    hWtx->errCode = WTX_ERR_NONE;
    hWtx->errMsg = NULL;

    return (WTX_OK);
    }


/*******************************************************************************
*
* wtxErrExceptionFunc - a function to handle an error using longjmp()
*
* This function is called as part of the error handling process shown in the
* discussion of C++ style exception catching in the wtx library description
* (\f2WTX C Library\fP).  The WTX_TRY macro, which registers the error
* handler wtxErrExceptionFunc(), is found in wtx.h.  <pClientData> contains
* the address of <jumpBuf> from WTX_TRY and <pCallData> is the error code 
* that is returned by WTX_TRY and should be cast to the type WTX_ERROR_T..
*
* RETURNS: FALSE if <pClientData> is NULL, otherwise it does not return.
* It executes a longjmp() back to <jumpBuf> in the WTX_TRY macro, which
* returns the error code passed back by <pCallData>.
*
* SEE ALSO: wtxErrHandlerAdd(), wtxErrDispatch()
*/

BOOL32 wtxErrExceptionFunc 
    (
    HWTX hWtx,			/* WTX API handle */
    void * pClientData,		/* pointer to a jump buffer */
    void * pCallData		/* error code to return via setjmp() */
    )
    {
    if (pClientData != NULL)
	longjmp (pClientData, (int) pCallData);

    return FALSE;
    }


/*******************************************************************************
*
* wtxClientDataGet - get the client data associated with the WTX handle
*
* This routine sets the pointer pointed at by <ppClientData> to the
* value set by the last call to wtxClientDataSet() for the handle <hWtx>.
*
* RETURNS: WTX_OK or WTX_ERROR.
*
* ERRORS: 
* .iP WTX_ERR_API_INVALID_ARG 12
* <ppClientData> is NULL.
*
* SEE ALSO: wtxClientDataSet()
*/

STATUS wtxClientDataGet 
    (
    HWTX hWtx,			/* WTX API handle */
    void ** ppClientData	/* RETURN: pointer to client data pointer */
    )
    {
    WTX_CHECK_HANDLE (hWtx, WTX_ERROR);

    if (ppClientData == NULL)
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_INVALID_ARG, WTX_ERROR);

    *ppClientData = hWtx->pClientData;

    return (WTX_OK);
    }


/*******************************************************************************
*
* wtxClientDataSet - set the client data associated with the WTX handle
*
* This routine causes the value <pClientData> to be associated with the
* WTX API handle <hWtx>. The client data can be used by the
* caller in any way and, except when in the set and get routines,
* is not used or altered in any way by the WTX API. The initial value
* of the client data before it is set is always NULL.
*
* RETURNS: WTX_OK or WTX_ERROR.
*
* SEE ALSO: wtxClientDataGet().
*/

STATUS wtxClientDataSet 
    (
    HWTX hWtx,			/* WTX API handle */
    void * pClientData		/* value to associate with handle */
    )
    {
    WTX_CHECK_HANDLE (hWtx, WTX_ERROR);
    hWtx->pClientData = pClientData;

    return (WTX_OK);
    }

/*******************************************************************************
*
* wtxAgentModeGet - get the mode of the target agent
*
* This routine returns the running mode of the target agent that the 
* current target server is attached to.  
*
* EXPAND ../../../include/wtxtypes.h WTX_AGENT_MODE_TYPE
*
* RETURNS: The current agent running mode or WTX_ERROR.
*
* SEE ALSO: WTX_AGENT_MODE_GET, wtxTsInfoGet(), wtxAgentModeSet()
*/

WTX_AGENT_MODE_TYPE wtxAgentModeGet 
    (
    HWTX		hWtx		/* WTX API handle */
    )
    {
    WTX_ERROR_T		callStat;	/* client status */
    WTX_AGENT_MODE_TYPE	result;		/* debug agent mode */
    WTX_MSG_RESULT	out;		/* WTX result */

    /*
     * All API calls that require a connection to the target server
     * should use the WTX_CHECK_CONNECTED macro before doing anything else.
     * This macro also calls WTX_CHECK_HANDLE to validate the handle.
     */

    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    /* Always remember to initialize the out params */

    memset (&out, 0, sizeof (out));

    callStat = exchange (hWtx, WTX_AGENT_MODE_GET, &hWtx->msgToolId, &out);

    /*
     * If there is an error we don't need to free the result with 
     * wtxExchangeFree() because wtxExchange() (called by exchange()) 
     * does this for us when there is an error.
     */

    if (callStat != WTX_ERR_NONE)
	/*
	 * On error the WTX_ERROR_RETURN macro calls wtxErrDispatch() for
	 * us (which sets the error code in the API handle and calls any
	 * registered error handlers). It then returns the last param as
	 * the result. In the case where the out param was dynamically
	 * allocated (see wtxEventGet() for example) is the API call fails
	 * then we must call free() on the result before the error return.
	 */

	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    /* 
     * Here we extract any information we want from the out param, then
     * free it using wtxExchangeFree() so any exchange allocated memory is
     * released and then return the desired result value.
     */

    result = out.val.value_u.v_int32;

    wtxExchangeFree (hWtx->server, WTX_AGENT_MODE_GET, &out);

    return result;
    }

/*******************************************************************************
*
* wtxAgentModeSet - set the mode of the target agent
*
* This routine sets the mode of the target agent that the current target
* server is attached to.  A given agent may not support all the possible 
* modes.
*
* EXPAND ../../../include/wtxtypes.h WTX_AGENT_MODE_TYPE
*
* RETURNS: WTX_OK or WTX_ERROR.
*
* SEE ALSO: WTX_AGENT_MODE_SET, wtxTsInfoGet(), wtxAgentModeGet()
*/

STATUS wtxAgentModeSet 
    (
    HWTX		hWtx,		/* WTX API handle */
    WTX_AGENT_MODE_TYPE	agentMode	/* debug agent mode */
    )
    {
    WTX_ERROR_T		callStat;	/* client status */
    WTX_MSG_PARAM	in;		/* WTX param */
    WTX_MSG_RESULT	out;		/* WTX result */

    /*
     * All API calls that require a connection to the target server
     * should use the WTX_CHECK_CONNECTED macro before doing anything else.
     * This macro also calls WTX_CHECK_HANDLE to validate the handle.
     */

    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    /* Always remember to initialize the in and out params */

    memset (&in, 0, sizeof (in));
    memset (&out, 0, sizeof (out));

    /* Fill in the relevant fields of the in params for the call */

    in.param.valueType = V_UINT32;
    in.param.value_u.v_uint32 = agentMode;

    /* Now make the call recording the result status */

    callStat = exchange (hWtx, WTX_AGENT_MODE_SET, &in, &out);

    /*
     * If there is an error we don't need to free the result with 
     * wtxExchangeFree() because wtxExchange() (called by exchange()) 
     * does this for us when there is an error.
     */

    if (callStat != WTX_ERR_NONE)

	/*
	 * On error the WTX_ERROR_RETURN macro calls wtxErrDispatch() for
	 * us (which sets the error code in the API handle and calls any
	 * registered error handlers). It then returns the last param as
	 * the result. In the case where the out param was dynamically
	 * allocated (see wtxEventGet() for example) is the API call fails
	 * then we must call free() on the result before the error return.
	 */

	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    /* 
     * Here we extract any information we want from the out param, then
     * free it using wtxExchangeFree() so any exchange allocated memory is
     * released and then return the desired result value.
     */

    wtxExchangeFree (hWtx->server, WTX_AGENT_MODE_SET, &out);

    return (WTX_OK);
    }

/*******************************************************************************
*
* wtxAsyncNotifyEnable - start the asynchronous event notification
*
* This function creates a socket-based link between the target server and 
* the client where events will be sent. If a user defined function 
* is given (pFunc != NULL) then this function is called each time an 
* event is received by the target server, this function must take 
* a WTX_EVENT_DESC as input parameter. If no user defined function is 
* given then received events are just stored in the asynchronous event 
* list, those events can be get using wtxAsyncEventGet().
*
* In conjunction with wtxAsyncNotifyEnable(), wtxRegisterForEvent() (or
* wtxUnregisterForEvent()) must be used to specify which type of events must
* be sent to the requester.
*
* Upon calling this function, the wtxEventGet() and wtxEventListGet() 
* routines will always return nothing.
*
* When an event is coming to the client, the event is put in an 
* WTX_EVENT_DESC structure and passed to the user provided function 
* pointed to by <pFunc>.
*
* All strings contained in the WTX_EVENT_DESC structure must be copied 
* before to be used because their life duration is depending on the events
* stream.
*
* The user defined function must take a WTX_EVENT_DESC * as input 
* parameter. It is called each time an event is received by the target
* server.
* 
* NOTE:
* This service uses WTX_COMMAND_SEND request of the WTX protocol. 
*
* EXAMPLE:
*
* Declaration of a simple event handler.
*
* .CS
*   LOCAL void eventHookRtn	/@ Hook routine used by the notification @/
*       (
*       WTX_EVENT_DESC * event  /@ Event received from the target server @/
*       )
*       {
*	/@ Just print the event string and exit @/
*
*       if (event->event != NULL)
*           printf ("Received event: %s\n", event->event);
*	}
* .CE
*
* Start of the asynchronous notification from the C API.
*
* .CS
*	/@ Initialization @/
*		.
*		.
*
*	/@ Start the notification @/
*
*       if (wtxAsyncNotifyEnable (wtxh, (FUNCPTR) eventHookRtn) 
*	    != (UINT32) WTX_OK)
*	    return (WTX_ERROR);
*
*	/@ core of the WTX application @/
*		.
*		.
* .CE
*
* RETURN: WTX_OK or WTX_ERROR if the request failed.
*
* ERRORS:
* .iP WTX_ERR_API_SERVICE_ALREADY_STARTED 12
* An wtxAsyncNotifyDisable must be called before.
* .iP WTX_ERR_API_CANT_OPEN_SOCKET
* Socket creation, bind or listen failed.
* .iP WTX_ERR_API_CANT_GET_HOSTNAME
* Cannot get name of current host.
*
* SEE ALSO: WTX_COMMAND_SEND, wtxAsyncNotifyEnable(), wtxAsyncEventGet()
*/

STATUS wtxAsyncNotifyEnable
    (
    HWTX            hWtx,              /* WTX API handle        */
    FUNCPTR         pFunc              /* User defined function */
    )
    {
#ifdef HOST
    WTX_MSG_PARAM       in;            /* Query parameters         */
    WTX_MSG_RESULT      out;           /* Query result             */
    WTX_ERROR_T         callStat;      /* Exchange result          */
    int			status;	       /* Return value		   */
    struct hostent *    he;            /* Host entries             */
    char                buf [128];     /* Input param. string      */
    char                bufPort [32];  /* String for port #        */
    char                hostName [32]; /* Host name                */
    SOCK_DESC           sockDesc;      /* Socket descriptor        */

    /*
     * All API calls that require a connection to the target server
     * should use the WTX_CHECK_CONNECTED macro before doing anything else.
     * This macro also calls WTX_CHECK_HANDLE to validate the handle.
     */

    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    /* Open a socket, create the event list and start the threads */

    status = wtxAsyncInitialize (&sockDesc, pFunc, &hWtx->asyncHandle);

    if (status != WTX_OK)
        {
        return (status);
        }

    /* Set input - ouput parameters for WTX_COMMAND_SEND */

    in.param.valueType = V_PCHAR;

    if (gethostname (hostName, sizeof (hostName)) != OK)
        {
        /* Tchao */

        return (WTX_ERR_API_CANT_GET_HOSTNAME);
        }

    he = gethostbyname (hostName);

    memset (buf, 0, sizeof (buf));
    strcpy (buf, WTX_ASYNC_NOTIFY_ENABLE_CMD);
    strcat (buf, SEPARATOR);

    strcat (buf, (const char *) inet_ntoa (*((struct in_addr *)he->h_addr)));
    strcat (buf, SEPARATOR);

    sprintf (bufPort, "%d", sockDesc.portNumber);
    strcat (buf, bufPort);
    in.param.value_u.v_pchar = buf;

    memset (&out, 0, sizeof (out));

    /* Call WTX_COMMAND_SEND - wtxAsyncNotifyEnable service */

    callStat = exchange (hWtx, WTX_COMMAND_SEND, &in, &out);

    if (callStat != WTX_ERR_NONE)
        {
        /* Set off the asynchronous notification */

	wtxAsyncStop (&hWtx->asyncHandle);
	hWtx->asyncHandle = NULL;

        /* Tchao */

        WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);
        }

    wtxExchangeFree (hWtx->server, WTX_COMMAND_SEND, &out);

#endif /* HOST */

    return (WTX_OK);
    }

/*******************************************************************************
*
* wtxAsyncNotifyDisable - stop the asynchronous event notification
*
* This function sends to the target server an order to stop the asynchronous
* notification of events.
*
* NOTE:
* This service uses WTX_COMMAND_SEND request of the WTX protocol.
*
* RETURN: WTX_OK or WTX_ERROR if exchange failed.
*
* SEE ALSO: WTX_COMMAND_SEND, wtxAsyncNotifyEnable(), wtxAsyncEventGet()
*/

STATUS wtxAsyncNotifyDisable
    (
    HWTX        hWtx                       /* WTX API handle */
    )
    {
#ifdef HOST
    WTX_ERROR_T         callStat = WTX_OK; /* Exchange result */
    WTX_MSG_PARAM       in;            /* Query parameters */
    WTX_MSG_RESULT      out;           /* Query result     */
    char                buf [32];      /* Input buffer     */

    /*
     * All API calls that require a connection to the target server
     * should use the WTX_CHECK_CONNECTED macro before doing anything else.
     * This macro also calls WTX_CHECK_HANDLE to validate the handle.
     */

    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    /* Set input - ouput parameters for WTX_COMMAND_SEND */

    in.param.valueType = V_PCHAR;
    strcpy (buf, WTX_ASYNC_NOTIFY_DISABLE_CMD);
    in.param.value_u.v_pchar = buf;

    memset (&out, 0, sizeof (out));

    /* Call WTX_COMMAND_SEND - wtxAsyncNotifyDisable service */

    callStat = exchange (hWtx, WTX_COMMAND_SEND, &in, &out);

    if (callStat == WTX_ERR_NONE)
	wtxExchangeFree (hWtx->server, WTX_COMMAND_SEND, &out);

    /* Free the event list and stop the async. notif. mechanism */

    wtxAsyncStop (&hWtx->asyncHandle);
    hWtx->asyncHandle = NULL;

    /* Tchao */
#endif
    return (WTX_OK);
    }

/*******************************************************************************
*
* wtxBreakpointAdd - create a new WTX eventpoint of type breakpoint
*
* This routine creates a new eventpoint on the target to represent a
* breakpoint at the address <tgtAddr> for the target context <contextId>.
* The target server maintains a list of eventpoints created on the target
* and this can be queried using wtxEventpointList().  When a context is
* destroyed on the target or the target is reset, eventpoints are deleted
* automatically without intervention from the creator.
*
* The context types are given by:
*
* EXPAND ../../../include/wtxtypes.h WTX_CONTEXT_TYPE
*
* When <contextType> is set to WTX_CONTEXT_SYSTEM, then only breakpoints
* in system mode can be added. When <contextType> is set to WTX_CONTEXT_TASK,
* then only breakpoints in context task can be added.
*
* RETURNS: The ID of the new breakpoint or WTX_ERROR if the add fails.
*
* SEE ALSO: WTX_EVENTPOINT_ADD, wtxEventpointAdd(), wtxEventpointDelete(), 
* wtxEventpointList()
*/

UINT32 wtxBreakpointAdd 
    (
    HWTX		hWtx,		/* WTX API handle */
    WTX_CONTEXT_TYPE	contextType,	/* type of context to put bp in */
    WTX_CONTEXT_ID_T	contextId,	/* associated context */
    TGT_ADDR_T		tgtAddr		/* breakpoint address */
    )
    {
    WTX_MSG_RESULT	out;
    WTX_MSG_EVTPT_DESC	in;
    WTX_ERROR_T		callStat;
    UINT32		bpId;
    
    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    memset (&out, 0, sizeof (out));
    memset (&in, 0, sizeof (in));

    in.wtxEvtpt.event.eventType = WTX_EVENT_TEXT_ACCESS;
    in.wtxEvtpt.event.eventArg = (TGT_ARG_T) tgtAddr;

    in.wtxEvtpt.context.contextType = contextType;
    in.wtxEvtpt.context.contextId = contextId;

    /* ??? Not used by target agent */
    in.wtxEvtpt.action.actionType = WTX_ACTION_STOP | WTX_ACTION_NOTIFY; 

    in.wtxEvtpt.action.actionArg = 0;	/* ??? Not used by target agent */
    in.wtxEvtpt.action.callRtn = 0;	/* ??? Not used by target agent */
    in.wtxEvtpt.action.callArg = 0;	/* ??? Not used by target agent */

    callStat = exchange (hWtx, WTX_EVENTPOINT_ADD, &in, &out);

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    bpId = out.val.value_u.v_uint32; 
    wtxExchangeFree (hWtx->server, WTX_EVENTPOINT_ADD, &out);

    return bpId;
    }

/*******************************************************************************
*
* wtxCacheTextUpdate - synchronize the instruction and data caches.
*
* This function flushes the data chache, then invalidates the instruction cache.
* This operation forces the instruction cache to fetch code that may have been 
* created via the data path.
*
* ERRORS:
* .iP WTX_ERR_API_INVALID_ARG 12
* <nBytes> is invalid.
*
* RETURNS: WTX_OK or WTX_ERROR if exchange failed.
*
* SEE ALSO: WTX_CACHE_TEXT_UPDATE, wtxMemRead(), wtxMemWrite()
*/

STATUS wtxCacheTextUpdate
    (
    HWTX        hWtx,                   /* WTX API handle            */
    TGT_ADDR_T  addr,                   /* remote addr to update     */
    UINT32      nBytes                  /* number of bytes to update */
    )
    {
    WTX_ERROR_T                 callStat;
    WTX_MSG_MEM_BLOCK_DESC      in;
    WTX_MSG_RESULT              out;

    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    if (nBytes == 0)
        WTX_ERROR_RETURN (hWtx, WTX_ERR_API_INVALID_ARG, WTX_ERROR);

    memset (&in, 0, sizeof (in));
    memset (&out, 0, sizeof (out));

    in.startAddr = addr;
    in.numBytes = nBytes;

    callStat = exchange (hWtx, WTX_CACHE_TEXT_UPDATE, &in, &out);

    if (callStat != WTX_ERR_NONE)
        WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    wtxExchangeFree (hWtx->server, WTX_CACHE_TEXT_UPDATE, &out);

    return (WTX_OK);
    }

/******************************************************************************
*
* wtxCommandSend - Pass a string to be interpreted by the target server
*
* This function is used to control the target server behavior.
* Services availables are: wtxTsKill(), wtxTsRestart(), wtxLogging (),
* wtxTsLock(), wtxTsUnlock(), wtxTargetReset(), wtxAsyncNotifyEnable(),
* wtxAsyncNotifyDisable().
*
* This service sends a string to the target server which interprets it.
* If the requested service is not known by tsWtxCommandSend(), then
* the string is sent back to the client.
*
* EXAMPLE
* The following is a sample of string
*
* .CS
*    commandString = "wtxTsLock_30";    /@ lock the target server for 30s @/
*    commandString = "wtxTsUnlock";     /@ Unlock the target server @/
*    commandString = "wtxLoggingOn_/folk/pascal/wtx.txt"; /@ wtx log started @/
* .CE
*
* NOTE:
* A separator is used ("_") and only some command strings can be 
* recognized by the target server (plus options if needed):
* .iP "wdbLoggingOn"
* Open the file and log all WDB requests.
* .iP "wdbLoggingOff"
* Close the file and stop the log.
* .iP "wtxLoggingOn"
* Open the file and log all WTX requests.
* .iP "wtxLoggingOff"
* Close the file and stop the log.
* .iP "allLoggingOff"
* Stop WDB and WTX log.
* .iP "wtxAsyncNotifyEnable"
* Start the asynchronous notification of events.
* .iP "wtxAsyncNotifyDisable"
* Stop the asynchronous notification of events.
* .iP "wtxObjKillDestroy"
* Kill the target server.
* .iP "wtxObjKillRestart"
* Restart the target server.
* .iP "wtxTsLock"
* Lock the target server.
* .iP "wtxTsUnlock"
* Unlock the target server.
* .iP "wtxTargetReset"
* Reset the target.
* .iP "wtxTargetIpAddressGet"
* Get the target's IP address (if applicable) in a.b.c.d form.
* .iP "wtxTargetToolNameGet"
* Get the name of the tool used to build target run-time (e.g. "gnu").
*
* RETURNS: string containing the command's result.
* 
* SEE ALSO: wtxTsKill(), wtxTsRestart(), wtxLogging(), wtxTsLock(), 
* wtxTsUnlock(), wtxTargetReset(), wtxAsyncNotifyEnable(),
* wtxAsyncNotifyDisable(), wtxTargetToolNameGet(), wtxTargetIpAddressGet().
*/

char * wtxCommandSend
    (
    HWTX    hWtx,                          /* WTX API handle           */
    char *  commandString                  /* String to be interpreted */
    )
    {
    WTX_MSG_RESULT  out;
    WTX_MSG_PARAM   in;
    WTX_ERROR_T     callStat;

    WTX_CHECK_CONNECTED (hWtx, NULL);

    memset (&out, 0, sizeof (out));
    memset (&in, 0, sizeof (in));

    in.param.valueType = V_PCHAR;
    in.param.value_u.v_pchar = commandString;

    callStat = exchange (hWtx, WTX_COMMAND_SEND, &in, &out);

    if (callStat != WTX_ERR_NONE)
        WTX_ERROR_RETURN (hWtx, callStat, NULL);

    wtxExchangeFree (hWtx->server, WTX_COMMAND_SEND, &out);

    return (out.val.value_u.v_pchar);
    }

/*******************************************************************************
*
* wtxTargetIpAddressGet - gets target IP address.
*
* This routine returns the target's IP address in network byte order. The
* returned address is the one used by the target server to connect the target.
*
* On error (the target have no IP address), this value will be (UINT32) 0.
*
* RETURNS: the target's IP address in network byte order or 0 on error.
*/

UINT32 wtxTargetIpAddressGet 
    (
    HWTX		hWtx		/* WTX API handle             */
    )
    {
    WTX_MSG_RESULT	out;		/* output structure desc      */
    WTX_MSG_PARAM	in;		/* input structure desc       */
    WTX_ERROR_T		callStat;	/* WTX exchange result status */
    UINT32		targetIP = 0;	/* returned target IP address */

    WTX_CHECK_CONNECTED (hWtx, 0);

    memset (&out, 0, sizeof (out));
    memset (&in, 0, sizeof (in));

    in.param.valueType = V_PCHAR;
    in.param.value_u.v_pchar = WTX_TARGET_IP_ADDRESS_CMD;

    callStat = exchange (hWtx, WTX_COMMAND_SEND, &in, &out);

    if (callStat != WTX_ERR_NONE)
        WTX_ERROR_RETURN (hWtx, callStat, 0);

    targetIP = inet_addr (out.val.value_u.v_pchar);

    /* free the RPC buffers */

    wtxExchangeFree (hWtx->server, WTX_COMMAND_SEND, &out);

    return (targetIP);
    }

/*******************************************************************************
*
* wtxTargetToolNameGet - get target tool name.
*
* This routine returns the name of the tool used to build the target run-time.
*
* On error (the target have no tool name), this value will be "Unknown".
*
* RETURNS: a string representing a tool name (like "gnu") or NULL
*
* SEE ALSO: wtxTsInfoGet
*/

char * wtxTargetToolNameGet 
    (
    HWTX		hWtx		/* WTX API handle             */
    )
    {
#ifdef HOST
    WTX_MSG_RESULT	out;		/* output structure desc      */
    WTX_MSG_PARAM	in;		/* input structure desc       */
    WTX_ERROR_T		callStat;	/* WTX exchange result status */

    WTX_CHECK_CONNECTED (hWtx, 0);

    /* return cached name */

    if (hWtx->targetToolName != NULL)
	return hWtx->targetToolName;

    memset (&out, 0, sizeof (out));
    memset (&in, 0, sizeof (in));

    in.param.valueType = V_PCHAR;
    in.param.value_u.v_pchar = WTX_TARGET_TOOLNAME_GET_CMD;

    callStat = exchange (hWtx, WTX_COMMAND_SEND, &in, &out);

    if (callStat != WTX_ERR_NONE)
        WTX_ERROR_RETURN (hWtx, callStat, 0);

    hWtx->targetToolName = strdup (out.val.value_u.v_pchar);

    /* free the RPC buffers */

    wtxExchangeFree (hWtx->server, WTX_COMMAND_SEND, &out);
#endif /* HOST*/

    return (hWtx->targetToolName);
    }
/*******************************************************************************
*
* wtxCpuInfoGet - gets cpu related information from architecturedb file
*
* This function allows to get target CPU related informations. <cpuNum>
* specifies the CPU number in data base, while <cpuInfoType> specifies which
* parameter to get.
*
* <cpuInfoType> can be from the following type
*
* EXPAND ../../../include/wtx.h CPU_INFO
*
* NOTE:
* returned value if not NULL should be freed by caller through a call to free()
*
* RETURNS: a string containing parameter in string format or NULL.
*
* SEE ALSO: wtxTsInfoGet()
*/

char * wtxCpuInfoGet
    (
    int		cpuNum,			/* cpu number to get info on       */
    CPU_INFO	cpuInfoType		/* cpu info type to get            */
    )
    {
    char *	cpuInfo = NULL;		/* returned target cpu information */
#ifdef HOST
    char *	windBase = NULL;	/* win base directory name         */
    char *	cpuFamilyName = NULL;	/* family name of CPU num <cpuNum> */
    char	section [100];		/* section number e.g. CPU ID      */
    char	paramBuf [100];		/* receiving parameter buffer      */
    char	configFile [PATH_MAX];	/* configuration file name         */
    int		ix = 0;			/* loop counter                    */
    int		nChar = 0;		/* # of chars in returned value    */

    CPU_INFO_TYPE	targetCpuInfo[] =	/* cpu data initialization */
	{
	/* param,	string,				section */

	{CPU_INFO_NONE,	"none",					NULL},
	{ARCH_DIR,	"Architecture Directory",		NULL},
	{LEADING_CHAR,	"Leading Character",			NULL},
	{DEMANGLER,	"demangler",				NULL},
	{CPU_NAME,	"cpuname",				NULL},
	{ARCH_OBJ_DIR,	"Architecture Object Directory",	NULL}
	};

    sprintf (section, "CPU_%d", cpuNum);

    /* setting architecture data base file name */

    windBase = wpwrGetEnv ("WIND_BASE");
    if (windBase == NULL)
        {
	goto error1;
	}

    strcpy (configFile, windBase);
    strcat (configFile, "/host/resource/target/architecturedb");

#ifdef WIN32
    wpwrBackslashToSlash (configFile);
#endif /* WIN32 */

    /* get CPU family name */

    nChar = GetPrivateProfileString (section, "cpuFamilyName", "unknown",
				     paramBuf, sizeof (paramBuf), configFile);

    if ( (! strcmp (paramBuf, "unknown")) ||
	 ( (cpuFamilyName = strdup (paramBuf)) == NULL))
	{
	goto error1;
	}

    /* section assignement for parameters */

    targetCpuInfo[1].section = cpuFamilyName;
    targetCpuInfo[2].section = cpuFamilyName;
    targetCpuInfo[3].section = "misc";
    targetCpuInfo[4].section = (char *) section;
    targetCpuInfo[5].section = cpuFamilyName;

    /* set searched string */

    for (ix = 1 ; ix < (int) NELEMENTS (targetCpuInfo) ; ix++)
	{
	if (targetCpuInfo[ix].param == cpuInfoType)
	    {
	    if ( (targetCpuInfo[ix].section != NULL) &&
		 (targetCpuInfo[ix].string != NULL))
		{
		nChar = GetPrivateProfileString (targetCpuInfo[ix].section,
						 targetCpuInfo[ix].string,
						 "unknown", paramBuf,
						 sizeof (paramBuf), configFile);
		}
	    else
		{
		goto error2;
		}
	    break;
	    }
	}

    /* allocate and verify returned value */

    if ( (! strcmp (paramBuf, "unknown")) ||
	 ( (cpuInfo = strdup (paramBuf)) == NULL))
	{
	goto error2;
	}

error2:
    free (cpuFamilyName);
error1:
#endif /* HOST */
    return cpuInfo;
    }

/*******************************************************************************
*
* wtxEventpointAdd - create a new WTX eventpoint
*
* This routine creates a new eventpoint on the target. This routine is a
* generic facility for setting breakpoints. An eventpoint specifies the
* context to which it applies, the type of event to detect (for example 
* breakpoint, context exit), and the action to be taken upon detection of 
* the event (for example, notify the target server of the event).
*
* The target server maintains a list of eventpoints created on the target
* and this can be queried using wtxEventpointList().  When a context is
* destroyed on the target or the target is reset, eventpoints are deleted
* automatically without intervention from the creator.
*
* When <pEvtpt->context.contextType> is set to WTX_CONTEXT_SYSTEM, then 
* only eventpoints in system mode can be added. 
* When <pEvtpt->context.contextType> is set to WTX_CONTEXT_TASK, then only 
* eventpoints in context task can be added.
*
* The eventpoint parameters are grouped into the structure:
*
* EXPAND ../../../include/wtxmsg.h WTX_EVTPT_2
*
* where WTX_EVENT_2 is:
*
* EXPAND ../../../include/wtxmsg.h WTX_EVENT_2
* 
* where WTX_CONTEXT is:
*
* EXPAND ../../../include/wtxmsg.h WTX_CONTEXT
* 
* The context types are given by:
*
* EXPAND ../../../include/wtxtypes.h WTX_CONTEXT_TYPE
*
* If contextType is set to WTX_CONTEXT_TASK and contextId set to 0, 
* then it is equivalent to contextType set to WTX_CONTEXT_ANY_TASK.
*
* where WTX_ACTION is:
*
* EXPAND ../../../include/wtxmsg.h WTX_ACTION
*
* The action types are given by WTX_ACTION_TYPE and can be or'ed 
* together if multiple actions are needed:
*
* EXPAND ../../../include/wtxtypes.h WTX_ACTION_TYPE
*
* RETURNS: The ID of the new eventpoint or WTX_ERROR if the add fails.
*
* SEE ALSO: WTX_EVENTPOINT_ADD_2, wtxEventpointDelete(), wtxEventpointList()
*/

UINT32 wtxEventpointAdd 
    (
    HWTX		hWtx,		/* WTX API handle */
    WTX_EVTPT_2	*	pEvtpt		/* eventpoint descriptor */
    )
    {
    WTX_MSG_RESULT		out;
    WTX_MSG_EVTPT_DESC_2	in;
    WTX_ERROR_T			callStat;
    UINT32			evtptId;
    
    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    memset (&out, 0, sizeof (out));
    memset (&in, 0, sizeof (in));

    /*
     * XXX - must have WTX_EVTPT_2 fields identical to those
     * WTX_MSG_EVTPT_DESC_2 from wtxEvtpt onwards. Awaits change to
     * message definitions on wtxmsg.h so this hack will go away.
     */

    memcpy (&in.wtxEvtpt, pEvtpt, sizeof (WTX_EVTPT_2));

    callStat = exchange (hWtx, WTX_EVENTPOINT_ADD_2, &in, &out);

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    evtptId = out.val.value_u.v_uint32; 
    wtxExchangeFree (hWtx->server, WTX_EVENTPOINT_ADD_2, &out);

    return evtptId;
    }

/*******************************************************************************
*
* wtxEventpointDelete - delete an eventpoint from the target
*
* This routine deletes an existing eventpoint in the context given when it
* was created.  The eventpoint is identified by <evtptId> which is the
* ID returned by wtxBreakpointAdd(), wtxContextExitNotifyAdd()... If the 
* eventpoint has already been deleted on the target (for example, because 
* the context associated with it no longer exists), then the first call of 
* wtxEventpointDelete() returns WTX_OK. (The target server ignores the 
* deletion of a valid eventpoint ID that no longer exists on the target.)  
* If the target server eventpoint list does not contain the <evtptId>, then 
* the error WTX_ERR_SVR_INVALID_EVENTPOINT occurs.
* 
* RETURNS: WTX_OK or WTX_ERROR if the delete fails.
*
* SEE ALSO: WTX_EVENTPOINT_DELETE, wtxBreakpointAdd(), 
* wtxContextExitNotifyAdd(), wtxEventpointAdd(), wtxHwBreakpointAdd(), 
* wtxEventpointList() 
*/

STATUS wtxEventpointDelete 
    (
    HWTX	hWtx,		/* WTX API handle */
    UINT32	evtptId		/* ID of eventpoint to delete */
    )
    {
    WTX_MSG_RESULT	out;
    WTX_MSG_PARAM	in;
    WTX_ERROR_T		callStat;

    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    memset (&in, 0, sizeof (in));
    memset (&out, 0, sizeof (out));

    in.param.valueType = V_UINT32;
    in.param.value_u.v_uint32 = evtptId;
   
    callStat = exchange (hWtx, WTX_EVENTPOINT_DELETE, &in, &out);

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    wtxExchangeFree (hWtx->server, WTX_EVENTPOINT_DELETE, &out);

    return (WTX_OK);
    }

/*******************************************************************************
*
* wtxConsoleCreate - create a console window (UNIX only)
*
* This routine creates a new console window. The name passed in <name>
* is used as the title for the new window. If <name> is NULL, a unique 
* name is assigned by the target server.  When the target server is
* running in an X-Window environment, the <display> parameter
* identifies the X display device to create the window on. The number of
* buffered lines (default 88) can be changed by setting the environment variable
* `WTX_CONSOLE_LINES' to the number of desired buffered lines. Set this
* variable before launching the target server. The <fdIn>
* and <fdOut> parameters are pointers to INT32 variables in
* which are stored the target server file descriptors for the
* input and output of the new console.  wtxVioCtl() can be used
* to redirect VIO input and output to the console.
* 
* UNIX HOSTS
* On UNIX systems the returned ID is actually a process
* ID of the new console running on the same host as the target server.
*
* WINDOWS HOSTS
* This request is not implemented on Windows.  Windows allows only one virtual
* console and it must be started on the target server command line with the
* `-c' option.
*
* RETURNS: The ID number of the new console or WTX_ERROR.
*
* ERRORS:
* .iP WTX_ERR_API_INVALID_ARG 12
* Either <fdIn> or <fdOut> is NULL.
*
* SEE ALSO: WTX_CONSOLE_CREATE, wtxConsoleKill(), wtxVioCtl()
*/

INT32 wtxConsoleCreate
    (
    HWTX hWtx,			/* WTX API handle */
    const char *name,		/* Name of console window */
    const char *display,	/* Display name eg: host:0 */
    INT32 *fdIn,		/* RETURN: input file descriptor */
    INT32 *fdOut		/* RETURN: output file descriptor */
    )
    {
    WTX_MSG_CONSOLE_DESC	out;
    WTX_MSG_CONSOLE_DESC	in;
    WTX_ERROR_T			callStat;
    INT32			pid;

    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    if (fdIn == 0 || fdOut == 0)
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_INVALID_ARG, WTX_ERROR);

    memset (&in, 0, sizeof (in));
    memset (&out, 0, sizeof (out));

    in.wtxConsDesc.name = (char *) name;
    in.wtxConsDesc.display = (char *) display;
   
    callStat = exchange (hWtx, WTX_CONSOLE_CREATE, &in, &out);

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    *fdIn = out.wtxConsDesc.fdIn;
    *fdOut = out.wtxConsDesc.fdOut;
    pid = out.wtxConsDesc.pid;
    wtxExchangeFree (hWtx->server, WTX_CONSOLE_CREATE, &out);

    return pid;
    }


/*******************************************************************************
*
* wtxConsoleKill - destroy a console (UNIX only)
*
* This routine destroys a console identified by <consoleId> and created by 
* a previous call to wtxConsoleCreate().
* WINDOWS HOSTS
* This request is not implemented on Windows.  If issued, it returns an
* error, but does not attempt to kill an existing console.
*
* RETURNS: WTX_OK or WTX_ERROR.
*
* SEE ALSO: WTX_CONSOLE_KILL, wtxConsoleCreate() 
*/

STATUS wtxConsoleKill
    (
    HWTX hWtx,		/* WTX API handle */
    INT32 consoleId	/* id of console to destroy */
    )
    {
    WTX_MSG_RESULT	out;
    WTX_MSG_PARAM	in;
    WTX_ERROR_T		callStat;

    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    memset (&in, 0, sizeof (in));
    memset (&out, 0, sizeof (out));

    in.param.valueType = V_INT32;
    in.param.value_u.v_int32 = consoleId;
   
    callStat = exchange (hWtx, WTX_CONSOLE_KILL, &in, &out);

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    wtxExchangeFree (hWtx->server, WTX_CONSOLE_KILL, &out);

    return (WTX_OK);
    }

/*******************************************************************************
*
* wtxContextCont - continue execution of a target context
*
* This routine synchronously continues execution of the context <contextId>
* on the target after it has stopped at a breakpoint.  This routine should
* be used instead of wtxContextResume() when debugging a task to prevent
* the task hitting the breakpoint a second time.
*
* The context types are given by:
*
* EXPAND ../../../include/wtxtypes.h WTX_CONTEXT_TYPE
*
* RETURNS: WTX_OK or WTX_ERROR on failure.
*
* SEE ALSO: WTX_CONTEXT_CONT, wtxContextResume(), wtxBreakpointAdd()
*/

STATUS wtxContextCont 
    (
    HWTX		hWtx,		/* WTX API handle */
    WTX_CONTEXT_TYPE	contextType,	/* type of context to continue */
    WTX_CONTEXT_ID_T	contextId	/* id of context to continue */
    )
    {
    WTX_MSG_RESULT	out;
    WTX_MSG_CONTEXT	in;
    WTX_ERROR_T	callStat;
    
    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    memset (&out, 0, sizeof (out));
    memset (&in, 0, sizeof (in));

    in.contextType = contextType;
    in.contextId = contextId;

    callStat = exchange (hWtx, WTX_CONTEXT_CONT, &in, &out);

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    wtxExchangeFree (hWtx->server, WTX_CONTEXT_CONT, &out);
    
    return (WTX_OK);
    }


/*******************************************************************************
* 
* wtxContextCreate - create a new context on the target
*
* This routine creates a new context using the information in the
* context descriptor pointed to by <pContextDesc> which describes
* parameters for a new task context or a system context. The caller is
* responsible for freeing information supplied in the context
* descriptor after the call.
*
* EXPAND ../../../include/wtx.h WTX_CONTEXT_DESC
*
* ERRORS:
* .iP WTX_ERR_API_INVALID_ARG 12
* <pContextDesc> is invalid
*
* RETURNS: The ID of the new context or WTX_ERROR. 
*
* SEE ALSO: WTX_CONTEXT_CREATE, wtxContextKill()
*/

WTX_CONTEXT_ID_T wtxContextCreate 
    (
    HWTX		   hWtx,		/* WTX API handle */
    WTX_CONTEXT_DESC *	   pContextDesc		/* context descriptor */
    )

    {
    WTX_MSG_CONTEXT_DESC	in;
    WTX_MSG_CONTEXT		out;
    WTX_ERROR_T			callStat;
    WTX_CONTEXT_ID_T		ctxId;
    
    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    if (pContextDesc == NULL)
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_INVALID_ARG, WTX_ERROR);

    memset (&in, 0, sizeof (in));
    memset (&out, 0, sizeof (out));

    /* Fill in the context create descritor message. */

    /*
     * XXX - must have WTX_CONTEXT_DESC fields identical to those
     * WTX_MSG_CONTEXT_DESC from contextType onwards. Awaits change to
     * message definitions on wtxmsg.h so this hack will go away.
     */

    memcpy (&in.contextType, pContextDesc, sizeof (WTX_CONTEXT_DESC));
    
    callStat = exchange (hWtx, WTX_CONTEXT_CREATE, &in, &out);

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    ctxId = out.contextId;
    wtxExchangeFree (hWtx->server, WTX_CONTEXT_CREATE, &out);
	
    return ctxId;
    }

/*******************************************************************************
*
* wtxContextResume - resume execution of a target context
*
* This routine synchronously resumes execution of the context <contextId>
* on the target after it has been suspended, perhaps by wtxContextSuspend().
* Use wtxContextCont() rather than this routine to continue a task from a 
* breakpoint; this routine causes the task to hit the breakpoint again.
*
* The context types are given by:
*
* EXPAND ../../../include/wtxtypes.h WTX_CONTEXT_TYPE
*
* RETURNS: WTX_OK or WTX_ERROR on failure.
*
* SEE ALSO: WTX_CONTEXT_RESUME, wtxContextSuspend()
*/

STATUS wtxContextResume 
    (
    HWTX		hWtx,		/* WTX API handle */
    WTX_CONTEXT_TYPE	contextType,	/* type of context to resume */
    WTX_CONTEXT_ID_T	contextId	/* id of context to resume */
    )
    {
    WTX_MSG_RESULT	out;
    WTX_MSG_CONTEXT	in;
    WTX_ERROR_T		callStat;
    
    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    memset (&out, 0, sizeof (out));
    memset (&in, 0, sizeof (in));

    in.contextType = contextType;
    in.contextId = contextId;

    callStat = exchange (hWtx, WTX_CONTEXT_RESUME, &in, &out);

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    wtxExchangeFree (hWtx->server, WTX_CONTEXT_RESUME, &out);

    return (WTX_OK);
    }

/*******************************************************************************
*
* wtxContextStatusGet - get the status of a context on the target
*
* This routine returns the status of the context <contextId> on the target. 
*
* EXPAND ../../../include/wtxtypes.h WTX_CONTEXT_STATUS
*
* The context types are given by:
*
* EXPAND ../../../include/wtxtypes.h WTX_CONTEXT_TYPE
*
* RETURNS: The status of the context or WTX_ERROR.
*
* SEE ALSO: WTX_CONTEXT_STATUS_GET, wtxContextSuspend(), wtxContextResume()
*/

WTX_CONTEXT_STATUS wtxContextStatusGet 
    (
    HWTX		hWtx,		/* WTX API handle */
    WTX_CONTEXT_TYPE	contextType,	/* type of context */
    WTX_CONTEXT_ID_T	contextId	/* id of context */
    )
    {
    WTX_MSG_RESULT	out;
    WTX_CONTEXT_STATUS	result;		/* context status */
    WTX_MSG_CONTEXT	in;
    WTX_ERROR_T		callStat;
    
    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    memset (&out, 0, sizeof (out));
    memset (&in, 0, sizeof (in));

    in.contextType = contextType;
    in.contextId = contextId;

    callStat = exchange (hWtx, WTX_CONTEXT_STATUS_GET, &in, &out);

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    result = out.val.value_u.v_int32;
    wtxExchangeFree (hWtx->server, WTX_CONTEXT_STATUS_GET, &out);

    return (result);
    }

/*******************************************************************************
*
* wtxContextExitNotifyAdd - add a context exit notification eventpoint
*
* This routine creates a new eventpoint that notifies the user when the
* specified context exits by sending a WTX_EVT_CTX_EXIT event.  The eventpoint 
* is removed automatically when the context exits or the routine
* wtxEventpointDelete() can be used to remove the eventpoint before the
* context exits.
*
* When <contextType> is set to WTX_CONTEXT_SYSTEM, then only eventpoints 
* in system mode can be added. 
* When <contextType> is set to WTX_CONTEXT_TASK, then only eventpoints in 
* context task can be added.
*
* The context types are given by:
*
* EXPAND ../../../include/wtxtypes.h WTX_CONTEXT_TYPE
*
* RETURNS: A unique identifier for the new eventpoint or WTX_ERROR.
*
* SEE ALSO: WTX_EVENTPOINT_ADD, wtxEventpointAdd(), wtxEventpointList(), 
* wtxEventpointDelete()
*/

UINT32 wtxContextExitNotifyAdd 
    (
    HWTX		hWtx,		/* WTX API handle */
    WTX_CONTEXT_TYPE	contextType,	/* type of context */
    WTX_CONTEXT_ID_T	contextId	/* associated context */
    )
    {
    WTX_MSG_RESULT	out;
    WTX_MSG_EVTPT_DESC	in;
    WTX_ERROR_T		callStat;
    UINT32		result;
    
    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    memset (&out, 0, sizeof (out));
    memset (&in, 0, sizeof (in));

    in.wtxEvtpt.event.eventType = WTX_EVENT_CTX_EXIT;
    in.wtxEvtpt.context.contextType = contextType;
    in.wtxEvtpt.context.contextId = contextId;

    in.wtxEvtpt.action.actionType = WTX_ACTION_NOTIFY;

    callStat = exchange (hWtx, WTX_EVENTPOINT_ADD, &in, &out);

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    result = out.val.value_u.v_uint32; 
    wtxExchangeFree (hWtx->server, WTX_EVENTPOINT_ADD, &out);

    return result;
    }

/*******************************************************************************
*
* wtxContextKill - kill a target context
*
* This routine synchronously kills the context specified by
* <contextId> on the target.  
*
* The context types are given by:
*
* EXPAND ../../../include/wtxtypes.h WTX_CONTEXT_TYPE
*
* NOTE: Killing the the system context may result in a reboot of the
* target. However, the correct way to reboot the target system is by calling
* wtxTargetReset().
*
* RETURNS: WTX_OK or WTX_ERROR if the context kill fails.
*
* SEE ALSO: WTX_CONTEXT_KILL, wtxTargetReset() 
*/

STATUS wtxContextKill 
    (
    HWTX		hWtx,		/* WTX API handle */
    WTX_CONTEXT_TYPE	contextType,	/* type of context to kill */
    WTX_CONTEXT_ID_T	contextId	/* id of context to kill   */
    )
    {
    WTX_MSG_RESULT	out;
    WTX_MSG_CONTEXT	in;
    WTX_ERROR_T		callStat;
    
    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    memset (&out, 0, sizeof (out));
    memset (&in, 0, sizeof (in));

    in.contextType = contextType;
    in.contextId = contextId;

    callStat = exchange (hWtx, WTX_CONTEXT_KILL, &in, &out);

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    wtxExchangeFree (hWtx->server, WTX_CONTEXT_KILL, &out);

    return (WTX_OK);
    }


/*******************************************************************************
*
* wtxContextStep - single step execution of a target context
*
* This routine asynchronously single steps execution of a context on
* the target. Stepping occurs while <stepStart> <= PC < <stepEnd>.  If
* stepStart == stepEnd == 0 then the context is single stepped one
* machine instruction only.  Note that <stepStart> does not set the
* initial program counter; this must be done separately.
*
* The context types are given by:
*
* EXPAND ../../../include/wtxtypes.h WTX_CONTEXT_TYPE
*
* NOTE: Stepping is an asynchronous operation so the return of this
* function does not signify completion of the stepping. After stepping
* completes, a breakpoint event is generated at the address which
* terminated the stepping operation.
*
* RETURNS: WTX_OK or WTX_ERROR if the step fails.
*
* SEE ALSO: WTX_CONTEXT_STEP, wtxContextCont(), wtxContextSuspend()
*/

STATUS wtxContextStep 
    (
    HWTX	 	hWtx,		/* WTX API handle */
    WTX_CONTEXT_TYPE	contextType,	/* type of context to resume */
    WTX_CONTEXT_ID_T	contextId,	/* id of context to step */
    TGT_ADDR_T	 	stepStart,	/* stepStart pc value */
    TGT_ADDR_T   	stepEnd		/* stepEnd PC vale */
    )
    {
    WTX_MSG_RESULT		out;
    WTX_MSG_CONTEXT_STEP_DESC	in;
    WTX_ERROR_T		callStat;
    
    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    memset (&out, 0, sizeof (out));
    memset (&in, 0, sizeof (in));

    in.context.contextType = contextType;
    in.context.contextId = contextId;
    in.startAddr = stepStart;
    in.endAddr = stepEnd;

    callStat = exchange (hWtx, WTX_CONTEXT_STEP, &in, &out);

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    wtxExchangeFree (hWtx->server, WTX_CONTEXT_STEP, &out);

    return (WTX_OK);
    }


/*******************************************************************************
*
* wtxContextSuspend - suspend a target context
*
* This routine synchronously suspends execution of a context on the
* target.
*
* The context types are given by:
*
* EXPAND ../../../include/wtxtypes.h WTX_CONTEXT_TYPE
*
* RETURNS: WTX_OK or WTX_ERROR if the suspend fails.
*
* SEE ALSO: WTX_CONTEXT_SUSPEND
*/

STATUS wtxContextSuspend 
    (
    HWTX		hWtx,		/* WTX API handle */
    WTX_CONTEXT_TYPE	contextType,	/* type of context to suspend */
    WTX_CONTEXT_ID_T	contextId	/* id of context being suspended */
    )
    {
    WTX_MSG_RESULT	out;
    WTX_MSG_CONTEXT	in;
    WTX_ERROR_T		callStat;
    
    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    memset (&out, 0, sizeof (out));
    memset (&in, 0, sizeof (in));

    in.contextType = contextType;
    in.contextId =  contextId;

    callStat = exchange (hWtx, WTX_CONTEXT_SUSPEND, &in, &out);

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    wtxExchangeFree (hWtx->server, WTX_CONTEXT_SUSPEND, &out);

    return (WTX_OK);
    }


/*******************************************************************************
*
* wtxEventAdd - send an event to the target server
*
* This routine sends an event to the target server. The target server adds 
* it to the event list of any attached tool that has selected that event
* by using wtxRegisterForEvent() with a suitable event pattern.  The
* event is not sent back to the tool that generated it.
*
* EXPAND ../../../include/wtxmsg.h WTX_MSG_EVENT_DESC
*
* EXPAND ../../../include/wtxmsg.h WTX_EVENT_DESC
*
* RETURNS: WTX_OK or WTX_ERROR.
*
* SEE ALSO: WTX_EVENT_ADD, wtxEventGet(), wtxEventListGet(), 
* wtxRegisterForEvent(), wtxUnregisterForEvent().
*/

STATUS wtxEventAdd
    (
    HWTX	 hWtx,		/* WTX API handle */
    const char * event,		/* event string to send */
    UINT32       addlDataLen,	/* length of addl data block in bytes */
    const void * pAddlData	/* pointer to additional data */
    )
    {
    WTX_MSG_EVENT_DESC	in;
    WTX_MSG_RESULT	out;
    
    WTX_ERROR_T		callStat;

    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    if (addlDataLen != 0 && pAddlData == NULL)
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_INVALID_ARG, WTX_ERROR);

    if (event == NULL)
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_INVALID_ARG, WTX_ERROR);

    memset (&out, 0, sizeof (out));
    memset (&in, 0, sizeof (in));

    if ((in.eventDesc.event = (char *) malloc (strlen (event) + 1)) == NULL)
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_MEMALLOC, WTX_ERROR);
    
    strcpy (in.eventDesc.event, event);
    in.eventDesc.addlDataLen = (INT32) addlDataLen;

    if (addlDataLen != 0)
        {
        if ((in.eventDesc.addlData = (char *) malloc (addlDataLen)) == NULL)
	    {
	    free (in.eventDesc.event);
	    WTX_ERROR_RETURN (hWtx, WTX_ERR_API_MEMALLOC, WTX_ERROR);
	    }

        memcpy (in.eventDesc.addlData, pAddlData, addlDataLen);
        }
    else
        in.eventDesc.addlData = NULL;

    callStat = exchange (hWtx, WTX_EVENT_ADD, &in, &out);

    if (callStat != WTX_ERR_NONE)
	{
	if (in.eventDesc.addlDataLen != 0)
	    free (in.eventDesc.addlData);
	free (in.eventDesc.event);

        WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);
	}

    if (in.eventDesc.addlDataLen != 0)
	free (in.eventDesc.addlData);
    free (in.eventDesc.event);

    return (WTX_OK);
    }


/*******************************************************************************
*
* wtxEventGet - get an event from the target
*
* This routine polls the target server for any events sent to this
* client tool. It only returns one event at a time.  The event
* descriptor returned is a string containing the primary event
* data, a length count of additional binary data and a pointer to the
* additional data.  The memory allocated for the event-descriptor
* result must be freed by the user using wtxResultFree(). Note that an
* event descriptor is always returned even when no real event is ready
* for the tool. In this case the event descriptor is WTX_EVENT_OTHER, 
* `addlDataLen' is 0, and `addlData' is NULL.
*
* EXPAND ../../../include/wtxmsg.h WTX_EVENT_DESC
*
* RETURNS: A pointer to the descriptor for the event received or NULL.
*
* SEE ALSO: WTX_EVENT_GET, wtxEventAdd(), wtxEventListGet(), 
* wtxResultFree(), wtxStrToEventType(), wtxStrToInt(), wtxStrToTgtAddr()
*/

WTX_EVENT_DESC * wtxEventGet
    (
    HWTX		hWtx		/* WTX API handle */
    )
    {
    WTX_ERROR_T			callStat;
    WTX_MSG_EVENT_DESC *	pOut;

    WTX_CHECK_CONNECTED (hWtx, NULL);

    pOut = calloc (1, sizeof (WTX_MSG_EVENT_DESC));

    if (pOut == NULL)
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_MEMALLOC, NULL);

    callStat = exchange (hWtx, WTX_EVENT_GET, &hWtx->msgToolId, pOut);

    if (callStat != WTX_ERR_NONE)
	{
	free (pOut);	/* Free allocated message */
	WTX_ERROR_RETURN (hWtx, callStat, NULL);
	}

    /* add pointer to the wtxResultFree () list */

    if (wtxFreeAdd (hWtx, (void *) &pOut->eventDesc, NULL, pOut, WTX_EVENT_GET,
		    hWtx->server, WTX_SVR_SERVER) != WTX_OK)
	{
	WTX_ERROR_RETURN (hWtx, wtxErrGet (hWtx), NULL);
	}

    return (&pOut->eventDesc);
    }

/******************************************************************************
*
* wtxEventListGet - get all the events in one call.
*
* This function flushes the target server events queue. If more than one 
* event is found, WTX_EVENT_NODE structure is filled with the events found
* up to the limit fixed by <nEvents>. If <nEvents> is equal to 1, then this
* function is equivalent to wtxEventGet(). If all the events will have to be
* flushed, then <nEvents> must be equal to WTX_ALL_EVENTS.
*
* WTX_EVENT_NODE contains a WTX_EVENT_DESC structure and a pointer to the
* next WTX_EVENT_NODE (or NULL if it is the last event).
*
* EXPAND ../../../include/wtxmsg.h WTX_EVENT_NODE
* EXPAND ../../../include/wtxmsg.h WTX_EVENT_DESC
*
* NOTE : The result of the wtxEventListGet() call should be freed by a call to
* wtxResultFree ().
*
* RETURN: A pointer to the event list or NULL if there's no events or if
* something failed (errCode must be checked).
*
* SEE ALSO: WTX_EVENT_LIST_GET, wtxEventGet(), wtxEventAdd(), 
* wtxRegisterForEvent(), wtxUnregisterForEvent(), wtxResultFree()
*/

WTX_EVENT_NODE * wtxEventListGet
    (
    HWTX                hWtx,           /* WTX API handle */
    UINT32		nEvents		/* Number of events to return */
    )
    {
    WTX_ERROR_T          callStat;
    WTX_MSG_PARAM        in;
    WTX_MSG_EVENT_LIST * pOut;

    WTX_CHECK_CONNECTED (hWtx, NULL);

    memset (&in, 0, sizeof (in));

    in.param.valueType = V_UINT32;
    in.param.value_u.v_uint32 = nEvents;

    /* Output parameter location */

    pOut = calloc (1, sizeof (WTX_MSG_EVENT_LIST));
    if (pOut == NULL)
        WTX_ERROR_RETURN (hWtx, WTX_ERR_API_MEMALLOC, NULL);

    callStat = exchange (hWtx, WTX_EVENT_LIST_GET, &in, pOut);

    if (callStat != WTX_ERR_NONE)
        {
        free (pOut);                   /* Free allocated message */
        WTX_ERROR_RETURN (hWtx, callStat, NULL);
        }

    /* Output parameter */

    /* add pointer to the wtxResultFree () list */

    if (wtxFreeAdd (hWtx, (void *) pOut->pEventList, NULL, pOut,
		    WTX_EVENT_LIST_GET, hWtx->server, WTX_SVR_SERVER) != WTX_OK)
	{
	WTX_ERROR_RETURN (hWtx, wtxErrGet (hWtx), NULL);
	}

    return (pOut->pEventList);
    }

/*******************************************************************************
*
* wtxEventpointList - list eventpoints on the target server
*
* This routine returns a pointer to a list of all eventpoints managed
* by the target server. These include breakpoints and context exit
* notifications. The list returned is dynamically allocated and should
* be freed by calling wtxResultFree()
*
* EXPAND ../../../include/wtx.h WTX_EVTPT_LIST
*
* RETURNS: A pointer to a list of eventpoints or NULL on error.
*
* SEE ALSO: WTX_EVENTPOINT_LIST, wtxEventpointAdd(), wtxBreakpointAdd(), 
* wtxContextExitNotifyAdd()
*/

WTX_EVTPT_LIST * wtxEventpointList 
    (
    HWTX hWtx		/* WTX API handle */
    )
    {
    WTX_MSG_EVTPT_LIST * pOut;
    WTX_ERROR_T		 callStat;

    WTX_CHECK_CONNECTED (hWtx, NULL);

    pOut = calloc (1, sizeof (WTX_MSG_EVTPT_LIST));    
    if (pOut == NULL)
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_MEMALLOC, NULL);

    callStat = exchange (hWtx, WTX_EVENTPOINT_LIST, &hWtx->msgToolId, pOut);

    if (callStat != WTX_ERR_NONE)
	{
	free (pOut);
	WTX_ERROR_RETURN (hWtx, callStat, NULL);
	}

    /* add pointer to the wtxResultFree () list */

    if (wtxFreeAdd (hWtx, (void *) &pOut->nEvtpt, NULL, pOut,
		    WTX_EVENTPOINT_LIST, hWtx->server, WTX_SVR_SERVER)
	!= WTX_OK)
	{
	WTX_ERROR_RETURN (hWtx, wtxErrGet (hWtx), NULL);
	}

    return (WTX_EVTPT_LIST *) &pOut->nEvtpt;
    }

/*******************************************************************************
*
* wtxEventpointListGet - list eventpoints on the target server
*
* This routine returns a pointer to a list of eventpoints info: eventpoints
* managed by the target server, the tool identifier and the eventpoint
* identifier.
*
* EXPAND ../../../include/wtx.h WTX_EVTPT_LIST_2
*
* NOTE : The result of a call to wtxEventpointListGet() should be freed via
* wtxResultFree()
*
* RETURNS: A pointer to an eventpoints info list or NULL on error.
*
* SEE ALSO: WTX_EVENTPOINT_LIST_GET, wtxEventpointList(), wtxBreakpointAdd(),
* wtxContextExitNotifyAdd(), wtxHwBreakpointAdd(), wtxEventpointAdd(),
* wtxResultFree()
*/

WTX_EVTPT_LIST_2 * wtxEventpointListGet
    (
    HWTX hWtx           /* WTX API handle */
    )
    {
    WTX_MSG_EVTPT_LIST_2 *	pOut;
    WTX_ERROR_T			callStat;

    WTX_CHECK_CONNECTED (hWtx, NULL);

    pOut = calloc (1, sizeof (WTX_MSG_EVTPT_LIST_2));
    if (pOut == NULL)
        WTX_ERROR_RETURN (hWtx, WTX_ERR_API_MEMALLOC, NULL);

    callStat = exchange (hWtx, WTX_EVENTPOINT_LIST_GET, &hWtx->msgToolId, pOut);

    if (callStat != WTX_ERR_NONE)
        {
        free (pOut);
        WTX_ERROR_RETURN (hWtx, callStat, NULL);
        }

    /* add pointer to the wtxResultFree () list */

    if (wtxFreeAdd (hWtx, (void *) &pOut->nEvtpt, NULL, pOut,
		    WTX_EVENTPOINT_LIST_GET, hWtx->server, WTX_SVR_SERVER)
	!= WTX_OK)
	{
	WTX_ERROR_RETURN (hWtx, wtxErrGet (hWtx), NULL);
	}

    return (WTX_EVTPT_LIST_2 *) &pOut->nEvtpt;
    }

/*******************************************************************************
*
* wtxFileClose - close a file on the target server
*
* This routine closes the file previously opened in a call of
* wtxFileOpen() and cancels any VIO redirection caused by that open.
*
* RETURNS: WTX_OK or WTX_ERROR.
*
* SEE ALSO: WTX_CLOSE, wtxFileOpen() 
*/

STATUS wtxFileClose 
    (
    HWTX	hWtx,		/* WTX API handle */
    INT32	fileDescriptor	/* file to close descriptor */
    )
    {
    WTX_MSG_RESULT	out;
    WTX_MSG_PARAM	in;
    WTX_ERROR_T		callStat;

    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    memset (&out, 0, sizeof (out));
    memset (&in, 0, sizeof (in));
   
    in.param.valueType = V_INT32;
    in.param.value_u.v_int32 = fileDescriptor;

    callStat = exchange (hWtx, WTX_CLOSE, &in, &out);

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    wtxExchangeFree (hWtx->server, WTX_CLOSE, &out);

    return (WTX_OK);
    }


/*******************************************************************************
*
* wtxFileOpen - open a file on the target server
*
* This routine requests that the target server open a file in the host
* environment and optionally redirects I/O from a VIO channel to it. The
* user may select which VIO channel is redirected to and from the file by
* specifying the <channel> argument. 
*
* Valid values for the <flags> parameter can be:
* 
* EXPAND  ../../../include/wtxtypes.h WTX_OPEN_FLAG
*
* or host specific flags (O_RDONLY etc...) as defined in sys/fcntl.h. The 
* latter are accepted for backward compatibility but should no longer be 
* used.
*
* WINDOWS HOSTS
* If target server runs on a WINDOWS host, the RDONLY flags are not supported.
* If an RDONLY flag is given for <flags> variable, a WTX_ERR_SVR_INVALID_FLAGS
* error will be returned. This is due to the lack of pipe opening on WINDOWS
* hosts.
*
* NOTE: <fileName> must specify a file accessible from the host where the
* target server is running.
*
* RETURNS: A file descriptor on success or WTX_ERROR if the open fails.
*
* SEE ALSO: WTX_OPEN, wtxFileClose()
*/

INT32 wtxFileOpen 
    (
    HWTX	  hWtx,		/* WTX API handle */
    const char *  fileName,	/* file name */
    WTX_OPEN_FLAG flags,	/* unix style flags */
    INT32	  mode,		/* unix style mode */
    INT32	  channel	/* channel id for redirection */
    )
    {
    WTX_ERROR_T		callStat;
    WTX_MSG_OPEN_DESC	in;
    WTX_MSG_RESULT	out;	
    INT32		fileId;

    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    memset (&in, 0, sizeof (WTX_MSG_OPEN_DESC));
    memset (&out, 0, sizeof (WTX_MSG_RESULT));

    in.filename = (char *) fileName;
    in.flags = flags;
    in.mode = mode;
    in.channel = channel;

    callStat = exchange (hWtx, WTX_OPEN, &in, &out);

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    fileId = out.val.value_u.v_int32;

    wtxExchangeFree (hWtx->server, WTX_OPEN, &out);

    return fileId;
    }

/*******************************************************************************
*
* wtxHwBreakpointAdd - create a new WTX eventpoint of type hardware breakpoint
*
* This routine creates a new eventpoint on the target to represent a
* hardware breakpoint of type <type> at the address <tgtAddr> for the 
* target context <contextId>. The target server maintains a list of 
* eventpoints created on the target and this can be queried using 
* wtxEventpointList().  When a context is destroyed on the target or the 
* target is reset, eventpoints are deleted automatically without 
* intervention from the creator.
*
* When <contextType> is set to WTX_CONTEXT_SYSTEM, then only eventpoints in 
* system mode can be added. 
* When <contextType> is set to WTX_CONTEXT_TASK, then only eventpoints in 
* context task can be added.
*
* The context types are given by:
*
* EXPAND ../../../include/wtxtypes.h WTX_CONTEXT_TYPE
*
* NOTE
* The types of hardware breakpoints vary with the architectures.
* Generally, a hardware breakpoint can be a data breakpoint or an
* instruction breakpoint.
*
* RETURNS: The ID of the new breakpoint or WTX_ERROR if the add fails.
*
* SEE ALSO: WTX_EVENTPOINT_ADD_2, wtxEventpointAdd(), wtxEventpointDelete(),
* wtxEventpointList(), wtxBreakpointAdd()
*/

UINT32 wtxHwBreakpointAdd 
    (
    HWTX		hWtx,		/* WTX API handle */
    WTX_CONTEXT_TYPE	contextType,	/* type of context to put bp in */
    WTX_CONTEXT_ID_T	contextId,	/* associated context */
    TGT_ADDR_T		tgtAddr,	/* breakpoint address */
    TGT_INT_T		type		/* access type (arch dependant) */
    )
    {
    WTX_MSG_RESULT		out;
    WTX_MSG_EVTPT_DESC_2	in;
    WTX_ERROR_T			callStat;
    UINT32			hwBpId;		/* hardware breakpoint ID */
    TGT_ARG_T			args[3];
    
    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    memset (&out, 0, sizeof (out));
    memset (&in, 0, sizeof (in));

    /* fill event field */

    in.wtxEvtpt.event.eventType = WTX_EVENT_HW_BP;
    in.wtxEvtpt.event.args = args;
    in.wtxEvtpt.event.numArgs = (TGT_ARG_T) 3;	/* Number of arguments  */
    in.wtxEvtpt.event.args[0] = (TGT_ARG_T) tgtAddr;	/* bkpt address */
    in.wtxEvtpt.event.args[1] = (TGT_ARG_T) 0;		/* bkpt count */	
    in.wtxEvtpt.event.args[2] = (TGT_ARG_T) type;	/* hw bkpt type */	

    /* fill context field */

    in.wtxEvtpt.context.contextType = contextType;
    in.wtxEvtpt.context.contextId = contextId;

    /* fill action field */

    in.wtxEvtpt.action.actionType = WTX_ACTION_STOP|WTX_ACTION_NOTIFY; 
    in.wtxEvtpt.action.actionArg = 0;
    in.wtxEvtpt.action.callRtn = 0;
    in.wtxEvtpt.action.callArg = 0;

    callStat = exchange (hWtx, WTX_EVENTPOINT_ADD_2, &in, &out);

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    hwBpId = out.val.value_u.v_uint32; 
    wtxExchangeFree (hWtx->server, WTX_EVENTPOINT_ADD_2, &out);

    return hwBpId;
    }

/*******************************************************************************
*
* wtxSymListFree - free memory used to store symbols in a list
*
* This routine frees the memory associated with each node in a symbol list.
*
* The symbol list to free is defined by:
*
* EXPAND ../../../include/wtxmsg.h WTX_SYM_LIST
* EXPAND ../../../include/wtxmsg.h WTX_SYMBOL
*
* RETURNS: N/A
*/
 
void wtxSymListFree
    (
    WTX_SYM_LIST *	pSymList	/* symbol list to free */
    )
    {
    WTX_SYMBOL *	pCurSym;	/* current symbol in list */
    WTX_SYMBOL *	pNextSym;	/* next symbol in list */

    if (pSymList == NULL)
	return;
 
    pCurSym = pSymList->pSymbol;	/* get first node in list */
 
    if(pCurSym == 0)			/* list is empty nothing to free */
	return;
 
    pNextSym = (WTX_SYMBOL *) pCurSym->next;	/* get next node pointer */

    if (pCurSym->name != NULL)
	{
	free (pCurSym->name);			/* free name space */
	}

    if (pCurSym->moduleName != NULL)
	{
	free (pCurSym->moduleName);		/* free module name space */
	}
    free (pCurSym);				/* free first node space */
 
    while(pNextSym != NULL)
	{
	pCurSym  = pNextSym;		/* go to next node */
	pNextSym = (WTX_SYMBOL *) pCurSym->next;
 
	if (pCurSym->name != NULL)
	    {
	    free (pCurSym->name);		/* free name space */
	    }

	if (pCurSym->moduleName != NULL)
	    {
	    free (pCurSym->moduleName);	/* free module name space */
	    }

	free (pCurSym);			/* free list node space */
	}
    }

/*******************************************************************************
* 
* wtxResultFree - free the memory used by a WTX API call result
*
* This routine frees the memory allocated for the result of a WTX API
* call.  <pResult> must point to a WTX structure that was returned by
* a WTX API call.
*
* The routine always returns WTX_OK, except if the WTX handler is invalid. This
* is to avoid overriding previously set WTX errors.
*
* NOTE: To ensure correct and complete freeing of memory allocated to
* API call results, use wtxResultFree() to free target server call
* results before calling wtxToolDetach() and to free registry call results 
* before calling wtxTerminate().
*
* RETURNS: WTX_OK or WTX_ERROR if the WTX handler is invalid.
*/

STATUS wtxResultFree
    (
    HWTX		hWtx,			/* WTX session handler     */
    void *		pToFree			/* pointer to be freed     */
    )
    {
    WTX_FREE_NODE *	pWtxFreeNode = NULL;	/* wtxResultFree node desc */

    WTX_CHECK_HANDLE (hWtx, WTX_ERROR);

    /* first check that the pointer is not NULL */

    if ((pToFree == NULL) || (hWtx->pWtxFreeList == NULL) )
	{
	goto error;
	}

    /* now we can look in the list */

    if ( (pWtxFreeNode = (WTX_FREE_NODE *) sllEach (hWtx->pWtxFreeList,
						    wtxPtrMatch, (int) pToFree))
	 == NULL)
	{
	goto error;
	}

    /*
     * the pointer pToFree has been found in list, free it by calling the
     * associated routine
     * if pWtxFreeNode->pFreeFunc is set to NULL, it means that we have to
     * free pWtxFreeNode->pToFree using the RPC free routines
     */

    if (pWtxFreeNode->pFreeFunc != NULL)
	{
	pWtxFreeNode->pFreeFunc (pWtxFreeNode->pToFree);
	}
    else
	{
	if (pWtxFreeNode->server != NULL)
	    {
	    wtxExchangeFree (pWtxFreeNode->server, pWtxFreeNode->svcNum,
			     pWtxFreeNode->pMsgToFree);
	    }
	}

    /* if the pMsgToFree pointer is not NULL, we have to free it too */

    if ( pWtxFreeNode->pMsgToFree != NULL)
	{
	free (pWtxFreeNode->pMsgToFree);
	}

    sllRemove (hWtx->pWtxFreeList, (SL_NODE *) pWtxFreeNode,
	       sllPrevious ( (SL_LIST *) hWtx->pWtxFreeList, 
			     (SL_NODE *) pWtxFreeNode));
    free (pWtxFreeNode);

error:
    return (WTX_OK);
    }

/*******************************************************************************
*
* wtxFreeAdd - adds an element in the wtxResultFree() list
*
* This routine adds a new element in the wtxResultFree() list. It creates a new
* element in the list. The given <pToFree> and <pFreeFunc> are assigned to the
* new elemetn <pToFree> and <pFreeFunc> fields.
*
* .iP hWtx 10 3
* The WTX session handler. It allows this routine to return an error message if
* one request fails.
* .iP pToFree
* Basically the pointer that is to be freed by a call to <pFreeFunc>
* .iP pFreeFunc
* The address of the routine to use to free <pToFree>
* .iP pMsgToFree
* The <pToFree> pointer may be embedded inside a WTX message. In that case, the
* message also has to be freed. The free() routine is used to free <pMsgToFree>.
* If <pToFree> is not embedded inside a WTX message, <pMsgToFree> should be set
* to NULL.
* .iP svcNum
* When <pToFree> uses the RPC free routines, this variable should be set to the
* WTX_REQUEST number of the service that allocated this RPC message. This
* variable will be evaluated only if the <pFreeFunc> routine pointer is set to
* NULL.
* .iP server
* If <svcNum> is evaluated, the <server> variable specifies the RPC server to
* connect to.
* .lE
*
* RETURNS: WTX_OK or WTX_ERROR
*
*/

STATUS wtxFreeAdd
    (
    HWTX		hWtx,		/* wtx session handler            */
    void *		pToFree,	/* pointer to be freed            */
    FUNCPTR		pFreeFunc,	/* routine to use to free pToFree */
    void *		pMsgToFree,	/* message to free if needed      */
    WTX_REQUEST		svcNum,		/* service num to free if needed  */
    WTX_XID		server,		/* RPC server to connect to       */
    WTX_SVR_TYPE	svrType		/* server to connect type         */
    )
    {
    WTX_FREE_NODE *	pFreeNode = NULL;	/* new node in list       */
    STATUS		status = WTX_ERROR;	/* routine return status  */

    /* initialize the wtxResultFree list if needed */

    if (hWtx->pWtxFreeList == NULL)
	{
	if ( (hWtx->pWtxFreeList = sllCreate ()) == NULL)
	    {
	    wtxErrSet (hWtx, WTX_ERR_API_MEMALLOC);
	    goto error;
	    }
	}

    /*
     * add the pointer in the wtxResultFree list so it can be freed later
     * calling wtxResultFree()
     */

    if (pToFree == NULL)
	{
	/*
	 * pointer to free is NULL, just don't add it to the list, in order to
	 * not break the user's application, we just exit and send the WTX_OK
	 * status. There should not be any problem to do it.
	 */

	status = WTX_OK;
	goto error;
	}

    /* allocate room for the new pointer description in the queue */

    if ( (pFreeNode = calloc (1, sizeof (WTX_FREE_NODE))) == NULL)
	{
	wtxErrSet (hWtx, WTX_ERR_API_MEMALLOC);
	goto error;
	}

    pFreeNode->pToFree = pToFree;
    pFreeNode->pFreeFunc = pFreeFunc;
    pFreeNode->pMsgToFree = pMsgToFree;
    pFreeNode->svcNum = svcNum;
    pFreeNode->server = server ;
    pFreeNode->svrType = svrType;

    sllPutAtTail ( (SL_LIST *) hWtx->pWtxFreeList, (SL_NODE *) pFreeNode);

    status = WTX_OK;

error:
    return (status);
    }

/*******************************************************************************
*
* wtxPtrMatch - matches with a pointer value
*
* This routine is used by wtxResultFree() to match for the pointer to free in
* the SL_LIST.
*
* RETURNS: TRUE if it matches, FALSE if different.
*
* NOMANUAL
*/

LOCAL BOOL wtxPtrMatch
    (
    WTX_FREE_NODE *	pWtxFreeNode,	/* element in singaly linked list */
    void *		pToMatch	/* pointer value to find */
    )
    {
    if (pToMatch == pWtxFreeNode->pToFree)
	{
	return (FALSE);
	}
    else
	{
	return (TRUE);
	}
    }

/*******************************************************************************
*
* wtxResultFreeListTerminate - empty the wtxResultFree pointers list
*
* This routine is called by wtxTerminate() to free each of the remaining
* pointers in the wtxResultFree() pointers list.
*
* RETURNS: TRUE in order to free all the nodes
*
* NOMANUAL
*/

LOCAL BOOL wtxResultFreeListTerminate
    (
    WTX_FREE_NODE *	pWtxFreeNode,	/* element in singaly linked list */
    HWTX *		hWtx		/* WTX session handler            */
    )
    {
    wtxResultFree ( (HWTX) (*hWtx), pWtxFreeNode->pToFree);
    return (TRUE);
    }

/*******************************************************************************
*
* wtxResultFreeServerListTerminate - empty the wtxResultFree list for tgtsvr
*
* This routine is called by wtxToolDetach() to free each of the remaining
* target server pointers in the wtxResultFree() pointers list.
*
* RETURNS: TRUE in order to free all the nodes
*
* NOMANUAL
*/

LOCAL BOOL wtxResultFreeServerListTerminate
    (
    WTX_FREE_NODE *	pWtxFreeNode,	/* element in singaly linked list */
    HWTX *		hWtx		/* WTX session handler            */
    )
    {
    if (pWtxFreeNode->svrType == WTX_SVR_SERVER)
	{
	wtxResultFree ( (HWTX) (*hWtx), pWtxFreeNode->pToFree);
	}
    return (TRUE);
    }

/*******************************************************************************
*
* wtxGopherEval - evaluate a Gopher string on the target 
*
* This routine evaluates the Gopher string <inputString> and populates
* the Gopher result tape WTX_GOPHER_TAPE with the result
* data. It is the caller's duty to free the result tape by calling
* wtxResultFree().
*
* EXPAND ../../../include/wtxmsg.h WTX_GOPHER_TAPE
*
* The Gopher result tape is a byte-packed (opaque) data stream.  The data is a 
* series of pairs, each consisting of a type
* code and its associated data.  The type codes are defined in
* the file .../wpwr/share/src/agents/wdb/wdb.h as follows:
*
* .CS
*       /@ gopher stream format type codes @/
*
* #define GOPHER_UINT32           0
* #define GOPHER_STRING           1
* #define GOPHER_UINT16           2
* #define GOPHER_UINT8            3
* #define GOPHER_FLOAT32          4
* #define GOPHER_FLOAT64          5
* #define GOPHER_FLOAT80          6
* .CE
*
* For an example showing how to format the Gopher result tape, see
* .I API Programmer's Guide: The WTX Protocol.
*
* RETURNS: A pointer to the Gopher result tape or NULL on error.
*
* SEE ALSO: WTX_GOPHER_EVAL,
* .I API Reference Manual: WTX Protocol WTX_GOPHER_EVAL,
* .I API Programmer's Guide: The WTX Protocol
*/

WTX_GOPHER_TAPE * wtxGopherEval 
    (
    HWTX		  hWtx,		/* WTX API handle */
    const char *	  inputString	/* Gopher program to evaluate */
    )
    {
    WTX_MSG_PARAM		in;
    WTX_MSG_GOPHER_TAPE *	pOut;
    WTX_ERROR_T			callStat;

    WTX_CHECK_CONNECTED (hWtx, NULL);

    pOut = calloc (1, sizeof (WTX_MSG_GOPHER_TAPE));
    if (pOut == NULL)	
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_MEMALLOC, NULL);

    memset (&in, 0, sizeof (in));

    in.param.valueType = V_PCHAR;
    in.param.value_u.v_pchar = (char *) inputString;

    callStat = exchange (hWtx, WTX_GOPHER_EVAL, &in, pOut);

    if (callStat != WTX_ERR_NONE)
	{
	free (pOut);	/* Free allocated message */
	WTX_ERROR_RETURN (hWtx, callStat, NULL);
	}

    /* add pointer to the wtxResultFree () list */

    if (wtxFreeAdd (hWtx, (void *) &pOut->tape, NULL, pOut, WTX_GOPHER_EVAL,
		    hWtx->server, WTX_SVR_SERVER) != WTX_OK)
	{
	WTX_ERROR_RETURN (hWtx, wtxErrGet (hWtx), NULL);
	}

    return (&pOut->tape);
    }

/*******************************************************************************
*
* wtxLogging - WDB / WTX requests logging controls
*
* This function allows control of WDB / WTX requests logging. 
* 
* Options available for the WTX log request are:
* .iP LOG_ON
* Start the logging service
* .iP LOG_OFF
* Stop the logging service
* .iP filename
* Name where info will be written
* .iP maxSize
* Maximum size of the log file
* .iP regExp
* Regular expression used to filter events
*
* Options available for the WDB log request are:
* .iP LOG_ON
* Start the logging service
* .iP LOG_OFF
* Stop the logging service
* .iP filename
* Name where info will be written
* .iP maxSize
* Maximum size of the log file
*
* By default, there's no maximum size (<maxSize> is set to WTX_LOG_NO_LIMIT) 
* and <regExp> is set to "WTX_EVENT_GET".
*
* EXAMPLE:
*
* Start the WTX logging with a file never bigger than 10000 bytes and 
* containing all WTX requests except "WTX_TOOL_ATTACH".
*
* .CS
*   wtxLogging (
*              hWtx,                          /@ WTX API handle @/
*              LOG_WTX,                       /@ WTX logging    @/
*              LOG_ON,                        /@ start logging  @/
*              "/folk/pascal/wtx.txt",        /@ log file name  @/
*              10000,                         /@ log max size   @/
*              "WTX_TOOL_ATTACH"              /@ never printed  @/
*              );
* .CE
*
* Start the WTX logging without size limit and with a default WTX 
* requests filter.
*
* .CS
*   wtxLogging (
*              hWtx,                          /@ WTX API handle @/
*              LOG_WTX,                       /@ WTX logging    @/
*              LOG_ON,                        /@ start logging  @/
*              "/folk/pascal/wtx.txt",        /@ log file name  @/
*              WTX_LOG_NO_LIMIT,              /@ no max size    @/
*              NULL                           /@ default filter @/
*              );
* .CE
*
* Stop the WTX and the WDB logging in one call: <fileName>, <maxSize> and 
* <filter> are not used in this case and may be set to a default value.
* 
* .CS
*   wtxLogging (
*              hWtx,                          /@ WTX API handle @/
*              LOG_ALL,                       /@ WTX WDB log    @/
*              LOG_OFF,                       /@ stop logging   @/
*              NULL,                          /@ not used       @/
*              WTX_LOG_NO_LIMIT,              /@ not used       @/
*              NULL                           /@ not used       @/
*              );
* .CE
*
* RETURNS: WTX_OK or WTX_ERROR
*
* ERRORS:
* .iP WTX_ERR_API_INVALID_ARG 12
* <fileName> is NULL or an argument is wrong
*
* SEE ALSO: WTX_COMMAND_SEND, 
*/

STATUS wtxLogging
    (
    HWTX        hWtx,           /* WTX API handle                      */
    int         type,           /* LOG_WDB, LOG_WTX or LOG_ALL logging */
    int         action,         /* Logging LOG_ON or LOG_OFF           */
    char *      fileName,       /* Logging file name                   */
    int         maxSize,        /* log file max size                   */
    char *      filter          /* Filter for the log file.            */
    )
    {
    WTX_MSG_RESULT  out;
    WTX_MSG_PARAM   in;
    char            commandString [256];

    WTX_ERROR_T     callStat;

    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    memset (&out, 0, sizeof (out));
    memset (&in, 0, sizeof (in));

    /* Is it a WTX_LOGGING request ? */

    if (type == LOG_WTX)
        {
        /* We want to activate the logging facilities */

        if (action == LOG_ON)
            {
	    /* Test the arguments */

            if (fileName == NULL)
                WTX_ERROR_RETURN (hWtx, WTX_ERR_API_INVALID_ARG, WTX_ERROR);

            if (maxSize < 0)
                WTX_ERROR_RETURN (hWtx, WTX_ERR_API_INVALID_ARG, WTX_ERROR);

            if (filter == NULL)
                { 
                /* We have the max size */

                sprintf (commandString, "%s%s%s%s%d", WTX_LOGGING_ON_CMD, 
                         SEPARATOR, fileName, SEPARATOR, maxSize);
                }
            else
                {
                /* We have a filter for the log file */

                sprintf (commandString, "%s%s%s%s%d%s%s", WTX_LOGGING_ON_CMD,
                         SEPARATOR, fileName, SEPARATOR, maxSize, SEPARATOR,
                         filter);
                }
            }
        else
            {
            /* We want to stop the logging facilities */

            if (action == LOG_OFF)
                {
                sprintf (commandString, "%s", WTX_LOGGING_OFF_CMD);
                }

            /* This is an error */

            else
                {
		WTX_ERROR_RETURN (hWtx, WTX_ERR_API_INVALID_ARG, WTX_ERROR);
                }
            }
        }
    else
        {
        /* Is it a WDB_LOGGING request ? */

        if (type == LOG_WDB)
            {
            /* We want to activate the logging facilities */

            if (action == LOG_ON)
                {
		/* Test the arguments */

                if (fileName == NULL)
                    WTX_ERROR_RETURN (hWtx, WTX_ERR_API_INVALID_ARG, WTX_ERROR);

                if (maxSize < 0)
                    WTX_ERROR_RETURN (hWtx, WTX_ERR_API_INVALID_ARG, WTX_ERROR); 

                /* We have the max size */

                sprintf (commandString, "%s%s%s%s%d", WDB_LOGGING_ON_CMD,
                         SEPARATOR, fileName, SEPARATOR, maxSize);
                }
            else
                {
                /* We want to stop the logging facilities */

                if (action == LOG_OFF)
                    {
                    sprintf (commandString, "%s", WDB_LOGGING_OFF_CMD);
                    }

                /* This is an error */

                else
                    {
		    WTX_ERROR_RETURN (hWtx, WTX_ERR_API_INVALID_ARG, WTX_ERROR);
                    }
                }
            }
        else
            {
            /* Is it a WTX_LOGGING & WDB_LOGGING <LOG_OFF> request ? */

            if (type == LOG_ALL)
                {
                /* If this is <LOG_OFF>: OK */

                if (action == LOG_OFF)
                    {
                    sprintf (commandString, "%s", ALL_LOGGING_OFF_CMD);
                    }
                else
                    {
                    /* This is an error */

		    WTX_ERROR_RETURN (hWtx, WTX_ERR_API_INVALID_ARG, WTX_ERROR);
                    }
                }
            else
                {
                /* Otherwise: error */

		WTX_ERROR_RETURN (hWtx, WTX_ERR_API_INVALID_ARG, WTX_ERROR);
                }
            }
        }

    in.param.valueType = V_PCHAR;
    in.param.value_u.v_pchar = commandString;

    callStat = exchange (hWtx, WTX_COMMAND_SEND, &in, &out);

    if (callStat != WTX_ERR_NONE)
        WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    wtxExchangeFree (hWtx->server, WTX_COMMAND_SEND, &out);

    /* Tchao */

    return (WTX_OK);
    }

/*******************************************************************************
* 
* wtxMemInfoGet - get information about the target-server-managed memory pool
*
* This routine queries the target server to find out information about
* the target-server-managed target memory pool. The result is a pointer
* to a WTX_MEM_INFO structure that must be freed by a user call to 
* wtxResultFree().
*
* EXPAND ../../../include/wtx.h WTX_MEM_INFO
*
* RETURNS: Pointer to the module info or NULL on failure. 
*
* SEE ALSO: WTX_MEM_INFO_GET, wtxMemAlloc().
*/

WTX_MEM_INFO * wtxMemInfoGet 
    (
    HWTX		hWtx		/* WTX API handle */
    )
    {
    WTX_ERROR_T		callStat;	/* WTX status */
    WTX_MSG_MEM_INFO *	pOut;

    WTX_CHECK_CONNECTED (hWtx, NULL);
    
    pOut = calloc (1, sizeof (WTX_MSG_MEM_INFO));

    if (pOut == NULL)
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_MEMALLOC, NULL);

    callStat = exchange (hWtx, WTX_MEM_INFO_GET, &hWtx->msgToolId, pOut);

    if (callStat != WTX_ERR_NONE)
	{
	free (pOut);		/* free allocated message */
	WTX_ERROR_RETURN (hWtx, callStat, NULL);
	}

    /* add pointer to the wtxResultFree () list */

    if (wtxFreeAdd (hWtx, (void *) &pOut->curBytesFree, NULL, pOut,
		    WTX_MEM_INFO_GET, hWtx->server, WTX_SVR_SERVER) != WTX_OK)
	{
	WTX_ERROR_RETURN (hWtx, wtxErrGet (hWtx), NULL);
	}

    return ((WTX_MEM_INFO *) &pOut->curBytesFree);
    }


/*******************************************************************************
* 
* wtxMemAlloc - allocate a block of memory to the TS-managed target memory pool
*
* This routine allocates <numBytes> of memory on the target. The block
* location and alignment are unspecified.
* 
* RETURNS: The address of the target memory allocated or NULL on error.
*
* SEE ALSO: WTX_MEM_ALLOC, wtxMemFree()
*/

TGT_ADDR_T wtxMemAlloc 
    (
    HWTX	hWtx,		/* WTX API handle */
    UINT32	numBytes	/* size to allocate in bytes */
    )
    {
    WTX_ERROR_T		callStat;
    WTX_MSG_PARAM	in;
    WTX_MSG_RESULT	out;
    TGT_ADDR_T		result;

    WTX_CHECK_CONNECTED (hWtx, 0);

    memset (&in, 0, sizeof (in));
    memset (&out, 0, sizeof (out));

    in.param.valueType = V_UINT32;
    in.param.value_u.v_uint32 = numBytes;

    callStat = exchange (hWtx, WTX_MEM_ALLOC, &in, &out);

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, 0);

    result = out.val.value_u.v_tgt_addr;

    wtxExchangeFree (hWtx->server, WTX_MEM_ALLOC, &out);

    return result;
    }


/*******************************************************************************
* 
* wtxMemChecksum - perform a checksum on target memory
*
* This routine returns a checksum for <numBytes> of memory starting at
* address <startAddr>. Errors must be detected by using the wtxErrXXX()
* routines.
*
* RETURNS: A checksum value.
*
* SEE ALSO: WTX_MEM_CHECKSUM
*/

UINT32 wtxMemChecksum
    (
    HWTX	hWtx,		/* WTX API handle */
    TGT_ADDR_T	startAddr,	/* remote addr to start checksum at */
    UINT32	numBytes	/* number of bytes to checksum */
    )
    {
    WTX_ERROR_T			callStat;
    WTX_MSG_MEM_SET_DESC	in;
    WTX_MSG_RESULT		out;
    UINT32			result;

    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    memset (&in, 0, sizeof (in));
    memset (&out, 0, sizeof (out));

    in.startAddr = startAddr;
    in.numItems = numBytes;

    callStat = exchange (hWtx, WTX_MEM_CHECKSUM, &in, &out);

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    result = out.val.value_u.v_uint32;
    wtxExchangeFree (hWtx->server, WTX_MEM_CHECKSUM, &out);

    return result;
    }


/*******************************************************************************
* 
* wtxMemMove - move a block of target memory
*
* This routine moves <numBytes> from <srcAddr> to <destAddr>.  Note that
* the source and destination buffers may overlap.
*
* RETURNS: WTX_OK or WTX_ERROR.
*
* SEE ALSO: WTX_MEM_MOVE
*/

STATUS wtxMemMove
    (
    HWTX	hWtx,		/* WTX API handle */
    TGT_ADDR_T	srcAddr,	/* remote addr to move from */
    TGT_ADDR_T	destAddr,	/* remote addr to move to */
    UINT32	numBytes	/* number of bytes to move */
    )
    {
    WTX_ERROR_T			callStat;
    WTX_MSG_MEM_MOVE_DESC	in;
    WTX_MSG_RESULT		out;

    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    memset (&in, 0, sizeof (in));
    memset (&out, 0, sizeof (out));

    in.source = srcAddr;
    in.destination = destAddr;
    in.numBytes = numBytes;

    callStat = exchange (hWtx, WTX_MEM_MOVE, &in, &out);

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    wtxExchangeFree (hWtx->server, WTX_MEM_MOVE, &out);

    return (WTX_OK);
    }


/*******************************************************************************
* 
* wtxMemFree - free a block of target memory
*
* This routine frees a target memory block previously allocated with
* wtxMemAlloc().
* 
* RETURNS: WTX_OK or WTX_ERROR.
*
* SEE ALSO: WTX_MEM_FREE, wtxMemAlloc()
*/

STATUS wtxMemFree 
    (
    HWTX	hWtx,		/* WTX API handle */
    TGT_ADDR_T	address		/* target memory block address to free */
    )
    {
    WTX_ERROR_T	callStat;
    WTX_MSG_PARAM	in;
    WTX_MSG_RESULT	out;

    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    memset (&in, 0, sizeof (in));
    memset (&out, 0, sizeof (out));

    in.param.valueType = V_TGT_ADDR;
    in.param.value_u.v_tgt_addr = address;

    callStat = exchange (hWtx, WTX_MEM_FREE, &in, &out);

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    wtxExchangeFree (hWtx->server, WTX_MEM_FREE, &out);

    return (WTX_OK);
    }


/*******************************************************************************
* 
* wtxMemRead - read memory from the target
*
* This routine reads <numBytes> of memory from the target starting at 
* <fromAddr> and writes the contents to <toAddr>. 
*
* NOTE: Because the target agent tests that all memory is readable,
* this routine can never do a partial read. The return value is
* always <numBytes> or WTX_ERROR.
* 
* ERRORS:
* .iP WTX_ERR_API_INVALID_ARG 12
* <toAddr> or <numBytes> is invalid.
*
* RETURNS: The number of bytes read (see note) or WTX_ERROR.
* 
* SEE ALSO: WTX_MEM_READ, wtxMemWrite()
*/

UINT32 wtxMemRead 
    (
    HWTX	hWtx,		/* WTX API handle */
    TGT_ADDR_T	fromAddr,	/* Target addr to read from */
    void *	toAddr,		/* Local addr to read to */
    UINT32	numBytes	/* Bytes to read */
    )
    {
    WTX_ERROR_T			callStat;
    WTX_MSG_MEM_READ_DESC	in;
    WTX_MSG_MEM_COPY_DESC	out;

    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    if ((toAddr == 0) || (numBytes == 0))
        WTX_ERROR_RETURN (hWtx, WTX_ERR_API_INVALID_ARG, WTX_ERROR);

    memset (&in, 0, sizeof (in));
    memset (&out, 0, sizeof (out));

    in.source = fromAddr;
    in.destination = (UINT32) toAddr;
    in.numBytes = numBytes;

    callStat = exchange (hWtx, WTX_MEM_READ, &in, &out);

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    wtxExchangeFree (hWtx->server, WTX_MEM_READ, &out);

    /* NOTE : WTX_MEM_READ can only read 0 or all of the bytes, hence the
     * return value is actually 0 (a status code == WTX_OK) when all is
     * well.
     */

    return numBytes;
    }

/*******************************************************************************
*
* wtxMemWidthRead - read memory from the target
*
* This routine reads <numBytes> (on <width> bytes large) from the target 
* starting at <fromAddr> and writes the contents to <toAddr>.
*
* NOTE: Because the target agent tests that all memory is readable,
* this routine can never do a partial read. The return value is
* always <numBytes> or WTX_ERROR.
*
* NOTE: This request is not implemented on WDB side, wtxMemWidthRead()
* is mapped on wtxMemRead() and the <width> parameter is simply ignored.
* 
* ERRORS:
* .iP WTX_ERR_API_INVALID_ARG 12
* <toAddr>, <numBytes> or <width> is invalid
*
* RETURNS: The number of bytes read (see note) or WTX_ERROR.
*
* SEE ALSO: WTX_MEM_WIDTH_READ, wtxMemRead(), wtxMemWidthWrite()
*/

UINT32 wtxMemWidthRead
    (
    HWTX        hWtx,                      /* WTX API handle           */
    TGT_ADDR_T  fromAddr,                  /* Target addr to read from */
    void *      toAddr,                    /* Local addr to read to    */
    UINT32      numBytes,                  /* Bytes to read            */
    UINT8       width                      /* Value width in bytes     */
    )
    {
    WTX_ERROR_T                 callStat;
    WTX_MSG_MEM_WIDTH_READ_DESC in;
    WTX_MSG_MEM_COPY_DESC       out;

    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    if ((toAddr == 0) || (numBytes == 0))
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_INVALID_ARG, WTX_ERROR);

    if ((width != 1) && (width != 2) && (width != 4))
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_INVALID_ARG, WTX_ERROR);

    memset (&in, 0, sizeof (in));
    memset (&out, 0, sizeof (out));

    in.source = fromAddr;
    in.destination = (UINT32) toAddr;
    in.numBytes = numBytes;
    in.width = width;

    callStat = exchange (hWtx, WTX_MEM_WIDTH_READ, &in, &out);

    if (callStat != WTX_ERR_NONE)
        WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    wtxExchangeFree (hWtx->server, WTX_MEM_WIDTH_READ, &out);

    /*
     * NOTE : WTX_MEM_WIDTH_READ can only read 0 or all of the bytes, hence the
     * return value is actually 0 (a status code == WTX_OK) when all is
     * well.
     */

    return (numBytes);
    }

/*******************************************************************************
* 
* wtxMemWrite - write memory on the target
*
* This routine writes <numBytes> of memory to <toAddr> on the target from
* the local address <fromAddr>. 
*
* NOTE: Because the target agent tests that all memory is writable,
* this routine can never do a partial write. The return value is
* always <numBytes> or WTX_ERROR.
* 
* ERRORS:
* .iP WTX_ERR_API_INVALID_ARG 12
* <fromAddr> or <numBytes> is invalid.
*
* RETURNS: The number of bytes written (see note) or WTX_ERROR. 
*
* SEE ALSO: WTX_MEM_WRITE, wtxMemRead()
*/

UINT32 wtxMemWrite 
    (
    HWTX	hWtx,		/* WTX API handle */
    void *	fromAddr,	/* Local addr to write from */
    TGT_ADDR_T	toAddr,		/* Remote addr to write to */
    UINT32	numBytes	/* Bytes to read */
    )
    {
    WTX_ERROR_T			callStat;
    WTX_MSG_MEM_COPY_DESC	in;
    WTX_MSG_RESULT		out;

    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    if ((fromAddr == 0) || (numBytes == 0))
        WTX_ERROR_RETURN (hWtx, WTX_ERR_API_INVALID_ARG, WTX_ERROR);

    memset (&in, 0, sizeof (in));
    memset (&out, 0, sizeof (out));

    in.source = (UINT32) fromAddr;
    in.destination = toAddr;
    in.numBytes = numBytes;

    callStat = exchange (hWtx, WTX_MEM_WRITE, &in, &out);

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    wtxExchangeFree (hWtx->server, WTX_MEM_WRITE, &out);

    /* NOTE : WTX_MEM_WRITE can only write 0 or all of the bytes, hence the
     * return value is actually 0 (a status code == WTX_OK) when all is
     * well.
     */
    return numBytes;
    }

/*******************************************************************************
*
* wtxMemWidthWrite - write memory on the target
*
* This routine writes <numBytes> (on <width> bytes large) at <toAddr>
* on the target from the local address <fromAddr>.
*
* NOTE: Because the target agent tests that all memory is writable,
* this routine can never do a partial write. The return value is
* always <numBytes> or WTX_ERROR.
*
* NOTE: This request is not implemented on WDB side,
* wtxMemWidthWrite() is mapped on wtxMemWrite() and the <width> parameter is
* simply ignored.
*
* ERRORS:
* .iP WTX_ERR_API_INVALID_ARG 12
* <fromAddr>, <numBytes> or <width> is invalid
*
* RETURNS: The number of bytes written (see note) or WTX_ERROR.
*
* SEE ALSO: WTX_MEM_WIDTH_WRITE, wtxMemWrite(), wtxMemWidthRead()
*/

UINT32 wtxMemWidthWrite
    (
    HWTX        hWtx,                      /* WTX API handle             */
    void *      fromAddr,                  /* Local addr to write from   */
    TGT_ADDR_T  toAddr,                    /* Remote addr to write to    */
    UINT32      numBytes,                  /* Bytes to read              */
    UINT8       width                      /* Width value: 1, 2, 4 bytes */
    )
    {
    WTX_ERROR_T                 callStat;
    WTX_MSG_MEM_WIDTH_COPY_DESC in;
    WTX_MSG_RESULT              out;

    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    memset (&in, 0, sizeof (in));
    memset (&out, 0, sizeof (out));

    if ((fromAddr == 0) || (numBytes == 0))
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_INVALID_ARG, WTX_ERROR);

    if ((width != 1) && (width != 2) && (width != 4))
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_INVALID_ARG, WTX_ERROR);

    in.source = (UINT32) fromAddr;
    in.destination = toAddr;
    in.numBytes = numBytes;
    in.width = width;

    callStat = exchange (hWtx, WTX_MEM_WIDTH_WRITE, &in, &out);

    if (callStat != WTX_ERR_NONE)
        WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    wtxExchangeFree (hWtx->server, WTX_MEM_WIDTH_WRITE, &out);

    /*
     * NOTE : WTX_MEM_WIDTH_WRITE can only write 0 or all of the bytes, hence
     * the return value is actually 0 (a status code == WTX_OK) when all is
     * well.
     */

    return (numBytes);
    }

/*******************************************************************************
* 
* wtxMemSet - set target memory to a given value
*
* This routine sets <numBytes> at address <addr> to the value <val>.
*
* RETURNS: WTX_OK or WTX_ERROR.
*
* SEE ALSO: WTX_MEM_SET
*/

UINT32 wtxMemSet 
    (
    HWTX	hWtx,		/* WTX API handle */
    TGT_ADDR_T	addr,		/* remote addr to write to */
    UINT32	numBytes,	/* number of bytes to set */
    UINT32	val		/* value to set */
    )
    {
    WTX_ERROR_T			callStat;
    WTX_MSG_MEM_SET_DESC	in;
    WTX_MSG_RESULT		out;

    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    memset (&in, 0, sizeof (in));
    memset (&out, 0, sizeof (out));

    in.startAddr = addr;
    in.width 	= 1;
    in.numItems = numBytes;
    in.value 	= (UINT32) val;

    callStat = exchange (hWtx, WTX_MEM_SET, &in, &out);

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    wtxExchangeFree (hWtx->server, WTX_MEM_SET, &out);

    return (WTX_OK);
    }


/******************************************************************************
*
* wtxMemAddToPool - add memory to the agent pool
*
* This function adds the block of memory starting at <address> and spanning
* <size> bytes to the agent pool, enlarging it.  It is possible to enlarge
* the agent pool with memory obtained from the runtime memory manager
* or with any currently unclaimed memory.
*
* RETURNS: WTX_OK or WTX_ERROR.
*
* SEE ALSO: WTX_MEM_ADD_TO_POOL, wtxMemInfoGet(), wtxMemAlloc(), 
* wtxMemRealloc(), wtxMemFree()
*/

STATUS wtxMemAddToPool
    (
    HWTX		hWtx,		/* WTX API handle */
    TGT_ADDR_T		address,	/* base of memory block to add */
    UINT32		size		/* size of memory block to add */
    )
    {
    WTX_ERROR_T		    	callStat;
    WTX_MSG_MEM_BLOCK_DESC	in;
    WTX_MSG_RESULT		out;

    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    memset (&in, 0, sizeof (in));
    memset (&out, 0, sizeof (out));

    in.startAddr = address;
    in.numBytes  = size;

    callStat = exchange (hWtx, WTX_MEM_ADD_TO_POOL, &in, &out);

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    wtxExchangeFree (hWtx->server, WTX_MEM_ADD_TO_POOL, &out);

    return (WTX_OK);
    }

/******************************************************************************
*
* wtxMemRealloc - reallocate a block of target memory
*
* This function changes the size of the block starting at <address> to 
* <numBytes> bytes and returns the address of the block (which may have moved).
*
* RETURNS: The address of the target memory reallocated or NULL on error.
*
* SEE ALSO: WTX_MEM_REALLOC, wtxMemAlloc()
*/

TGT_ADDR_T wtxMemRealloc
    (
    HWTX		hWtx,		/* WTX API handle */
    TGT_ADDR_T		address,	/* memory block to reallocate */
    UINT32		numBytes	/* new size */
    )
    {
    WTX_ERROR_T			callStat;
    WTX_MSG_MEM_BLOCK_DESC	in;
    WTX_MSG_RESULT		out;
    TGT_ADDR_T			result;

    WTX_CHECK_CONNECTED (hWtx, 0);

    memset (&in, 0, sizeof (in));
    memset (&out, 0, sizeof (out));

    in.startAddr = address;
    in.numBytes  = numBytes;

    callStat = exchange (hWtx, WTX_MEM_REALLOC, &in, &out);

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, 0);

    result = out.val.value_u.v_tgt_addr;
    wtxExchangeFree (hWtx->server, WTX_MEM_REALLOC, &out);

    return result;
    }

/******************************************************************************
*
* wtxMemAlign - allocate aligned target memory
*
* This function allocates a block of memory of <numBytes> bytes aligned on 
* a <alignment>-byte boundary.  <alignment> must be a power of 2.
*
* RETURNS: The address of the target memory allocated or NULL on error.
*
* SEE ALSO: WTX_MEM_ALIGN, wtxMemAlloc()
*/

TGT_ADDR_T wtxMemAlign
    (
    HWTX		hWtx,		/* WTX API handle */
    TGT_ADDR_T		alignment,	/* alignment boundary */
    UINT32		numBytes	/* size of block to allocate */
    )
    {
    WTX_ERROR_T			callStat;
    WTX_MSG_MEM_BLOCK_DESC	in;
    WTX_MSG_RESULT		out;
    TGT_ADDR_T			result;

    WTX_CHECK_CONNECTED (hWtx, 0);

    memset (&in, 0, sizeof (in));
    memset (&out, 0, sizeof (out));

    in.startAddr = alignment;
    in.numBytes  = numBytes;

    callStat = exchange (hWtx, WTX_MEM_ALIGN, &in, &out);

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, 0);

    result = out.val.value_u.v_tgt_addr;
    wtxExchangeFree (hWtx->server, WTX_MEM_ALIGN, &out);

    return result;
    }

/******************************************************************************
*
* wtxMemScan - scan target memory for the presence or absence of a pattern
*
* This routine scans the target memory from <startAddr> to <endAddr>.
* When <match> is set to TRUE, the first address containing the pattern
* pointed to by <pattern> is returned.  When <match> is
* FALSE, the first address not matching <pattern> is returned.
* <pattern> is a pointer to a host array of byte values of length
* <numBytes>. If the pattern cannot be found then an error is returned.
*
* RETURNS: WTX_OK or WTX_ERROR.
*
* SEE ALSO: WTX_MEM_SCAN
*/
STATUS wtxMemScan 
    (
    HWTX		hWtx,		/* WTX API handle */
    BOOL32		match,		/* Match/Not-match pattern boolean */
    TGT_ADDR_T		startAddr,	/* Target address to start scan */
    TGT_ADDR_T		endAddr,	/* Target address to finish scan */
    UINT32		numBytes,	/* Number of bytes in pattern */
    void *		pattern,	/* Pointer to pattern to search for */
    TGT_ADDR_T *	pResult		/* Pointer to result address */
    )
    {
    WTX_ERROR_T			callStat;
    WTX_MSG_MEM_SCAN_DESC	in;
    WTX_MSG_RESULT		out;

    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    if (pResult == NULL)
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_INVALID_ARG, WTX_ERROR);

    memset (&in, 0, sizeof (in));
    memset (&out, 0, sizeof (out));

    in.startAddr = startAddr;
    in.endAddr = endAddr;
    in.numBytes = numBytes;
    in.pattern = pattern;
    in.match = match ; 

    callStat = exchange (hWtx, WTX_MEM_SCAN, &in, &out);

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    *pResult = out.val.value_u.v_tgt_addr;
    wtxExchangeFree (hWtx->server, WTX_MEM_SCAN, &out);

    return (WTX_OK);
    }

/******************************************************************************
*
* wtxObjModuleChecksum - checks validity of target memory.
*
* This routine compares object module checksum on target with object module
* checksum in target server memory cache. This is a way to control the 
* target memory integrity. If <moduleId> is set to WTX_ALL_MODULE_ID or 
* <fileName> set to "WTX_ALL_MODULE_CHECK" then all modules will be checked. 
*
* NOTE:
* Because elf modules may have more than 1 section of text, text sections
* are gathered in a text segment. But, if on the target memory (or in the
* module) sections are contigous, they may not be contigous in the host 
* memory cache (due to different malloc for each sections). Then checksums
* cannot be compared for the text segment but only for the first text section.
*
* ERRORS:
* .iP WTX_ERR_API_MEMALLOC 12
* <fileName> buffer cannot be allocated
* .iP WTX_ERR_API_INVALID_ARG
* <moduleId> or <fileName> invalid
*
* RETURN: WTX_OK or WTX_ERROR if exchange failed
*
* SEE ALSO: WTX_OBJ_MODULE_CHECKSUM, wtxObjModuleList(), wtxMemChecksum()
*/

STATUS wtxObjModuleChecksum
    (
    HWTX            hWtx,                  /* WTX API handle */
    INT32           moduleId,              /* Module Id      */
    char *          fileName               /* Module name    */
    )
    {
    WTX_ERROR_T                 callStat;
    WTX_MSG_MOD_NAME_OR_ID      in;
    WTX_MSG_RESULT              out;

    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    memset (&out, 0, sizeof (out));
    memset (&in, 0, sizeof (in));

    if ((moduleId == 0) || (moduleId == WTX_ERROR) || (fileName == NULL))
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_INVALID_ARG, WTX_ERROR);
 
    in.moduleId = moduleId;
    in.filename = fileName;

    if (moduleId == WTX_ALL_MODULE_ID)
	{
	if ((in.filename = (char *) malloc (strlen (WTX_ALL_MODULE_CHECK) + 1))
	    == NULL)
	    WTX_ERROR_RETURN (hWtx, WTX_ERR_API_MEMALLOC, WTX_ERROR);

        strcpy (in.filename, WTX_ALL_MODULE_CHECK);
	}

    callStat = exchange (hWtx, WTX_OBJ_MODULE_CHECKSUM, &in, &out);

    if (callStat != WTX_ERR_NONE)
        WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    wtxExchangeFree (hWtx->server, WTX_OBJ_MODULE_CHECKSUM, &out);

    return (WTX_OK);
    }

/*******************************************************************************
*
* wtxObjModuleFindId - find the ID of an object module given its name
*
* This routine returns the ID of the object module that was loaded
* from the file with the name <moduleName>.  The object name must not
* include any directory components as this is not stored in the
* object module.
*
* RETURNS: The object module ID or WTX_ERROR.
*
* SEE ALSO: WTX_OBJ_MODULE_FIND, wtxObjModuleFindName(), wtxObjModuleInfoGet()
*/

UINT32	wtxObjModuleFindId 
    (
    HWTX		hWtx,		/* WTX API handle */
    const char *	moduleName	/* object module file name */
    )
    {
    WTX_MSG_MOD_NAME_OR_ID in;
    WTX_MSG_MOD_NAME_OR_ID out;
    WTX_ERROR_T		   callStat;
    UINT32		   moduleId;

    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    memset (&in, 0, sizeof (in));
    memset (&out, 0, sizeof (out));

    in.filename = (char *) moduleName;

    callStat = exchange (hWtx, WTX_OBJ_MODULE_FIND, &in, &out);

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    moduleId = out.moduleId;

    wtxExchangeFree (hWtx->server, WTX_OBJ_MODULE_FIND, &out);

    return out.moduleId;
    }

/*******************************************************************************
*
* wtxObjModuleFindName - find object module name given its ID
*
* This routine returns the name of the object for the module that has
* the ID <moduleId>.  The name returned is the name of the object file
* from which it was loaded and does not include the directory where
* the object file was loaded. The returned string must be freed by
* the user calling wtxResultFree().
*
* ERRORS:
* .iP WTX_ERR_API_INVALID_ARG 12
* <moduleId> is invalid
*
* RETURNS: The object module filename or NULL.
*
* SEE ALSO: WTX_OBJ_MODULE_FIND, wtxObjModuleFindId(), wtxObjModuleInfoGet() 
*/

char * wtxObjModuleFindName 
    (
    HWTX	hWtx,		/* WTX API handle                      */
    UINT32	moduleId	/* id of module to find object name of */
    )
    {
    WTX_MSG_MOD_NAME_OR_ID *	pOut = NULL;
    WTX_MSG_MOD_NAME_OR_ID	in;
    WTX_ERROR_T			callStat;

    WTX_CHECK_CONNECTED (hWtx, NULL);

    memset (&in, 0, sizeof (in));

    if ((moduleId == 0) || (moduleId == (UINT32) WTX_ERROR))
        WTX_ERROR_RETURN (hWtx, WTX_ERR_API_INVALID_ARG, NULL);

    /* arguments have been checked, allocate room for the out structure */

    pOut = (WTX_MSG_MOD_NAME_OR_ID *) calloc (1,
					      sizeof (WTX_MSG_MOD_NAME_OR_ID));
    if (pOut == NULL)
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_MEMALLOC, NULL);


    in.moduleId = moduleId;

    callStat = exchange (hWtx, WTX_OBJ_MODULE_FIND, &in, pOut);

    if (callStat != WTX_ERR_NONE)
	{
	free (pOut);
	WTX_ERROR_RETURN (hWtx, callStat, NULL);
	}

    /* add pointer to the wtxResultFree () list */

    if (wtxFreeAdd (hWtx, (void *) pOut->filename, NULL, pOut,
		    WTX_OBJ_MODULE_FIND, hWtx->server, WTX_SVR_SERVER)
	!= WTX_OK)
	{
	WTX_ERROR_RETURN (hWtx, wtxErrGet (hWtx), NULL);
	}

    return (pOut->filename);
    }

/*******************************************************************************
*
* wtxObjModuleInfoGet - return information on a module given its module ID
*
* This routine returns a pointer to a module information structure for
* the module with the supplied ID <modId>. The module information must be
* freed by the user calling wtxResultFree().
*
* EXPAND ../../../include/wtx.h WTX_MODULE_INFO
*
* where OBJ_SEGMENT is:
*
* EXPAND ../../../include/wtxmsg.h OBJ_SEGMENT
*
* ERRORS:
* .iP WTX_ERR_API_INVALID_ARG 12
* <modId> is invalid
*
* RETURNS: A pointer to the module information structure or NULL on error.
*
* SEE ALSO: WTX_OBJ_MODULE_INFO_GET, wtxObjModuleList() 
*/

WTX_MODULE_INFO * wtxObjModuleInfoGet 
    (
    HWTX		  hWtx,		/* WTX API handle */
    UINT32		  modId		/* id of module to look for */
    )
    {
    WTX_ERROR_T			callStat;
    WTX_MSG_MOD_NAME_OR_ID	in;
    WTX_MSG_MODULE_INFO *	pOut;

    WTX_CHECK_CONNECTED (hWtx, NULL);

    if ((modId == 0) || (modId == (UINT32) WTX_ERROR))
        WTX_ERROR_RETURN (hWtx, WTX_ERR_API_INVALID_ARG, NULL);

    memset (&in, 0, sizeof (in));
    in.moduleId = modId;
    in.filename = NULL;

    pOut = calloc (1, sizeof (WTX_MSG_MODULE_INFO));
    if (pOut == NULL)
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_MEMALLOC, NULL);

    callStat = exchange (hWtx, WTX_OBJ_MODULE_INFO_GET, &in, pOut);

    if (callStat != WTX_ERR_NONE)
	{
	free (pOut);		/* Free allocated message */
	WTX_ERROR_RETURN (hWtx, callStat, NULL);
	}

    /* add pointer to the wtxResultFree () list */

    if (wtxFreeAdd (hWtx, (void *) &pOut->moduleId, NULL, pOut,
		    WTX_OBJ_MODULE_INFO_GET, hWtx->server, WTX_SVR_SERVER)
	!= WTX_OK)
	{
	WTX_ERROR_RETURN (hWtx, wtxErrGet (hWtx), NULL);
	}

    return ((WTX_MODULE_INFO *) &pOut->moduleId);
    }

/*******************************************************************************
*
* wtxObjModuleInfoAndPathGet - return information on a module given its ID
*
* This routine returns a pointer to a module information structure for
* the module with the supplied ID <modId>. The module information must be
* freed by the user calling wtxResultFree().
*
* This request returned the complete pathname in the <name> field of the
* result.
*
* EXPAND ../../../include/wtx.h WTX_MODULE_INFO
*
* where OBJ_SEGMENT is:
*
* EXPAND ../../../include/wtxmsg.h OBJ_SEGMENT
*
* ERRORS:
* .iP WTX_ERR_API_INVALID_ARG 12
* <modId> is invalid
*
* RETURNS: A pointer to the module information structure or NULL on error.
*
* SEE ALSO: WTX_OBJ_MODULE_INFO_GET, wtxObjModuleInfoGet(), wtxObjModuleList() 
*/

WTX_MODULE_INFO * wtxObjModuleInfoAndPathGet 
    (
    HWTX		  hWtx,		/* WTX API handle */
    UINT32		  modId		/* id of module to look for */
    )
    {
    WTX_ERROR_T			callStat;
    WTX_MSG_MOD_NAME_OR_ID	in;
    WTX_MSG_MODULE_INFO *	pOut;

    WTX_CHECK_CONNECTED (hWtx, NULL);

    if ((modId == 0) || (modId == (UINT32) WTX_ERROR))
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_INVALID_ARG, NULL);

    memset (&in, 0, sizeof (in));
    in.moduleId = modId;
    in.filename = WTX_OBJ_MODULE_PATHNAME_GET;

    pOut = calloc (1, sizeof (WTX_MSG_MODULE_INFO));
    if (pOut == NULL)
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_MEMALLOC, NULL);

    callStat = exchange (hWtx, WTX_OBJ_MODULE_INFO_GET, &in, pOut);

    if (callStat != WTX_ERR_NONE)
	{
	free (pOut);		/* Free allocated message */
	WTX_ERROR_RETURN (hWtx, callStat, NULL);
	}

    /* add pointer to the wtxResultFree () list */

    if (wtxFreeAdd (hWtx, (void *) &pOut->moduleId, NULL, pOut,
		    WTX_OBJ_MODULE_INFO_GET, hWtx->server, WTX_SVR_SERVER)
	!= WTX_OK)
	{
	WTX_ERROR_RETURN (hWtx, wtxErrGet (hWtx), NULL);
	}

    return ((WTX_MODULE_INFO *) &pOut->moduleId);
    }

/*******************************************************************************
*
* wtxObjModuleList - fetch a list of loaded object modules from the target
*
* This routine queries the target server symbol table for a list of
* loaded modules. A pointer to the module list is returned and must
* be freed by the caller using wtxResultFree().
*
* EXPAND ../../../include/wtx.h WTX_MODULE_LIST
*
* RETURNS: A pointer to the module list or NULL on error.
*
* SEE ALSO: WTX_OBJ_MODULE_LIST, wtxObjModuleInfoGet()
*/

WTX_MODULE_LIST * wtxObjModuleList 
    (
    HWTX		  hWtx		/* WTX API handle */
    )
    {
    WTX_ERROR_T			callStat;
    WTX_MSG_MODULE_LIST *	pOut;

    WTX_CHECK_CONNECTED (hWtx, NULL);

    pOut = calloc (1, sizeof (WTX_MSG_MODULE_LIST));
    if (pOut == NULL)
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_MEMALLOC, NULL);

    callStat = exchange (hWtx, WTX_OBJ_MODULE_LIST, &hWtx->msgToolId, pOut);

    if (callStat != WTX_ERR_NONE)
	{
	free (pOut);		/* Free allocated message */
	WTX_ERROR_RETURN (hWtx, callStat, NULL);
	}

    /* add pointer to the wtxResultFree () list */

    if (wtxFreeAdd (hWtx, (void *) &pOut->numObjMod, NULL, pOut,
		    WTX_OBJ_MODULE_LIST, hWtx->server, WTX_SVR_SERVER)
	!= WTX_OK)
	{
	WTX_ERROR_RETURN (hWtx, wtxErrGet (hWtx), NULL);
	}

    return ((WTX_MODULE_LIST *) &pOut->numObjMod);
    }

/*******************************************************************************
*
* wtxObjModuleLoad - Load a multiple section object file
*
* This routine loads a module onto the target and into the target server 
* module table. The caller should pass in a pointer to a WTX_LD_M_FILE_DESC 
* structure:
*
* EXPAND ../../../include/wtxmsg.h WTX_LD_M_FILE_DESC
*
* The user must set the `filename' field to the name of the file containing
* the object module and the `loadFlag' to the loader flags required:
* .iP LOAD_NO_SYMBOLS 12
* No symbols are added to the system symbol table
* .iP LOAD_LOCAL_SYMBOLS
* Only local symbols are added to the system symbol table
* .iP LOAD_GLOBAL_SYMBOLS
* Only external symbols are added to the system symbol table
* .iP LOAD_ALL_SYMBOLS
* All symbols are added to the system symbol table
* .iP LOAD_FULLY_LINKED
* No relocation is required
* .iP LOAD_NO_DOWNLOAD
* The module is not loaded on the target
* .iP LOAD_HIDDEN_MODULE
* Load the module but make it invisible to WTX_MSG_MODULE_LIST
* .iP LOAD_COMMON_MATCH_NONE
* Allocate common symbols, but do not search for any matching symbols (the
* default)
* .iP LOAD_COMMON_MATCH_USER
* Allocate common symbols, but search for matching symbols in user-loaded
* modules
* .iP LOAD_COMMON_MATCH_ALL
* Allocate common symbols, but search for matching symbols in user-loaded
* modules and the target-system core file
* .iP LOAD_BAL_OPTIM
* Use branch-and-link optimization (for i960 targets only)
* .iP LOAD_CPLUS_XTOR_AUTO
* C++ ctors/dtors management is explicitly turned on
* .iP LOAD_CPLUS_XTOR_MANUAL
* C++ ctors/dtors management is explicitly turned off
* .iP LOAD_MODULE_INFO_ONLY
* Create and initialize a module, do not load a file
* .LP
*
* When no section description is given and `nSections' is set to zero, the 
* host-based loader allocates memory for the text, data and bss sections of 
* the module using memory from the target memory pool. If the `nSections' 
* field is non-zero, the user can specify the location to load the sections 
* and the `section' field points to an array of section descriptors:
*
* EXPAND ../../../include/wtxmsg.h LD_M_SECTION
*
* If the load is successful, the return value is a pointer to a file descriptor
* for the new object module containing the actual information about the file 
* loaded.  If there are undefined symbols in the module, `undefSymList' points
* to a list of those symbols that were not resolved:
*
* EXPAND ../../../include/wtxmsg.h WTX_SYM_LIST
*
* EXPAND ../../../include/wtxmsg.h WTX_SYMBOL
*
* Use free() to free the input WTX_LD_M_FILE_DESC parameter and wtxResultFree()
* to free the output WTX_LD_M_FILE_DESC parameter.
*
* NOTE: COFF object modules with more than 3 sections are not supported.
*
* Files loaded on the target by the target server can be opened either by
* the target server or by the client. These two behaviors can be controled by
* <moduleId>: "WTX_LOAD_FROM_CLIENT" opens the file where the client is, 
* "WTX_LOAD_FROM_TARGET_SERVER" opens the file where the target server is.
*
* EXAMPLE:
* Load a module on the target, the file is opened either by the client or by
* the target server according to the place where the file is.
*
* .CS
*   ...
*
*   pFileDescIn->filename  = "/folk/pascal/Board/ads860/objSampleWtxtclTest2.o";
*   pFileDescIn->loadFlag  = LOAD_GLOBAL_SYMBOLS;
*   pFileDescIn->nSections = 0;
*   pFileDescIn->moduleId  = WTX_LOAD_FROM_CLIENT;
*
*   /@ Open the file locally @/
*
*   if (isTgtSvrLocal)
*       if((pFileDescOut = wtxObjModuleLoad (wtxh, pFileDescIn)) == NULL)
*	    return (WTX_ERROR);
*	else
*	     {
*            /@ Do something @/
*
*	     ... 
*            }
*        }
*    else	/@ The target server opens the file @/
*        {
*        pFileDescIn->moduleId = WTX_LOAD_FROM_TARGET_SERVER;
*
*        if((pFileDescOut = wtxObjModuleLoad (wtxh, pFileDescIn)) == NULL)
*            return (WTX_ERROR);
*        else
*            {
*            /@ Do something @/
*
*	     ...
*            }
*        }
*   ...
* .CE
*
* RETURNS: A pointer to a WTX_LD_M_FILE_DESC for the new module or NULL on
* error.
*
* ERRORS:
* .iP WTX_ERR_API_MEMALLOC 12
* A buffer cannot be allocated
* .iP WTX_ERR_API_INVALID_ARG
* filename is NULL
* .iP WTX_ERR_API_FILE_NOT_ACCESSIBLE
* filename doesn't exist or bad permission
* .iP WTX_ERR_API_FILE_NOT_FOUND
* Cant bind the path name to the file descriptor
* .iP WTX_ERR_API_NULL_SIZE_FILE
* File have a null size
* .iP WTX_ERR_API_CANT_READ_FROM_FILE
* Read in file failed
* .iP WTX_ERR_SVR_EINVAL
* Request issued out of sequence. A tool should not submit a load request if
* it already has one pending.
* .iP WTX_ERR_SVR_NOT_ENOUGH_MEMORY
* Cannot allocate memory for internal needs
* .iP WTX_ERR_TGTMEM_NOT_ENOUGH_MEMORY
* Not enough memory in the target-server-managed target memory pool to store the
* object module segments.
* .iP WTX_ERR_LOADER_XXX
* See WTX_OBJ_MODULE_LOAD
*
* SEE ALSO: WTX_OBJ_MODULE_LOAD_2, wtxObjModuleLoad(), wtxObjModuleUnload(),
* wtxObjModuleList(), wtxObjModuleInfoGet(), wtxObjModuleLoadStart(),
* wtxObjModuleLoadCancel(), wtxObjModuleLoadProgressReport(),
* .I API Reference Manual: Target Server Internal Routines,
* .I API Programmer's Guide: Object Module Loader
*/

WTX_LD_M_FILE_DESC * wtxObjModuleLoad
    (
    HWTX                        hWtx,           /* WTX API handle    */
    WTX_LD_M_FILE_DESC *        pFileDesc       /* Module descriptor */
    )
    {
    WTX_MSG_FILE_LOAD_DESC      in;             /* Input value           */
    WTX_MSG_LD_M_FILE_DESC *    pOut;           /* Return value          */
    WTX_ERROR_T                 callStat;       /* Exchange return value */
    UINT32			scnIdx;		/* Loop counter		 */
    int				fileNameLen=0;	/* file name length      */
#ifdef HOST
    int                         fd;             /* File descriptor       */
    int                         fileSize;       /* Size of object module */
    int                         nBytes;         /* Number of bytes read  */
    char *                      buf;            /* Slice of file to load */
#ifndef WIN32
    struct stat                 infoFile;       /* Some info on the file */
#else
    struct _stat                infoFile;       /* Some info on the file */
#endif /* WIN 32 */
#endif /* HOST */

    WTX_CHECK_CONNECTED (hWtx, NULL);

    /* Initialize the input parameter */

    memset (&in, 0, sizeof (in));

    /* Output parameter location */

    pOut = (WTX_MSG_LD_M_FILE_DESC *) calloc (1, 
					      sizeof (WTX_MSG_LD_M_FILE_DESC));
    if (pOut == NULL)
        {
        WTX_ERROR_RETURN (hWtx, WTX_ERR_API_MEMALLOC, NULL);
        }

    pOut->wtxCore.protVersion = (UINT32) pOut;

    /* 
     * This ack is used to prevent garbage bits in moduleId :
     *	- user can only set WTX_LOAD_FROM_XXX
     *	- we can use WTX_LOAD_ASYNCHRONOUSLY & WTX_LOAD_PROGRESS_REPORT
     */

    if (!internalLoadCall)
	pFileDesc->moduleId &= WTX_LOAD_FROM_TARGET_SERVER;
    else
	internalLoadCall = FALSE;

    /* Set the asynchronous / progress report load flag if needed */

    if (pFileDesc->moduleId == WTX_LOAD_PROGRESS_REPORT)
	{
	wtxErrClear (hWtx);

	in.flags = WTX_LOAD_PROGRESS_REPORT;

        callStat = exchange (hWtx, WTX_OBJ_MODULE_LOAD_2, &in, pOut);

        if (callStat != WTX_ERR_NONE) 
            {
	    free (pOut);
            WTX_ERROR_RETURN (hWtx, callStat, NULL);
            }

	if (pOut->loadFlag == WTX_ERR_LOADER_LOAD_IN_PROGRESS)
	    {
	    wtxErrSet (hWtx, WTX_ERR_LOADER_LOAD_IN_PROGRESS);
	    }

	if (wtxFreeAdd (hWtx, (void *) ((int) pOut +
					OFFSET (WTX_MSG_DUMMY, field)),
			NULL, pOut, WTX_OBJ_MODULE_LOAD_2, hWtx->server,
			WTX_SVR_SERVER) != WTX_OK)
	    {
	    WTX_ERROR_RETURN (hWtx, wtxErrGet (hWtx), NULL);
	    }

        /* Output parameter */

        return ((WTX_LD_M_FILE_DESC *) ((int) pOut + 
					OFFSET (WTX_MSG_DUMMY, field)));
	}

    if (pFileDesc->moduleId & WTX_LOAD_ASYNCHRONOUSLY) 
	{
	in.flags = WTX_LOAD_ASYNCHRONOUSLY;
	}
    else
	{
	in.flags = 0;
	}

    /* Set the input structure */

    if (pFileDesc->filename == NULL)
        {
        free (pOut);
        WTX_ERROR_RETURN (hWtx, WTX_ERR_API_INVALID_ARG, NULL);
        }

    /* allocate room for the file name string */

    fileNameLen = strlen (pFileDesc->filename) + 1;
    if ( (in.fileDesc.filename = (char *) malloc (fileNameLen)) == NULL)
	{
	free (pOut);
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_MEMALLOC, NULL);
	}

    strcpy (in.fileDesc.filename, pFileDesc->filename);

    if (pFileDesc->nSections != 0)
        {
        in.fileDesc.section = (LD_M_SECTION *) calloc (1, 
			pFileDesc->nSections * sizeof (LD_M_SECTION));
        if (in.fileDesc.section == NULL)
            {
            free (pOut);
            free (in.fileDesc.filename);
            WTX_ERROR_RETURN (hWtx, WTX_ERR_API_MEMALLOC, NULL);
            }

	for (scnIdx = 0; scnIdx < pFileDesc->nSections; scnIdx++)
	    in.fileDesc.section [scnIdx].addr =
				pFileDesc->section [scnIdx].addr;
        }
    else
        in.fileDesc.section = NULL;

    memset ((void *) &in.fileDesc.undefSymList, 0, sizeof (WTX_SYM_LIST));

    in.wtxCore.errCode = 0;
    in.wtxCore.protVersion = 0;

    in.fileDesc.moduleId = 0;
    in.fileDesc.loadFlag = pFileDesc->loadFlag;
    in.fileDesc.nSections = pFileDesc->nSections;
    in.numPacket = 0;

    /* The file must be opened by the target server */

    if ((pFileDesc->moduleId & WTX_LOAD_FROM_TARGET_SERVER) ||
	((pFileDesc->loadFlag & LOAD_MODULE_INFO_ONLY) 
		== LOAD_MODULE_INFO_ONLY))
        {
        in.buffer = NULL;
        in.numItems = 0;
        in.fileSize = 0;
        in.numPacket = 0;

        callStat = exchange (hWtx, WTX_OBJ_MODULE_LOAD_2, &in, pOut);

        if (callStat != WTX_ERR_NONE)
            {
            free (pOut);
            free (in.fileDesc.filename);
            if (in.fileDesc.section != NULL)
                free (in.fileDesc.section);
            WTX_ERROR_RETURN (hWtx, callStat, NULL);
            }

        free (in.fileDesc.filename);
        if (in.fileDesc.section != NULL)
            free (in.fileDesc.section);

        /* Output parameter */

	if (wtxFreeAdd (hWtx, (void *) ((int) pOut + OFFSET (WTX_MSG_DUMMY,
							     field)),
			NULL, pOut, WTX_OBJ_MODULE_LOAD_2, hWtx->server,
			WTX_SVR_SERVER) != WTX_OK)
	    {
	    WTX_ERROR_RETURN (hWtx, wtxErrGet (hWtx), NULL);
	    }

        return ((WTX_LD_M_FILE_DESC *) ((int) pOut + 
					OFFSET (WTX_MSG_DUMMY, field)));
        }

    /* Open object module file */
#ifdef HOST
#ifdef WIN32
    if((fd = open (pFileDesc->filename, O_RDONLY | _O_BINARY, 0)) == ERROR)
#else /* WIN32 */
    if((fd = open (pFileDesc->filename, O_RDONLY , 0)) == ERROR)
#endif /* WIN32 */
        {
        free (pOut);
        free (in.fileDesc.filename);
        if (in.fileDesc.section != NULL)
            free (in.fileDesc.section);
        WTX_ERROR_RETURN (hWtx, WTX_ERR_API_FILE_NOT_ACCESSIBLE, NULL);
        }

    /* Bind the path to the file descriptor */

    if (pathFdBind (fd, pFileDesc->filename) == ERROR)
        {
        close (fd);
        free (pOut);
        free (in.fileDesc.filename);
        if (in.fileDesc.section != NULL)
            free (in.fileDesc.section);
        WTX_ERROR_RETURN (hWtx, WTX_ERR_API_FILE_NOT_FOUND, NULL);
        }

    /* Get the object file size */

#ifndef WIN32
    if (fstat (fd, &infoFile) != OK)
#else
    if (_fstat (fd, &infoFile) != OK)
#endif /* WIN32 */
        {
        close (fd);
        free (pOut);
        free (in.fileDesc.filename);
        if (in.fileDesc.section != NULL)
            free (in.fileDesc.section);
        WTX_ERROR_RETURN (hWtx, WTX_ERR_API_FILE_NOT_ACCESSIBLE, NULL);
        }

    fileSize = (int) infoFile.st_size;
    if (fileSize == 0)
        {
        close (fd);
        free (pOut);
        free (in.fileDesc.filename);
        if (in.fileDesc.section != NULL)
            free (in.fileDesc.section);
        WTX_ERROR_RETURN (hWtx, WTX_ERR_API_NULL_SIZE_FILE, NULL);
        }

    /* Set the input structure */

    in.fileSize = fileSize;

    do
        {
        /* Malloc on buf to store a part of the file */

        buf = (char *) calloc (WTX_FILE_SLICE_SIZE, sizeof(char));
        if (buf == NULL)
            {
            free (pOut);
            free (in.fileDesc.filename);
            if (in.fileDesc.section != NULL)
                free (in.fileDesc.section);
            WTX_ERROR_RETURN (hWtx, WTX_ERR_API_MEMALLOC, NULL);
            }

        /* The object file is read and stored in the host memory */

        nBytes = read (fd, buf, WTX_FILE_SLICE_SIZE);
        if (nBytes == ERROR)
            {
            free (pOut);
            free (in.fileDesc.filename);
            if (in.fileDesc.section != NULL)
                free (in.fileDesc.section);
            free (buf);
            WTX_ERROR_RETURN (hWtx, WTX_ERR_API_CANT_READ_FROM_FILE, NULL);
            }

        /* EOF reached: exit while loop */

        if (nBytes == 0)
            {
            free (buf);
            break;
            }

        in.numPacket++;
        in.numItems = nBytes;
        in.buffer = buf;

        callStat = exchange (hWtx, WTX_OBJ_MODULE_LOAD_2, &in, pOut);

        if (callStat != WTX_ERR_NONE)
            {
            free (buf);
            free (pOut);
            free (in.fileDesc.filename);
            if (in.fileDesc.section != NULL)
                free (in.fileDesc.section);
            WTX_ERROR_RETURN (hWtx, callStat, NULL);
            }

        free (buf);

        /* One more time if more events to display */

        } while (nBytes != 0);

    /* Close the file */

    close (fd);
    free (in.fileDesc.filename);
    if (in.fileDesc.section != NULL)
        free (in.fileDesc.section);

    /* add pointer to the wtxResultFree () list */

    if (wtxFreeAdd (hWtx, (void *) ((int) pOut + OFFSET (WTX_MSG_DUMMY, field)),
		    NULL, pOut, WTX_OBJ_MODULE_LOAD_2, hWtx->server,
		    WTX_SVR_SERVER) != WTX_OK)
	{
	WTX_ERROR_RETURN (hWtx, wtxErrGet (hWtx), NULL);
	}

#endif /* HOST */
    /* Output parameter */

    return ((WTX_LD_M_FILE_DESC *) ((int) pOut + 
				    OFFSET (WTX_MSG_DUMMY, field)));
    }

/*******************************************************************************
*
* wtxObjModuleLoadStart - Load a multiple section object file asynchronously
*
* This routine loads a module onto the target and into the target server 
* module table. It returns when the module is in the target server memory,
* ready to be relocated and downloaded by the target server loader. 
* The caller should pass in a pointer to a WTX_LD_M_FILE_DESC structure:
*
* EXPAND ../../../include/wtxmsg.h WTX_LD_M_FILE_DESC
*
* The user must set the `filename' field to the name of the file containing
* the object module and the `loadFlag' to the loader flags required:
* .iP LOAD_NO_SYMBOLS 12
* No symbols are added to the system symbol table
* .iP LOAD_LOCAL_SYMBOLS
* Only local symbols are added to the system symbol table
* .iP LOAD_GLOBAL_SYMBOLS
* Only external symbols are added to the system symbol table
* .iP LOAD_ALL_SYMBOLS
* All symbols are added to the system symbol table
* .iP LOAD_FULLY_LINKED
* No relocation is required
* .iP LOAD_NO_DOWNLOAD
* The module is not loaded on the target
* .iP LOAD_HIDDEN_MODULE
* Load the module but make it invisible to WTX_MSG_MODULE_LIST
* .iP LOAD_COMMON_MATCH_NONE
* Allocate common symbols, but do not search for any matching symbols (the
* default)
* .iP LOAD_COMMON_MATCH_USER
* Allocate common symbols, but search for matching symbols in user-loaded
* modules
* .iP LOAD_COMMON_MATCH_ALL
* Allocate common symbols, but search for matching symbols in user-loaded
* modules and the target-system core file
* .iP LOAD_BAL_OPTIM
* Use branch-and-link optimization (for i960 targets only)
* .iP LOAD_CPLUS_XTOR_AUTO
* C++ ctors/dtors management is explicitly turned on
* .iP LOAD_CPLUS_XTOR_MANUAL
* C++ ctors/dtors management is explicitly turned off
* .iP LOAD_MODULE_INFO_ONLY
* Create and initialize a module, do not load a file
* .LP
*
* When no section description is given and `nSections' is set to zero, the 
* host-based loader allocates memory for the text, data and bss sections of 
* the module using memory from the target memory pool. If the `nSections' 
* field is non-zero, the user can specify the location to load the sections 
* and the `section' field points to an array of section descriptors:
*
* EXPAND ../../../include/wtxmsg.h LD_M_SECTION
*
* NOTE: COFF object modules with more than 3 sections are not supported.
*
* NOTE: Because this routine returns only WTX_OK or WTX_ERROR, the user
* must call wtxObjModuleLoadProgressReport() in order to have the module Id,
* sections addresses and the undefined symbols.
*
* Files loaded on the target by the target server can be opened either by
* the target server or by the client. These two behaviors can be controled by
* <moduleId>: "WTX_LOAD_FROM_CLIENT" opens the file where the client is, 
* "WTX_LOAD_FROM_TARGET_SERVER" opens the file where the target server is.
*
* EXAMPLE:
* Load a module on the target, the file is opened either by the client or by
* the target server according to the place where the file is.
*
* .CS
*   ...
*
*   pFileDescIn->filename  = "/folk/pascal/Board/ads860/objSampleWtxtclTest2.o";
*   pFileDescIn->loadFlag  = LOAD_GLOBAL_SYMBOLS;
*   pFileDescIn->nSections = 0;
*   pFileDescIn->moduleId  = WTX_LOAD_FROM_CLIENT;
*
*   /@ Open the file locally @/
*
*   if (isTgtSvrLocal)
*	{
*       if(wtxObjModuleLoadStart (wtxh, pFileDescIn) == WTX_ERROR)
*	    return (WTX_ERROR);
*	else
*	    {
*	    /@ Load is in progress: get its status @/
*
*	    if ((pLoadReport = wtxObjModuleLoadProgressReport (wtxh)) == NULL)
*	        return (WTX_ERROR);
*
*           /@ Do something @/
*
*	    ... 
*           }
*       }
*    else	/@ The target server opens the file @/
*       {
*       pFileDescIn->moduleId = WTX_LOAD_FROM_TARGET_SERVER;
*
*       if(wtxObjModuleLoadStart (wtxh, pFileDescIn) == WTX_ERROR)
*           return (WTX_ERROR);
*       else
*           {
*	    /@ Load is in progress: get its status @/
*
*	    if ((pLoadReport = wtxObjModuleLoadProgressReport (wtxh)) == NULL)
*	        return (WTX_ERROR);
*
*           /@ Do something @/
*
*	    ...
*           }
*       }
*   ...
* .CE
*
* RETURNS: WTX_OK or WTX_ERROR if something failed
*
* ERRORS:
* .iP WTX_ERR_API_MEMALLOC 12
* A buffer cannot be allocated
* .iP WTX_ERR_API_INVALID_ARG
* filename is NULL
* .iP WTX_ERR_API_FILE_NOT_ACCESSIBLE
* filename doesn't exist or bad permission
* .iP WTX_ERR_API_FILE_NOT_FOUND
* Cant bind the path name to the file descriptor
* .iP WTX_ERR_API_NULL_SIZE_FILE
* File have a null size
* .iP WTX_ERR_API_CANT_READ_FROM_FILE
* Read in file failed
* .iP WTX_ERR_SVR_EINVAL
* Request issued out of sequence. A tool should not submit a load request if
* it already has one pending.
* .iP WTX_ERR_SVR_NOT_ENOUGH_MEMORY
* Cannot allocate memory for internal needs
* .iP WTX_ERR_TGTMEM_NOT_ENOUGH_MEMORY
* Not enough memory in the target-server-managed target memory pool to store the
* object module segments.
* .iP WTX_ERR_LOADER_XXX
* See WTX_OBJ_MODULE_LOAD
*
* SEE ALSO: WTX_OBJ_MODULE_LOAD_2, wtxObjModuleLoad(), 
* wtxObjModuleLoadProgressReport(), wtxObjModuleLoadCancel(),
* .I API Reference Manual: Target Server Internal Routines,
* .I API Programmer's Guide: Object Module Loader
*/

STATUS wtxObjModuleLoadStart
    (
    HWTX                        hWtx,           /* WTX API handle    */
    WTX_LD_M_FILE_DESC *        pFileDesc       /* Module descriptor */
    )
    {
#ifdef HOST
    WTX_LD_M_FILE_DESC *	result;

    /* Keep only WTX_LOAD_FROM_XXX bits */

    pFileDesc->moduleId &= WTX_LOAD_FROM_TARGET_SERVER;

    /* module info request will be reported synchronously */

    if (pFileDesc->loadFlag & LOAD_MODULE_INFO_ONLY)
	pFileDesc->moduleId &= (~WTX_LOAD_ASYNCHRONOUSLY);
    else
	pFileDesc->moduleId |= WTX_LOAD_ASYNCHRONOUSLY;

    internalLoadCall = TRUE;

    result = wtxObjModuleLoad (hWtx, pFileDesc);

    if (result == NULL)
	return (WTX_ERROR);

    /* Free allocated memory */

    wtxResultFree (hWtx, result);

#endif /* HOST */
    return (WTX_OK);
    }

/*******************************************************************************
*
* wtxObjModuleLoadProgressReport - get the asynchronous load status
*
* This routine returns the current load context after a wtxObjModuleLoadStart
* request.
*
* EXPAND ../../../include/wtx.h WTX_LOAD_REPORT_INFO
*
* Use free() to free the output WTX_LOAD_REPORT_INFO parameter, and when
* the load is complete, use wtxSymListFree() to free the undefined symbols
* list if it exists, free also <filename> and <section>.
*
* EXAMPLE:
* Load a module on the target and each second, the load status is evaluated.
*
* .CS
*   ...
*
*   pFileDescIn->filename  = "/folk/pascal/Board/ads860/big.o";
*   pFileDescIn->loadFlag  = LOAD_GLOBAL_SYMBOLS;
*   pFileDescIn->nSections = 0;
*
*   /@ Load the module asynchronously @/
*
*   if (wtxObjModuleLoadStart (wtxh, pFileDescIn) == WTX_ERROR)
*       return (WTX_ERROR);
*
*   /@ Get info about the load @/
*
*   if ((pLoadReport = wtxObjModuleLoadProgressReport (wtxh)) == NULL)
*       return (WTX_ERROR)
*   else
*       {
*       /@ Define some shorthands @/
*
*       #define progress	pLoadReport->STATE_INFO.PROGRESS_INFO
*       #define mod		pLoadReport->STATE_INFO.WTX_LD_M_FILE_DESC
*
*       while (pLoadReport->loadState == LOAD_IN_PROGRESS)
*	    {
*   #ifdef WIN32
*	    Sleep (1000);
*   #else   
*	    sleep (1);
*   #endif
*
*	    /@ Print info about the load request @/
*
*	    printf ("\t\tLOAD_IN_PROGRESS");
*	    printf ("\tPHASE: %d\tdone: %d\tremaining: %d\n", 
*		    progress.state, progress.currentValue, 
*		    progress.maxValue - progress.currentValue);
*
*	    free (pLoadReport);
*
*	    if ((pLoadReport = wtxObjModuleLoadProgressReport (wtxh)) == NULL)
*		{
*		printf ("\nwtxObjModuleProgressReport failed\n");
*		return (WTX_ERROR);
*		}
*	    }
*
*	/@ The load is done: the module is on the target @/
*
*	printf ("\t\tLOAD_DONE\n");
*
*       /@ Do something @/
*
*	... 
*
*	/@ First free any symbols list @/
*
*	wtxSymListFree (&pLoadReport->undefSymList);
*
*       /@ Feeing dynamically allocated memory @/
*
*	if (pLoadReport->filename != NULL)
*           free (pLoadReport->filename);
*
*       if (pLoadReport->section != NULL)
*           free (pLoadReport->section);
*
*       free (pLoadReport);
*
*	...
* .CE
*
* RETURNS: A pointer to a WTX_LOAD_REPORT_INFO for the load status or NULL on
* error.
*
* ERRORS:
* .iP WTX_ERR_API_MEMALLOC 12
* A buffer cannot be allocated
* .iP WTX_ERR_API_INVALID_ARG
* An invalid argument had been received
* .iP WTX_ERR_LOADER_LOAD_CANCELED
* The load had been aborted
* .iP WTX_ERR_LOADER_LOAD_IN_PROGRESS
* The load operation is not finished
* .iP WTX_ERR_LOADER_OBJ_MODULE_NOT_FOUND
* The module was unloaded by another client
* .iP WTX_ERR_SVR_EINVAL
* Request issued out of sequence. A tool should submit a load before issuing
* this request.
* .iP WTX_ERR_SVR_NOT_ENOUGH_MEMORY
* Cannot allocate memory for internal needs
* .iP WTX_ERR_TGTMEM_NOT_ENOUGH_MEMORY
* Not enough memory in the target-server-managed target memory pool to store the
* object module segments.
* .iP WTX_ERR_LOADER_XXX
* See WTX_OBJ_MODULE_LOAD
*
* SEE ALSO: WTX_OBJ_MODULE_LOAD_2, wtxObjModuleLoadStart(), 
* wtxObjModuleLoad(), wtxObjModuleLoadCancel(),
* .I API Reference Manual: Target Server Internal Routines,
* .I API Programmer's Guide: Object Module Loader
*/

WTX_LOAD_REPORT_INFO * wtxObjModuleLoadProgressReport
    (
    HWTX                        hWtx            	/* WTX API handle     */
    )
    {
    WTX_LOAD_REPORT_INFO *      pReportInfo = NULL;	/* load progress info */
#ifdef HOST
    WTX_LD_M_FILE_DESC *        pFileDescIn = NULL;	/* load action spec   */
    WTX_LD_M_FILE_DESC *        pFileDescOut = NULL;	/* load result info   */
    UINT32                      index;			/* index counter      */

#define progress		pReportInfo->STATE_INFO.PROGRESS_INFO
#define mod			pReportInfo->STATE_INFO.WTX_LD_M_FILE_DESC

    /* Allocate memory to store the input / output parameters */

    if ( (pReportInfo = (WTX_LOAD_REPORT_INFO *) 
	calloc (1, sizeof (WTX_LOAD_REPORT_INFO))) == NULL)
        {
        wtxErrSet (hWtx, WTX_ERR_API_MEMALLOC);
        goto error;
        }

    if ( (pFileDescIn = (WTX_LD_M_FILE_DESC *) 
	calloc (1, sizeof (WTX_LD_M_FILE_DESC))) == NULL)
        {
        wtxErrSet (hWtx, WTX_ERR_API_MEMALLOC);
        goto error;
        }

    /* Tell wtxObjModuleLoad() that we are in a PROGRESS_REPORT request */

    pFileDescIn->moduleId = WTX_LOAD_PROGRESS_REPORT;
    internalLoadCall = TRUE;

    /* Do the call */

    pFileDescOut = wtxObjModuleLoad (hWtx, pFileDescIn);

    /* Something failed during the call */

    if (pFileDescOut == NULL)
	{
        goto error;
	}

    /* We have an error, it's OK : this is the answer of PROGRESS_REPORT */

    if (wtxErrGet (hWtx) == WTX_ERR_LOADER_LOAD_IN_PROGRESS)
	{
	/* If we have more than 1 section, it's an error */

	if (pFileDescOut->nSections != 1)
	    {
            wtxErrSet (hWtx, WTX_ERR_API_INVALID_ARG);
            goto error;
	    }

	/* Save the progress report info values */

	pReportInfo->loadState = LOAD_IN_PROGRESS;
	progress.state         = pFileDescOut->section[0].flags;
	progress.maxValue      = pFileDescOut->section[0].addr;
	progress.currentValue  = pFileDescOut->section[0].length;

	/* XXX - pai:
	 * Note that if this mod is accepted, it changes the API
	 * and breaks code which uses this routine as documented
	 * in the comment header.  Expect runtime errors if the
	 * following mod is compiled in.
	 *
	 * /@ add the pointer to the wtxResultFree() list @/
	 *
	 * if (wtxFreeAdd (hWtx, (void *) pReportInfo, (FUNCPTR) free, NULL, 0,
	 *			NULL, WTX_SVR_NONE) != WTX_OK)
	 *   {
	 *   WTX_ERROR_RETURN (hWtx, wtxErrGet (hWtx), NULL);
	 *   }
	 */
	}
    else
	{
	/* Otherwise, the load is done */

        pReportInfo->loadState = LOAD_DONE;

        /* Copy the info from wtxObjModuleLoad */

        mod.loadFlag  = pFileDescOut->loadFlag;
        mod.moduleId  = pFileDescOut->moduleId;
        mod.nSections = pFileDescOut->nSections;

        /* Copy the file name */

        if ((mod.filename = calloc (strlen (pFileDescOut->filename) + 1, 1))
            == NULL)
            {
            wtxErrSet (hWtx, WTX_ERR_API_MEMALLOC);
            goto error;
            }

        strcpy (mod.filename, pFileDescOut->filename);

        /* Copy the sections */

        if ((mod.section = calloc (mod.nSections * sizeof (LD_M_SECTION) , 1))
            == NULL)
            {
            wtxErrSet (hWtx, WTX_ERR_API_MEMALLOC);
            goto error;
            }

        for (index = 0; index < mod.nSections; index++)
            {
            mod.section[index].flags  = pFileDescOut->section[index].flags;
            mod.section[index].addr   = pFileDescOut->section[index].addr;
            mod.section[index].length = pFileDescOut->section[index].length;
            }
 
        /* Copy the undefined symbols list */

        if (pFileDescOut->undefSymList.pSymbol != NULL)
            {
            WTX_SYMBOL * pSymbol;		/* symbol in XDR list     */
            WTX_SYMBOL * pCurrent;		/* newly allocated symbol */
            WTX_SYMBOL * pPrevious = NULL;	/* previous symbol        */
            BOOL         firstSymbol = TRUE;	/* first symbol in list ? */

            for (pSymbol = pFileDescOut->undefSymList.pSymbol; pSymbol;
                 pSymbol = (WTX_SYMBOL *) pSymbol->next)
                {
                if ((pCurrent = (WTX_SYMBOL *) calloc (1, sizeof (WTX_SYMBOL)))
                    == NULL)
                    {
                    wtxErrSet (hWtx, WTX_ERR_API_MEMALLOC);
                    goto error;
                    }

                memcpy ((void *) pCurrent, (void *) pSymbol, 
                        sizeof (WTX_SYMBOL));

                if ((pCurrent->name = calloc (strlen (pSymbol->name) + 1, 
                    sizeof (char))) == NULL)
                    {
                    wtxErrSet (hWtx, WTX_ERR_API_MEMALLOC);
                    goto error;
                    }

                strcpy (pCurrent->name, pSymbol->name);
                pCurrent->next = NULL;

                if (firstSymbol)
                    {
                    mod.undefSymList.pSymbol = pCurrent;
                    firstSymbol = FALSE;
                    }
                else
                    pPrevious->next = pCurrent;

                pPrevious = pCurrent;
                }
            }

	/* XXX - pai:
	 * Note that if this mod is accepted, it changes the API
	 * and breaks code which uses this routine as documented
	 * in the comment header.  Expect runtime errors if the
	 * following mod is compiled in.
	 *
	 * /@ add the pointer to the wtxResultFree() list @/
	 *
	 * if (wtxFreeAdd (hWtx, (void *) pReportInfo,
	 *                (FUNCPTR) wtxObjModuleLoadProgressReportFree, NULL, 0,
	 *                NULL, WTX_SVR_NONE) != WTX_OK)
	 *     {
	 *     WTX_ERROR_RETURN (hWtx, wtxErrGet (hWtx), NULL);
	 *     }
	 */
        }

    /* Free allocated memory */

    free (pFileDescIn);

    wtxResultFree (hWtx, pFileDescOut);

#endif /* HOST */
 
    /* Output parameter */

    return (pReportInfo);

#ifdef HOST
error:

    if (pFileDescIn != NULL)
        free (pFileDescIn);

    if (mod.filename != NULL)
        free (mod.filename);

    if (mod.section != NULL)
        free (mod.section);

    wtxSymListFree (&mod.undefSymList);

    if (pReportInfo != NULL)
        free (pReportInfo);
    
    wtxResultFree (hWtx, pFileDescOut);

    return (NULL);
#endif /* HOST */
    }

#if 0
/* XXX : fle : for SRP#67326 */
#ifdef HOST
/*******************************************************************************
*
* wtxObjModuleLoadProgressReportFree - free a load status report
*
* This routine is to free a load status report that is in the LOAD_DONE state.
* It is only applicable for LOAD_DONE states.
*
* RETURNS: WTX_OK or WTX_ERROR on failure
*/

LOCAL STATUS wtxObjModuleLoadProgressReportFree
    (
    HWTX			hWtx,		/* WTX session handler */
    WTX_LOAD_REPORT_INFO *	pToFree		/* pointer to free     */
    )
    {
    STATUS		status = WTX_ERROR;	/* function status     */

    if ( (pToFree == NULL) || (pToFree->loadState != LOAD_DONE))
	{
	/* one of the given arguments is invalid */

	wtxErrSet (hWtx, WTX_ERR_API_INVALID_ARG);
	goto error;
	}

#define desc pToFree->STATE_INFO.WTX_LD_M_FILE_DESC
    if (desc.filename !=NULL)
	{
	free (desc.filename);
	}
    if (desc.section != NULL)
	{
	free (desc.section);
	}
    if (&desc.undefSymList != NULL)
	{
	wtxSymListFree (&desc.undefSymList);
	}

    free (pToFree);

    /* all the free operations went all right, exit with WTX_OK */

    status = WTX_OK;

error:
    return (status);
    }
#endif /* HOST */
#endif /* 0 */

/*******************************************************************************
*
* wtxObjModuleLoadCancel - Stop an asynchronous load request
*
* This routine stops an asynchronous load request. If the load had been
* already done or it can't be found, then it returns an error. 
*
* RETURNS: WTX_OK or WTX_ERROR on error.
*
* ERRORS:
* .iP WTX_ERR_SVR_INVALID_CLIENT_ID 12
* Tool not registered
* .iP WTX_ERR_SVR_EINVAL
* Request issued out of sequence. A tool should submit a load before issuing
* this request.
* .iP WTX_ERR_LOADER_ALREADY_LOADED
* The load is already finished
*
* SEE ALSO: WTX_COMMAND_SEND, wtxObjModuleLoadStart(), 
* wtxObjModuleLoadProgressReport(),
* .I API Reference Manual: Target Server Internal Routines,
* .I API Programmer's Guide: Object Module Loader
*/

STATUS wtxObjModuleLoadCancel
    (
    HWTX                hWtx           /* WTX API handle   */
    )
    {
#ifdef HOST
    WTX_ERROR_T         callStat;      /* Exchange result  */
    WTX_MSG_PARAM       in;            /* Query parameters */
    WTX_MSG_RESULT      out;           /* Query result     */
    char                buf [32];      /* Input buffer     */

    /*
     * All API calls that require a connection to the target server
     * should use the WTX_CHECK_CONNECTED macro before doing anything else.
     * This macro also calls WTX_CHECK_HANDLE to validate the handle.
     */

    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    /* Set input - ouput parameters for WTX_COMMAND_SEND */

    in.param.valueType = V_PCHAR;
    strcpy (buf, WTX_LOAD_CANCEL_CMD);
    in.param.value_u.v_pchar = buf;

    memset (&out, 0, sizeof (out));

    /* Call WTX_COMMAND_SEND - wtxObjModuleLoadCancel service */

    callStat = exchange (hWtx, WTX_COMMAND_SEND, &in, &out);

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    wtxExchangeFree (hWtx->server, WTX_COMMAND_SEND, &out);

#endif /* HOST */
    /* Tchao */

    return (WTX_OK);
    }

/******************************************************************************
*
* wtxObjModuleUndefSymAdd - add a undefined symbol to the undefined symbol list 
*
* This routine adds the symbol specified by <symId> to the corresponding
* module's undefined symbol list.
*
* RETURNS: WTX_OK or WTX_ERROR.
*
* SEE ALSO: WTX_OBJ_MODULE_UNDEF_SYM_ADD
* NOMANUAL
*/

STATUS wtxObjModuleUndefSymAdd
    (
    HWTX		hWtx,		/* WTX API handle */
    char *		symName,	/* undefined symbol name */
    UINT32		symGroup	/* undefined symbol group */
    )
    {
    WTX_ERROR_T         callStat;
    WTX_MSG_SYMBOL_DESC in;
    WTX_MSG_RESULT      out;

    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    memset (&in, 0, sizeof (in));
    memset (&out, 0, sizeof (out));

    in.symbol.name = symName;
    in.symbol.group = symGroup;

    callStat = exchange (hWtx, WTX_OBJ_MODULE_UNDEF_SYM_ADD, &in, &out);

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    wtxExchangeFree (hWtx->server, WTX_OBJ_MODULE_UNDEF_SYM_ADD, &out);

    return (WTX_OK);
    }

/*******************************************************************************
*
* wtxObjModuleUnload - unload an object module from the target
*
* This routine unloads the module specified by <moduleId> from the target.
* The module ID is returned when the module is loaded or by one of the
* information routines.
*
* RETURNS: WTX_OK or WTX_ERROR.
*
* SEE ALSO: WTX_OBJ_MODULE_UNLOAD_2, wtxObjModuleLoad(), wtxObjModuleList(),
* wtxObjModuleInfoGet()
*/

STATUS wtxObjModuleUnload
    (
    HWTX	hWtx,		/* WTX API handle */
    UINT32	moduleId	/* id of module to unload */
    )
    {
    WTX_ERROR_T         	callStat;
    WTX_MSG_MOD_NAME_OR_ID      in;
    WTX_MSG_RESULT      	out;

    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    memset (&in, 0, sizeof (in));
    memset (&out, 0, sizeof (out));

    in.filename = NULL;
    in.moduleId = moduleId;

    callStat = exchange (hWtx, WTX_OBJ_MODULE_UNLOAD_2, &in, &out);

    if (callStat != WTX_ERR_NONE)
        WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    wtxExchangeFree (hWtx->server, WTX_OBJ_MODULE_UNLOAD_2, &out);

    return (WTX_OK);
    }

/*******************************************************************************
*
* wtxObjModuleByNameUnload - unload an object module from the target
*
* This routine unloads the module specified by <name> from the target.
* The module ID is returned when the module is loaded or by one of the
* information routines.
*
* RETURNS: WTX_OK or WTX_ERROR if exchange failed.
*
* SEE ALSO: WTX_OBJ_MODULE_UNLOAD_2, wtxObjModuleUnload(), 
* wtxObjModuleLoad(), wtxObjModuleList(), wtxObjModuleInfoGet()
*/

STATUS wtxObjModuleByNameUnload
    (
    HWTX        hWtx,                      /* WTX API handle             */
    char *      name                       /* Name of module to look for */
    )
    {
    WTX_ERROR_T                  callStat;
    WTX_MSG_MOD_NAME_OR_ID       in;
    WTX_MSG_RESULT               out;

    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    memset (&in, 0, sizeof (in));
    memset (&out, 0, sizeof (out));

    in.filename = name;
    in.moduleId = 0;

    callStat = exchange (hWtx, WTX_OBJ_MODULE_UNLOAD_2, &in, &out);

    if (callStat != WTX_ERR_NONE)
        WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    wtxExchangeFree (hWtx->server, WTX_OBJ_MODULE_UNLOAD_2, &out);

    return (WTX_OK);
    }

/*******************************************************************************
*
* wtxRegisterForEvent - ask the WTX server to send events matching a regexp
*
* This routine takes a string <regExp>, which is a regular expression,
* and uses it to specify which events this user is interested in. By
* default a WTX client receives no event notifications.
*
* RETURNS: WTX_OK or WTX_ERROR.
*
* SEE ALSO: WTX_REGISTER_FOR_EVENT, wtxUnregisterForEvent(), 
* wtxEventGet(), wtxEventListGet()
*/

STATUS wtxRegisterForEvent 
    (
    HWTX	 hWtx,		/* WTX API handle */
    const char * regExp		/* Regular expression to select events */
    )
    {
    WTX_ERROR_T			callStat;
    WTX_MSG_EVENT_REG_DESC	in;
    WTX_MSG_RESULT		out;

    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    memset (&out, 0, sizeof (out));
    memset (&in, 0, sizeof (in));

    in.evtRegExp = (char *) regExp;

    callStat = exchange (hWtx, WTX_REGISTER_FOR_EVENT, &in, &out);

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    wtxExchangeFree (hWtx->server, WTX_REGISTER_FOR_EVENT, &out);

    return (WTX_OK);
    }

/*******************************************************************************
*
* wtxRegsGet - read register data from the target
*
* This routine reads <numBytes> of raw (target format) register data
* from the register set <regSet> starting from the byte at offset
* <firstByte>. The data read are stored in local memory at <regMemory>.
*
* The context types are given by:
*
* EXPAND ../../../include/wtxtypes.h WTX_CONTEXT_TYPE
*
* EXPAND ../../../include/wtxtypes.h WTX_REG_SET_TYPE
*
* RETURNS: WTX_OK or WTX_ERROR.
*
* SEE ALSO: WTX_REGS_GET, wtxRegsSet()
*/

STATUS wtxRegsGet 
    (
    HWTX		hWtx,		/* WTX API handle */
    WTX_CONTEXT_TYPE	contextType,	/* context type to get regs of */
    WTX_CONTEXT_ID_T	contextId,	/* context id to get regs of */
    WTX_REG_SET_TYPE	regSet,		/* type of register set */
    UINT32		firstByte,	/* first byte of register set */
    UINT32		numBytes,	/* number of bytes of register set */
    void *		regMemory	/* place holder for reg. values */
    )
    {
    WTX_MSG_MEM_XFER_DESC	out;
    WTX_MSG_REG_READ		in;
    WTX_ERROR_T			callStat;
    
    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    memset (&out, 0, sizeof (out));
    memset (&in, 0, sizeof (in));

    in.context.contextType = contextType;
    in.context.contextId = contextId;
    in.regSetType = regSet;
    in.memRegion.baseAddr = (TGT_ADDR_T) firstByte;
    in.memRegion.size = numBytes;

    /* NOTE: This relies on XDR to allocate memory to write register info to */

    callStat = exchange (hWtx, WTX_REGS_GET, &in, &out);

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    /* WTX_REGS_GET is a pain because it may not set the error flag */
    if (out.memXfer.numBytes != numBytes)
	{
	wtxExchangeFree (hWtx->server, WTX_REGS_GET, &out);
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_REGS_GET_PARTIAL_READ, WTX_ERROR);
	}
	
    memcpy(regMemory, out.memXfer.source, out.memXfer.numBytes);
    wtxExchangeFree (hWtx->server, WTX_REGS_GET, &out);

    return (WTX_OK);
    }

/*******************************************************************************
*
* wtxRegsSet - write to registers on the target
*
* This routine writes <numBytes> of raw (target format) register data
* to register set <regSet> starting at offset <firstByte>. Data is
* written from local memory at <regMemory>.
*
* The context types are given by:
*
* EXPAND ../../../include/wtxtypes.h WTX_CONTEXT_TYPE
*
* EXPAND ../../../include/wtxtypes.h WTX_REG_SET_TYPE
*
* RETURNS: WTX_OK or WTX_ERROR if the register set fails.
*
* SEE ALSO: WTX_REGS_SET
*/

STATUS wtxRegsSet 
    (
    HWTX		hWtx,		/* WTX API handle */
    WTX_CONTEXT_TYPE	contextType,	/* context type to set regs of */
    WTX_CONTEXT_ID_T	contextId,	/* context id to set regs of */
    WTX_REG_SET_TYPE	regSet,		/* type of register set */
    UINT32		firstByte,	/* first byte of reg. set */
    UINT32		numBytes,	/* number of bytes in reg. set. */
    void *		regMemory	/* register contents */
    )
    {
    WTX_MSG_RESULT	out;
    WTX_MSG_REG_WRITE	in;
    WTX_ERROR_T		callStat;
    
    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    memset (&out, 0, sizeof (out));
    memset (&in, 0, sizeof (in));

    in.context.contextId = contextId;
    in.context.contextType = contextType;
    in.regSetType = regSet;
    in.memXfer.destination = (TGT_ADDR_T) firstByte;
    in.memXfer.numBytes = numBytes;
    in.memXfer.source = (char *) regMemory;
  
    callStat = exchange (hWtx, WTX_REGS_SET, &in, &out);

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    wtxExchangeFree (hWtx->server, WTX_REGS_SET, &out);

    return (WTX_OK);
    }

/*******************************************************************************
*
* wtxStrToTgtAddr - convert a string argument to a TGT_ADDR_T value
*
* This routine takes a string and converts it to a TGT_ADDR_T.  The string
* is part of an event string.  See wtxEventGet() and the WTX library 
* discussion of WTX events in the \f2API Reference Manual: WTX Protocol\fP.
* 
* RETURNS: The address as a TGT_ADDR_T value.
*/

TGT_ADDR_T wtxStrToTgtAddr 
    (
    HWTX	hWtx,		/* WTX API handle */
    const char *str		/* string */
    )
    {
    WTX_CHECK_HANDLE (hWtx, 0);
    return (TGT_ADDR_T) strtoul (str, NULL, 16); /* Base 16 read */
    }

/*******************************************************************************
*
* wtxStrToContextId - convert a string argument to a WTX_CONTEXT_ID_T
*
* This routine takes a string and converts it to a context ID.  The string
* is part of an event string.  See wtxEventGet() and the WTX library 
* discussion of WTX events in the \f2API Reference Manual: WTX Protocol\fP.
* 
* RETURNS: The context ID.
*/

WTX_CONTEXT_ID_T wtxStrToContextId 
    (
    HWTX	hWtx,		/* WTX API handle */
    const char *str		/* string */
    )
    {
    WTX_CHECK_HANDLE (hWtx, 0);
    return (WTX_CONTEXT_ID_T) strtoul (str, NULL, 16); /* Hex read */
    }

/*******************************************************************************
*
* wtxStrToContextType - convert a string argument to a WTX_CONTEXT_TYPE value
*
* This routine takes a string and converts it to a WTX_CONTEXT_TYPE
* enumeration value.  The string is part of an event string.  See
* wtxEventGet() and the WTX library discussion of WTX events in the \f2API
* Reference Manual: WTX Protocol\fP.
* 
* The context types are given by:
*
* EXPAND ../../../include/wtxtypes.h WTX_CONTEXT_TYPE
*
* RETURNS: The context type or WTX_ERROR on error.
*/

WTX_CONTEXT_TYPE wtxStrToContextType 
    (
    HWTX	 hWtx,		/* WTX API handle */
    const char * str		/* string */
    )
    {
    INT32 value;

    WTX_CHECK_HANDLE (hWtx, WTX_ERROR);

    value = (INT32) strtol (str, NULL, 16); /* Hex read */
    
    if (value < WTX_CONTEXT_SYSTEM || value > WTX_CONTEXT_ISR_ANY)
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_INVALID_ARG, WTX_ERROR);
    else
	return (WTX_CONTEXT_TYPE) value;
    }

/*******************************************************************************
*
* wtxStrToInt32 - convert a string argument to an INT32 value
*
* This routine takes a string and converts it to a signed host integer.  The
* string is part of an event string.  See wtxEventGet() and the WTX library
* discussion of WTX events in the \f2API Reference Manual: WTX Protocol\fP.
* 
* RETURNS: The string as an INT32 value.
*/

INT32 wtxStrToInt32 
    (
    HWTX	hWtx,		/* WTX API handle */
    const char *str		/* string */
    )
    {
    return (INT32) strtol (str, NULL, 16); /* Base 16 read */
    }

/*******************************************************************************
*
* wtxStrToEventType - convert a string to a WTX event enumeration type
*
* This routine is a type converter for the event strings passed
* to a WTX client by the target server. It converts back to the
* event enumeration type so event types can be switched on.
*
* If the string is NULL, WTX_EVT_T_NONE is returned.  If
* the string does not match any of the pre-defined events,
* WTX_EVT_T_OTHER is returned.
*
* The event types are given by:
*
* EXPAND ../../../include/wtxtypes.h WTX_EVENT_TYPE
*
* RETURNS: The event type or WTX_EVT_T_INVALID on error.  
*/

WTX_EVENT_TYPE wtxStrToEventType 
    (
    HWTX	hWtx,		/* WTX API handle */
    const char *str		/* string */
    )
    {
    WTX_CHECK_HANDLE (hWtx, WTX_EVENT_INVALID);

    if (str == NULL)
	return WTX_EVENT_NONE;
    else if (STREQ (str, WTX_EVT_TGT_RESET))
	return WTX_EVENT_TGT_RESET;
    else if (STREQ (str, WTX_EVT_SYM_ADDED))
	return WTX_EVENT_SYM_ADDED;
    else if (STREQ (str,WTX_EVT_SYM_REMOVED))
	return WTX_EVENT_SYM_REMOVED;
    else if (STREQ (str,WTX_EVT_OBJ_LOADED))
	return WTX_EVENT_OBJ_LOADED;
    else if (STREQ (str,WTX_EVT_OBJ_UNLOADED))
	return WTX_EVENT_OBJ_UNLOADED;
    else if (STREQ (str,WTX_EVT_CTX_START))
	return WTX_EVENT_CTX_START;
    else if (STREQ (str,WTX_EVT_CTX_EXIT))
	return WTX_EVENT_CTX_EXIT;
    else if (STREQ (str,WTX_EVT_EXCEPTION))
	return WTX_EVENT_EXCEPTION;
    else if (STREQ (str,WTX_EVT_VIO_WRITE))
	return WTX_EVENT_VIO_WRITE;
    else if (STREQ (str,WTX_EVT_TOOL_ATTACH))
	return WTX_EVENT_TOOL_ATTACH;
    else if (STREQ (str,WTX_EVT_TOOL_DETACH))
	return WTX_EVENT_TOOL_DETACH;
    else if (STREQ (str,WTX_EVT_TOOL_MSG))
	return WTX_EVENT_TOOL_MSG;
    else if (STREQ (str,WTX_EVT_TEXT_ACCESS))
	return WTX_EVENT_TEXT_ACCESS;
    else if (STREQ (str,WTX_EVT_DATA_ACCESS))
	return WTX_EVENT_DATA_ACCESS;
    else if (STREQ (str,WTX_EVT_CALL_RETURN))
	return WTX_EVENT_CALL_RETURN;
    else if (STREQ (str,WTX_EVT_USER))
	return WTX_EVENT_USER;
    else if (STREQ (str,WTX_EVT_TS_KILLED))
	return WTX_EVENT_TS_KILLED;
    else if (STREQ (str,WTX_EVT_EVTPT_ADDED))
	return WTX_EVENT_EVTPT_ADDED;
    else if (STREQ (str,WTX_EVT_EVTPT_DELETED))
	return WTX_EVENT_EVTPT_DELETED;
    else if (STREQ (str,WTX_EVT_UNKNOWN))
	return WTX_EVENT_UNKNOWN;
    else
	return WTX_EVENT_OTHER;
    }

/*******************************************************************************
*
* wtxEventToStrType - convert a WTX event enumeration type to a string
*
* This routine is a string converter for the event type passed
* to the target server. It is the wtxStrToEventType routine counterpart.
*
* If the event is not known , the WTX_EVENT_UNKNOWN string is returned.
*
* The event types are given by:
*
* EXPAND ../../../include/wtxtypes.h WTX_EVENT_TYPE
*
* RETURNS: a string representing the event, or WTX_EVENT_UNKNOWN on error.
*/
 
const char * wtxEventToStrType
    (
    WTX_EVENT_TYPE        event           /* WTX Event */
    )
    {
    switch (event)
	{
	case WTX_EVENT_TGT_RESET:
	    return WTX_EVT_TGT_RESET;

	case WTX_EVENT_SYM_ADDED:
	    return WTX_EVT_SYM_ADDED;

	case WTX_EVENT_SYM_REMOVED:
	    return WTX_EVT_SYM_REMOVED;

	case WTX_EVENT_OBJ_LOADED:
	    return WTX_EVT_OBJ_LOADED;

	case WTX_EVENT_OBJ_UNLOADED:
	    return WTX_EVT_OBJ_UNLOADED;

	case WTX_EVENT_CTX_START:
	    return WTX_EVT_CTX_START;

	case WTX_EVENT_CTX_EXIT:
	    return WTX_EVT_CTX_EXIT;

	case WTX_EVENT_EXCEPTION:
	    return WTX_EVT_EXCEPTION;

	case WTX_EVENT_VIO_WRITE:
	    return WTX_EVT_VIO_WRITE;

	case WTX_EVENT_TOOL_ATTACH:
	    return WTX_EVT_TOOL_ATTACH;

	case WTX_EVENT_TOOL_DETACH:
	    return WTX_EVT_TOOL_DETACH;

	case WTX_EVENT_TOOL_MSG:
	    return WTX_EVT_TOOL_MSG;

	case WTX_EVENT_TEXT_ACCESS:
	    return WTX_EVT_TEXT_ACCESS;

	case WTX_EVENT_DATA_ACCESS:
	    return WTX_EVT_DATA_ACCESS;

	case WTX_EVENT_CALL_RETURN:
	    return WTX_EVT_CALL_RETURN;

	case WTX_EVENT_USER:
	    return WTX_EVT_USER;

	case WTX_EVENT_NONE:
	    return WTX_EVT_NONE;

        case WTX_EVENT_HW_BP:
            return WTX_EVT_HW_BP;

	case WTX_EVENT_OTHER:
	    return WTX_EVT_OTHER;

	case WTX_EVENT_INVALID:
	    return WTX_EVT_INVALID;

	case WTX_EVENT_TS_KILLED:
	    return WTX_EVT_TS_KILLED;

	case WTX_EVENT_EVTPT_ADDED:
	    return WTX_EVT_EVTPT_ADDED;

	case WTX_EVENT_EVTPT_DELETED:
	    return WTX_EVT_EVTPT_DELETED;

	default:
	    return WTX_EVT_UNKNOWN;
	}

    return WTX_EVT_UNKNOWN;
    }

/******************************************************************************
*
* wtxServiceAdd - add a new service to the agent
*
* This command adds a new service to the target server. The service is 
* provided in WDB- and WTX-level object modules. The WDB object module is 
* downloaded to the target, and the WTX object module is dynamically linked
* to the target server. 
* 
* The <wtxInProcName> and <wtxOutProcName> arguments are the names of 
* known structure filter procedures used to exchange data with the new
* service.
*
* <wtxObjFile> is the name of a dynamically loadable library file that
* contain the WTX service routine, the service initialization routine, and
* XDR filters code for the service input and output parameters.
* 
* Upon success the service number to be used for future requests to the newly
* added service is returned in <mySvcNumber>.
*
* Example:
* .CS
*
*   /@ 
*    * Add a WTX service called mySvc and get the service number in 
*    * mySvcNumber.
*    @/
*
*   if (wtxServiceAdd 
*           (
*           wtxh, 		       /@ our tool handle @/
*           &mySvcNumber, 	       /@ where to return service num. @/
*           NULL, NULL, NULL, NULL,    /@ no WDB items @/
*           "mySvc.so", 	       /@ new service DLL @/
*           "mySvcName", 	       /@ service routine name @/
*           "xdr_WTX_MY_SVC_IN_PARAM", /@ input param XDR filter @/
*           "xdr_WTX_MY_SVC_OUT_PARAM" /@ output param XDR filter @/
*           ) != OK)
*       {
*       printf("Error: cannot add mySvc service:%s\n",wtxErrMsgGet(wtxh));
*       wtxTerminate (wtxh);		/@ terminate tool connection @/
*       exit(0);
*       }
*
*     ...
*
*   /@ get our client handle in order to call the newly added service @/
*
*   pClient = (CLIENT *) wtxh->server->transport;
*
*   /@ set client call timeout @/
*
*   timeout.tv_sec = 10;
*   timeout.tv_usec = 0;
*
*   /@ Call the newly added service @/
*
*   rpcStat = clnt_call (           
*       pClient,                                /@ client handle @/
*       mySvcNumber,                            /@ service number @/
*       xdr_WTX_MY_SVC_IN_PARAM,                /@ request XDR filter @/
*       (char *) pIn,                           /@ ptr to input data @/
*       xdr_WTX_MY_SVC_OUT_PARAM,               /@ reply XDR filter @/
*       (char *) pOut,                          /@ ptr to result @/
*       timeout                                 /@ request timeout value @/
*       );
*
* .CE
*
* WARNING: In the Tornado 1.0 release, wtxServiceAdd() can only add WTX 
* services that do not require an additional WDB service.  The <wdbSvcNum>,
* <wdbName>, <wdbObjFile> and <wdbSvcInitRtn> parameters must be set to NULL.
* 
* RETURNS: WTX_OK or WTX_ERROR.
*
* SEE ALSO: WTX_SERVICE_ADD
*/

STATUS wtxServiceAdd
    (
    HWTX	hWtx,		/* WTX API handle                         */
    UINT32 *	wtxSvcNum,	/* where to return WTX RPC service number */
    UINT32	wdbSvcNum,	/* WDB RPC service number                 */
    char *	wdbName,	/* WDB service name                       */
    char *	wdbObjFile,	/* WDB service object module              */
    char *	wdbSvcInitRtn,	/* WDB svc initialization routine name    */
    char *	wtxObjFile,	/* WTX xdr/service DLL file               */
    char *	wtxName,	/* WTX service routine name               */
    char *	wtxInProcName,	/* WTX service input xdr procedure name   */
    char *	wtxOutProcName	/* WTX service output xdr procedure name  */
    )
    {
    WTX_MSG_SERVICE_DESC	in;			/* exch input desc  */
    WTX_MSG_RESULT		out;			/* exch result desc */
    WTX_ERROR_T			callStat;		/* exch error code  */
    STATUS			status = WTX_ERROR;	/* returned status  */

    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    /* first of all, check that all the WTX given parameters are not NULL */

    if ( (wtxObjFile == NULL) || (wtxName == NULL) || (wtxInProcName == NULL) ||
	 (wtxOutProcName == NULL))
	{
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_INVALID_ARG, WTX_ERROR);
	}

    /* clean in and out items */

    memset (&in, 0, sizeof (in));
    memset (&out, 0, sizeof (out));

    in.pWtxServiceDesc = (WTX_WTX_SERVICE_DESC *)
                         malloc (sizeof (WTX_WTX_SERVICE_DESC));

    if (in.pWtxServiceDesc == NULL)
	{
	wtxErrSet (hWtx, WTX_ERR_API_MEMALLOC);
	goto error1;
	}

    /*
     * allocate memory for the WDB service desc only if one of the WDB
     * parameters is not NULL
     */

    if ( (wdbSvcNum != 0) || (wdbName != NULL) || (wdbObjFile != NULL) ||
	 (wdbSvcInitRtn != NULL))
	{
	in.pWdbServiceDesc = (WTX_WDB_SERVICE_DESC *)
			     malloc (sizeof (WTX_WDB_SERVICE_DESC));
	if (in.pWdbServiceDesc == NULL)
	    {
	    wtxErrSet (hWtx, WTX_ERR_API_MEMALLOC);
	    goto error2;
	    }

	/* initialize the WDB service description */

	in.pWdbServiceDesc->rpcNum = wdbSvcNum;
	in.pWdbServiceDesc->name = wdbName;
	in.pWdbServiceDesc->svcObjFile = wdbObjFile;
	in.pWdbServiceDesc->initRtnName = wdbSvcInitRtn;
	}

    in.pWtxServiceDesc->svcObjFile = wtxObjFile;
    in.pWtxServiceDesc->svcProcName = wtxName;
    in.pWtxServiceDesc->inProcName = wtxInProcName;
    in.pWtxServiceDesc->outProcName = wtxOutProcName;

    callStat = exchange (hWtx, WTX_SERVICE_ADD, &in, &out);

    if (callStat != WTX_ERR_NONE)
	{
	wtxErrSet (hWtx, callStat);
	goto error3;
	}

    status = WTX_OK;
    *wtxSvcNum = out.val.value_u.v_uint32;
    wtxExchangeFree (hWtx->server, WTX_SERVICE_ADD, &out);

error3:
    /* free the input descriptions */
    if (in.pWdbServiceDesc != NULL)
	free (in.pWdbServiceDesc);

error2:
    if (in.pWtxServiceDesc != NULL)
	free (in.pWtxServiceDesc);

error1:
    return status;
    }

/******************************************************************************
*
* wtxSymAdd - add a symbol with the given name, value, and type
*
* This routine adds a new symbol with the specified name, value, and type to 
* the target server symbol table.
*
* RETURNS: WTX_OK or WTX_ERROR.
*
* SEE ALSO: WTX_SYM_ADD, wtxSymFind(), wtxSymListGet(), wtxSymRemove()
*/

STATUS wtxSymAdd
    (
    HWTX		hWtx,	/* WTX API handle */
    const char *	name,	/* name of symbol to add */
    TGT_ADDR_T		value,	/* value of symbol to add */
    UINT8 		type	/* type of symbol to add */
    )
    {
    WTX_MSG_SYMBOL_DESC	in;		/* input to WTX call */
    WTX_MSG_RESULT 	out;
    UINT32 		result;
    WTX_ERROR_T		callStat;	/* client status */
    
    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    memset (&in, 0, sizeof (in));
    memset (&out, 0, sizeof (out));

    in.symbol.name = (char *) name;
    in.symbol.value = value;
    in.symbol.type = type;

    callStat = exchange (hWtx, WTX_SYM_ADD, &in, &out);	

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    result = out.val.value_u.v_uint32;
    wtxExchangeFree (hWtx->server, WTX_SYM_ADD, &out);

    return (result);
    }

/******************************************************************************
*
* wtxSymAddWithGroup - add a symbol with the given name, value, type and group
*
* This routine adds a new symbol with the specified name, value, type and 
* group to the target server symbol table.
*
* RETURNS: WTX_OK or WTX_ERROR.
*
* SEE ALSO: WTX_SYM_ADD, wtxSymFind(), wtxSymListGet(), wtxSymRemove()
*/

STATUS wtxSymAddWithGroup
    (
    HWTX		hWtx,	/* WTX API handle */
    const char *	name,	/* name of symbol to add */
    TGT_ADDR_T		value,	/* value of symbol to add */
    UINT8 		type,	/* type of symbol to add */
    UINT16		group	/* group of symbol to add */
    )
    {
    WTX_MSG_SYMBOL_DESC	in;		/* input to WTX call */
    WTX_MSG_RESULT 	out;
    UINT32 		result;
    WTX_ERROR_T		callStat;	/* client status */
    
    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    memset (&in, 0, sizeof (in));
    memset (&out, 0, sizeof (out));

    in.symbol.name = (char *) name;
    in.symbol.value = value;
    in.symbol.type = type;
    in.symbol.group = group;

    callStat = exchange (hWtx, WTX_SYM_ADD, &in, &out);	

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    result = out.val.value_u.v_uint32;
    wtxExchangeFree (hWtx->server, WTX_SYM_ADD, &out);

    return (result);
    }

/*******************************************************************************
* 
* wtxSymFind - find information on a symbol given its name or value
*
* This routine searches for a symbol which matches the supplied
* search criteria.  If <symName> is non-NULL, the search is for a symbol
* whose name matches <symName>.  If <symName> is NULL, the 
* search is for one with value <symValue>.  When searching by name,
* the <exactName> flag is used to control whether the name must exactly
* match <symName> or only partially.  In both value and name searches,
* a symbol type <symType> can be supplied to further refine the search.
* Valid symbol types are defined in a_out.h.  To prevent the type being
* considered in the search set <typeMask> to 0.
*
* The result of the search is a pointer to a symbol which 
* must be freed by the user calling wtxResultFree().
*
* EXPAND ../../../include/wtxmsg.h WTX_SYMBOL
*
* RETURNS: A pointer to the symbol or NULL on error.
*
* SEE ALSO: WTX_SYM_FIND, wtxResultFree().
*/

WTX_SYMBOL * wtxSymFind
    (
    HWTX			hWtx,		/* WTX API handle */
    const char *		symName,	/* name of symbol */
    TGT_ADDR_T			symValue,	/* value of symbol */
    BOOL32			exactName,	/* must match name exactly */
    UINT8			symType,	/* type of symbol */
    UINT8			typeMask	/* mask to select type bits */
    )
    {
    WTX_MSG_SYMBOL_DESC		in;		/* input to WTX call */
    WTX_MSG_SYMBOL_DESC *	pOut;		/* result pointer */
    WTX_ERROR_T			callStat;	/* client status */
    
    WTX_CHECK_CONNECTED (hWtx, NULL);

    pOut = calloc (1, sizeof (WTX_MSG_SYMBOL_DESC));
    if (pOut == NULL)
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_MEMALLOC, NULL);

    memset (&in, 0, sizeof (in));

    in.symbol.name = (char *) symName;
    in.symbol.exactName = exactName;
    in.symbol.value = symValue;
    in.symbol.type = symType;
    in.symbol.typeMask = typeMask;

    callStat = exchange (hWtx, WTX_SYM_FIND, &in, pOut);

    if (callStat != WTX_ERR_NONE)
	{
	free (pOut);		/* Free allocated message */
	WTX_ERROR_RETURN (hWtx, callStat, NULL);
	}

    /* add pointer to the wtxResultFree () list */

    if (wtxFreeAdd (hWtx, (void *) &pOut->symbol, NULL, pOut, WTX_SYM_FIND,
		    hWtx->server, WTX_SVR_SERVER) != WTX_OK)
	{
	WTX_ERROR_RETURN (hWtx, wtxErrGet (hWtx), NULL);
	}

    return (&pOut->symbol);
    }    

/*******************************************************************************
*
* wtxSymListGet - get a list of symbols matching the given search criteria
*
* This routine gets a list of symbols matching given search criteria.
* <substring> is a symbol substring to match and <value> is a target symbol
* address value to search around.  Either <string> or <value> should be set
* to a non-NULL value, but not both. In addition, if <listUnknown> is set to
* TRUE then only unknown symbols are listed and <substring> and <value> are
* ignored.  If <moduleName> is non-NULL, it specifies the module to search.
* Even if no symbols match the search parameters, the pointer returned is
* not NULL. It points to a WTX_SYM_LIST with a NULL `pSymbol' field.
*
* The return value must be freed by the caller using wtxResultFree().
*
* EXPAND ../../../include/wtxmsg.h WTX_SYM_LIST
*
* EXPAND ../../../include/wtxmsg.h WTX_SYMBOL
*
* NOTE : The symbols allocated by wtxSymListGet() should be freed by caller.
*
* RETURNS: A pointer to a list of symbols or NULL on error.
* 
* SEE ALSO: WTX_SYM_LIST_GET, wtxResultFree()
*/

WTX_SYM_LIST * wtxSymListGet 
    (
    HWTX		hWtx,		/* WTX API handle */
    const char *	substring,	/* Symbol name substring to match */
    const char *	moduleName,	/* Module name to search in */
    TGT_ADDR_T		value,		/* Symbol value to match */
    BOOL32		listUnknown	/* List unknown symbols only flag */
    )
    {
    return (wtxSymListByModuleNameGet (hWtx, substring, moduleName, 
				       value, listUnknown));
    }

/*******************************************************************************
*
* wtxSymListByModuleIdGet - get a list of symbols given a module Id
*
* This routine gets a list of symbols matching given search criteria.
* <substring> is a symbol substring to match and <value> is a target symbol
* address value to search around.  Either <string> or <value> should be set
* to a non-NULL value, but not both. In addition, if <listUnknown> is set to
* TRUE then only unknown symbols are listed and <substring> and <value> are
* ignored.  If <moduleId> is non-zero, it specifies the module to search.
* Even if no symbols match the search parameters, the pointer returned is
* not NULL. It points to a WTX_SYM_LIST with a NULL `pSymbol' field.
*
* The return value must be freed by the caller using wtxResultFree().
*
* EXPAND ../../../include/wtxmsg.h WTX_SYM_LIST
*
* EXPAND ../../../include/wtxmsg.h WTX_SYMBOL
*
* NOTE : The symbols allocated by wtxSymListGet() should be freed by caller.
*
* RETURNS: A pointer to a list of symbols or NULL on error.
* 
* SEE ALSO: WTX_SYM_LIST_GET, wtxSymListGet(), wtxSymListByModuleNameGet(),
* wtxResultFree()
*/

WTX_SYM_LIST * wtxSymListByModuleIdGet
    (
    HWTX		hWtx,		/* WTX API handle                     */
    const char *	substring,	/* Symbol name substring to match     */
    UINT32		moduleId,	/* Module Id to search in             */
    TGT_ADDR_T		value,		/* Symbol value to match              */
    BOOL32		listUnknown	/* List unknown symbols only flag     */
    )
    {
    WTX_MSG_SYM_MATCH_DESC	in;		/* query parameters           */
    WTX_MSG_SYM_LIST *		pOut;		/* output message             */
    WTX_MSG_SYM_LIST		out;		/* local message              */
    WTX_SYMBOL *		firstSym = NULL;/* first symbol of list       */
    WTX_SYMBOL *		lastSym;	/* last symbol of list        */
    WTX_SYMBOL *		newSymbol;	/* new symbol in list         */
    WTX_SYMBOL *		precSymbol = NULL;/* preceding symbol in list */
    WTX_ERROR_T			callStat;	/* exchange result            */
    STATUS			status;		/* wtxResultFree() status     */
    int				stringLength;	/* string length for calloc   */
    
    WTX_CHECK_CONNECTED (hWtx, NULL);		/* checking fot connection */

    /* checking for variables incompatibility */

    if ( (value != 0) && ( (substring != NULL) || (moduleId != 0)))
        {
        WTX_ERROR_RETURN (hWtx, WTX_ERR_API_INVALID_ARG, NULL);
        }

    if ( (listUnknown && (moduleId == 0)) ||
         (listUnknown && (substring != NULL)) ||
         (listUnknown && (value != 0)))
        {
        WTX_ERROR_RETURN (hWtx, WTX_ERR_API_INVALID_ARG, NULL);
        }

    /* list get parameters initialization */

    memset (&in, 0, sizeof (in));

    in.wtxCore.objId = hWtx->msgToolId.wtxCore.objId;

    in.symTblId = 0;			/* Use default target symbol table */
    in.matchString = (substring != NULL);
    in.adrs = value;
    in.stringToMatch = (char *) substring;
    in.byModuleName = FALSE;
    in.module.moduleId = moduleId;
    in.listUnknownSym = listUnknown;
    in.giveMeNext = FALSE;

    pOut = (WTX_MSG_SYM_LIST *)calloc (1, sizeof (WTX_MSG_SYM_LIST));
    if (pOut == NULL)
        WTX_ERROR_RETURN (hWtx, WTX_ERR_API_MEMALLOC, NULL);

    lastSym = NULL;

    do
	{
	memset (&out, 0, sizeof (out));		/* Zero output */

        /* call for target server routine */

	callStat = exchange (hWtx, WTX_SYM_LIST_GET, &in, &out);

	if (callStat != WTX_ERR_NONE)
	    {
	    pOut->symList.pSymbol = firstSym;
	    wtxSymListFree (&pOut->symList);
	    free (pOut);
	    WTX_ERROR_RETURN (hWtx, callStat, NULL);
	    }

        lastSym = out.symList.pSymbol;

        if ((firstSym == NULL) && (lastSym != NULL))	/* first query */
            {
            if ((newSymbol = (WTX_SYMBOL *) calloc (1, sizeof (WTX_SYMBOL)))
                 == NULL)
                WTX_ERROR_RETURN (hWtx, WTX_ERR_API_MEMALLOC, NULL);

            *newSymbol = *lastSym;

            if (lastSym->name != NULL)
                {
                stringLength = strlen (lastSym->name);
                if ((newSymbol->name = (char *)calloc (stringLength + 1,
                                                        sizeof (char))) == NULL)
                    WTX_ERROR_RETURN (hWtx, WTX_ERR_API_MEMALLOC, NULL);

                strcpy (newSymbol->name, lastSym->name);
                }

            if (lastSym->moduleName != NULL)
                {
                stringLength = (int) strlen (lastSym->moduleName);
                if ( (newSymbol->moduleName = (char *)calloc (stringLength + 1, 
                                                              sizeof (char)))
                     == NULL)
                    WTX_ERROR_RETURN (hWtx, WTX_ERR_API_MEMALLOC, NULL);

                strcpy (newSymbol->moduleName, lastSym->moduleName);
                }

            firstSym = newSymbol;
            precSymbol = newSymbol;

            lastSym = lastSym->next;
            }

	while (lastSym != NULL)
            {
            newSymbol = (WTX_SYMBOL *)calloc (1, sizeof (WTX_SYMBOL));

            /* this should copy all fields, even a NULL for next */

            *newSymbol = *lastSym;

            /* allocating memory for string fields, then copy them */

            if (lastSym->name != NULL)
                {
                stringLength = strlen (lastSym->name);
                if ((newSymbol->name = (char *)calloc (stringLength + 1,
						       sizeof (char))) == NULL)
                    WTX_ERROR_RETURN (hWtx, WTX_ERR_API_MEMALLOC, NULL);

                strcpy (newSymbol->name, lastSym->name);
                }

            if (lastSym->moduleName != NULL)
                {
                stringLength = (int) strlen (lastSym->moduleName);
                if ( (newSymbol->moduleName = (char *)calloc (stringLength + 1, 
                                                              sizeof (char)))
                     == NULL)
                    WTX_ERROR_RETURN (hWtx, WTX_ERR_API_MEMALLOC, NULL);

                strcpy (newSymbol->moduleName, lastSym->moduleName);
                }

            /* updating local symbol list */

            precSymbol->next = newSymbol;
            precSymbol = precSymbol->next;

            lastSym = lastSym->next;
            }

	/* Ensure next time around, if any gets the remainng symbols */

	in.giveMeNext = out.moreToCome;

        wtxExchangeFree (hWtx->server, WTX_SYM_LIST_GET, &out);
	} while (in.giveMeNext);

    /* Set up pOut so that it can be freed by wtxResultFree() */

    pOut->wtxCore.protVersion = (UINT32) pOut;
    pOut->wtxCore.objId = WTX_SYM_LIST_GET;
 
    /* Then set the symbol pointer to the first symbol in the list */

    pOut->symList.pSymbol = firstSym;
    
    /* add the pointer description to the list of pointer to free */

    if ( (status = wtxFreeAdd (hWtx,  (void *) &pOut->symList,
			       (FUNCPTR) wtxSymListFree, pOut, 0, NULL,
			       WTX_SVR_NONE)) != WTX_OK)
	{
	WTX_ERROR_RETURN (hWtx, wtxErrGet (hWtx), NULL);
	}


    return &pOut->symList;
    }

/*******************************************************************************
*
* wtxSymListByModuleNameGet - get a list of symbols given a module name
*
* This routine gets a list of symbols matching given search criteria.
* <substring> is a symbol substring to match and <value> is a target symbol
* address value to search around.  Either <string> or <value> should be set
* to a non-NULL value, but not both. In addition, if <listUnknown> is set to
* TRUE then only unknown symbols are listed and <substring> and <value> are
* ignored.  If <moduleName> is non-NULL, it specifies the module to search.
* Even if no symbols match the search parameters, the pointer returned is
* not NULL. It points to a WTX_SYM_LIST with a NULL `pSymbol' field.
*
* The return value must be freed by the caller using wtxResultFree().
*
* EXPAND ../../../include/wtxmsg.h WTX_SYM_LIST
*
* EXPAND ../../../include/wtxmsg.h WTX_SYMBOL
*
* NOTE : The symbols allocated by wtxSymListGet() should be freed by caller.
*
* RETURNS: A pointer to a list of symbols or NULL on error.
* 
* SEE ALSO: WTX_SYM_LIST_GET, wtxSymListGet(), wtxSymListByModuleIdGet(),
* wtxResultFree()
*/

WTX_SYM_LIST * wtxSymListByModuleNameGet 
    (
    HWTX		hWtx,		/* WTX API handle */
    const char *	substring,	/* Symbol name substring to match */
    const char *	moduleName,	/* Module name to search in */
    TGT_ADDR_T		value,		/* Symbol value to match */
    BOOL32		listUnknown	/* List unknown symbols only flag */
    )
    {
    WTX_MSG_SYM_MATCH_DESC	in;			/* query parameters */
    WTX_MSG_SYM_LIST *		pOut;			/* output message */
    WTX_MSG_SYM_LIST		out;			/* local message */
    WTX_SYMBOL *		firstSym = NULL;	/* first sym of list */
    WTX_SYMBOL *		lastSym;		/* last sym of list */
    WTX_SYMBOL *		newSymbol;		/* new symbol in list */
    WTX_SYMBOL *		precSymbol = NULL;	/* prev sym in list */
    WTX_ERROR_T			callStat;		/* exchange result */
    STATUS			status = WTX_ERROR;	/* routine status */
    int				stringLength;		/* str len for calloc */
    
    WTX_CHECK_CONNECTED (hWtx, NULL);		/* checking fot connection */

    /* checking for variables incompatibility */

    if ((value != 0) && ((substring != NULL) || (moduleName != NULL)))
        {
        WTX_ERROR_RETURN (hWtx, WTX_ERR_API_INVALID_ARG, NULL);
        }

    if ( (listUnknown && (moduleName == NULL)) ||
         (listUnknown && (substring != NULL)) ||
         (listUnknown && (value != 0)))
        {
        WTX_ERROR_RETURN (hWtx, WTX_ERR_API_INVALID_ARG, NULL);
        }

    /* list get parameters initialization */

    memset (&in, 0, sizeof (in));

    in.wtxCore.objId = hWtx->msgToolId.wtxCore.objId;

    in.symTblId = 0;			/* Use default target symbol table */
    in.matchString = (substring != NULL);
    in.adrs = value;
    in.stringToMatch = (char *) substring;
    in.byModuleName = TRUE;
    in.module.moduleName = (char *) moduleName;
    in.listUnknownSym = listUnknown;
    in.giveMeNext = FALSE;

    pOut = (WTX_MSG_SYM_LIST *)calloc (1, sizeof (WTX_MSG_SYM_LIST));
    if (pOut == NULL)
        WTX_ERROR_RETURN (hWtx, WTX_ERR_API_MEMALLOC, NULL);

    lastSym = NULL;

    do
	{
	memset (&out, 0, sizeof (out));		/* Zero output */

        /* call for target server routine */

	callStat = exchange (hWtx, WTX_SYM_LIST_GET, &in, &out);

	if (callStat != WTX_ERR_NONE)
	    {
	    pOut->symList.pSymbol = firstSym;
	    wtxSymListFree (&pOut->symList);
	    free (pOut);
	    WTX_ERROR_RETURN (hWtx, callStat, NULL);
	    }

        lastSym = out.symList.pSymbol;

        if ( (firstSym == NULL) && (lastSym != NULL))	/* first query */
            {
            if ( (newSymbol = (WTX_SYMBOL *)calloc (1, sizeof (WTX_SYMBOL)))
                 == NULL)
                WTX_ERROR_RETURN (hWtx, WTX_ERR_API_MEMALLOC, NULL);

            *newSymbol = *lastSym;

            if (lastSym->name != NULL)
                {
                stringLength = strlen (lastSym->name);
                if ( (newSymbol->name = (char *)calloc (stringLength + 1,
                                                        sizeof (char))) == NULL)
                    WTX_ERROR_RETURN (hWtx, WTX_ERR_API_MEMALLOC, NULL);

                strcpy (newSymbol->name, lastSym->name);
                }

            if (lastSym->moduleName != NULL)
                {
                stringLength = (int) strlen (lastSym->moduleName);
                if ( (newSymbol->moduleName = (char *)calloc (stringLength + 1, 
                                                              sizeof (char)))
                     == NULL)
                    WTX_ERROR_RETURN (hWtx, WTX_ERR_API_MEMALLOC, NULL);

                strcpy (newSymbol->moduleName, lastSym->moduleName);
                }

            firstSym = newSymbol;
            precSymbol = newSymbol;

            lastSym = lastSym->next;
            }

	while (lastSym != NULL)
            {
            newSymbol = (WTX_SYMBOL *)calloc (1, sizeof (WTX_SYMBOL));

            /* this should copy all fields, even a NULL for next */

            *newSymbol = *lastSym;

            /* allocating mempry for string fields, then copy them */

            if (lastSym->name != NULL)
                {
                stringLength = strlen (lastSym->name);
                if ( (newSymbol->name = (char *)calloc (stringLength + 1,
                                                        sizeof (char))) == NULL)
                    WTX_ERROR_RETURN (hWtx, WTX_ERR_API_MEMALLOC, NULL);

                strcpy (newSymbol->name, lastSym->name);
                }

            if (lastSym->moduleName != NULL)
                {
                stringLength = (int) strlen (lastSym->moduleName);
                if ( (newSymbol->moduleName = (char *)calloc (stringLength + 1, 
                                                              sizeof (char)))
                     == NULL)
                    WTX_ERROR_RETURN (hWtx, WTX_ERR_API_MEMALLOC, NULL);

                strcpy (newSymbol->moduleName, lastSym->moduleName);
                }

            /* updating local symbol list */

            precSymbol->next = newSymbol;
            precSymbol = precSymbol->next;

            lastSym = lastSym->next;
            }

	/* Ensure next time around, if any gets the remainng symbols */

	in.giveMeNext = out.moreToCome;

        wtxExchangeFree (hWtx->server, WTX_SYM_LIST_GET, &out);
	} while (in.giveMeNext);

    /* Set up pOut so that it can be freed by wtxResultFree() */

    pOut->wtxCore.protVersion = (UINT32) pOut;
    pOut->wtxCore.objId = WTX_SYM_LIST_GET;
 
    /* Then set the symbol pointer to the first symbol in the list */

    pOut->symList.pSymbol = firstSym;

    /* add pointer to the wtxResultFree(0 list */

    if ( (status = wtxFreeAdd (hWtx,  (void *) &pOut->symList,
			       (FUNCPTR) wtxSymListFree, pOut, 0, NULL,
			       WTX_SVR_NONE)) != WTX_OK)
	{
	WTX_ERROR_RETURN (hWtx, wtxErrGet (hWtx), NULL);
	}

    return &pOut->symList;
    }

/*******************************************************************************
*
* wtxSymRemove - remove a symbol from the default target server symbol table
*
* This routine removes a symbol from the default target server symbol table.
* <symName> and <symType> identify the symbol to remove.
*
* RETURNS: WTX_OK or WTX_ERROR.
*
* SEE ALSO: WTX_SYM_REMOVE, wtxSymAdd(), wtxSymFind(), wtxSymListGet()
*/

STATUS wtxSymRemove 
    (
    HWTX	hWtx,		/* WTX API handle */
    const char *symName,	/* Name of symbol to remove */
    UINT8	symType		/* Type of symbol to remove */
    )
    {
    WTX_MSG_SYMBOL_DESC	in;		/* input to WTX call */
    WTX_MSG_RESULT	out;		/* result pointer */
    WTX_ERROR_T		callStat;	/* client status */
    
    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    memset (&in, 0, sizeof (in));
    memset (&out, 0, sizeof (out));

    in.symbol.name = (char *) symName;
    in.symbol.type = symType;

    callStat = exchange (hWtx, WTX_SYM_REMOVE, &in, &out);

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    wtxExchangeFree (hWtx->server, WTX_SYM_REMOVE, &out);

    return WTX_OK;
    }

/*******************************************************************************
*
* wtxSymTblInfoGet - return information about the target server symbol table
*
* This routine queries the target server to find out information about its
* symbol table. The return value must be freed by the caller using
* wtxResultFree().
*
* EXPAND ../../../include/wtxmsg.h WTX_SYM_TBL_INFO
*
* RETURNS: A pointer to the target server symbol-table information structure
* or NULL on error.
*
* SEE ALSO: WTX_SYM_TBL_INFO_GET
*/

WTX_SYM_TBL_INFO * wtxSymTblInfoGet
    (
    HWTX        hWtx    /* WTX API handle */
    )
    {
    WTX_MSG_SYM_TBL_INFO *      pOut;
    WTX_ERROR_T                 callStat;

    WTX_CHECK_CONNECTED (hWtx, NULL);

    pOut = calloc (1, sizeof (WTX_MSG_SYM_TBL_INFO));
    if (pOut == NULL)
        WTX_ERROR_RETURN (hWtx, WTX_ERR_API_MEMALLOC, NULL);

    callStat = exchange (hWtx, WTX_SYM_TBL_INFO_GET, &hWtx->msgToolId, pOut);

    if (callStat != WTX_ERR_NONE)
        {
        free (pOut);
        WTX_ERROR_RETURN (hWtx, callStat, NULL);
        }

    /* add pointer to the wtxResultFree () list */

    if (wtxFreeAdd (hWtx, (void *) &pOut->symTblId, NULL, pOut,
		    WTX_SYM_TBL_INFO_GET, hWtx->server, WTX_SVR_SERVER)
	!= WTX_OK)
	{
	WTX_ERROR_RETURN (hWtx, wtxErrGet (hWtx), NULL);
	}

    return ((WTX_SYM_TBL_INFO *) &(pOut->symTblId));
    }

/*******************************************************************************
*
* wtxTargetReset - reset the target 
*
* This routine resets the target as if it was manually rebooted.
* The target server restarts itself and if the reboot is successful,
* it reconnects to the target. If the call is successful a target
* reset event, TGT_RESET, is generated and sent to all clients
* registered to receive them. 
*
* NOTE: The client making the request must reconnect to the target
* server as restarting the server causes the server connection to
* be broken.
*
* RETURNS: WTX_OK or WTX_ERROR on failure.
*
* SEE ALSO: WTX_TARGET_RESET
*/

STATUS wtxTargetReset 
    (
    HWTX	hWtx	/* WTX API handle */
    )
    {
    WTX_ERROR_T	callStat;
    WTX_MSG_RESULT	out;

    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    memset (&out, 0, sizeof (out));

    callStat = exchange (hWtx, WTX_TARGET_RESET, &hWtx->msgToolId, &out);

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    wtxExchangeFree (hWtx->server, WTX_TARGET_RESET, &out);

    return (WTX_OK);
    }

/******************************************************************************
*
* wtxTsNameGet - get the full name of the currently attached target server
*
* This routine returns the full name of the target server to which the
* handle <hWtx> is attached (if any).  The string is an internal one and
* must not be freed by the caller.
*
* RETURNS: The target server name string or NULL if not connected.
*/

const char * wtxTsNameGet
    (
    HWTX	hWtx	/* WTX API handle */
    )
    {
    WTX_CHECK_HANDLE (hWtx, NULL);

    if (wtxToolConnected (hWtx))
	return hWtx->pServerDesc->wpwrName;
    else
	return NULL;
    }

/*******************************************************************************
*
* wtxTargetRtTypeGet - get the target-runtime-type information
*
* This routine returns a numeric value indicating the runtime type
* as returned by the target agent.  
*
* RETURNS: The runtime type code as a INT32 or WTX_ERROR on error.
*/

INT32 wtxTargetRtTypeGet 
    (
    HWTX hWtx	/* WTX API handle */
    )
    {
    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    if (hWtx->pTsInfo == NULL && wtxTsInfoGet (hWtx) == NULL)
	/* Error has already been dispatched, just return WTX_ERROR */
	return WTX_ERROR;
    
    return hWtx->pTsInfo->tgtInfo.rtInfo.rtType;
    }

/*******************************************************************************
*
* wtxTargetRtVersionGet - get the target-runtime-version information
*
* This routine returns a string containing the version information for
* the runtime of the current target.  The return value is a pointer to the
* string in temporary memory; the string must be copied if its value is to be 
* stored.
*
* RETURNS: The version string or NULL on error.
*/

const char * wtxTargetRtVersionGet 
    (
    HWTX hWtx	/* WTX API handle */
    )
    {
    WTX_CHECK_CONNECTED (hWtx, NULL);

    if (hWtx->pTsInfo == NULL && wtxTsInfoGet (hWtx) == NULL)
	/* Error has already been dispatched, just return NULL */
	return NULL;
    
    return hWtx->pTsInfo->tgtInfo.rtInfo.rtVersion;
    }

/*******************************************************************************
*
* wtxTargetCpuTypeGet - get the target CPU type code
*
* This routine returns a numeric value indicating the type of the CPU
* as returned by the target agent.  
*
* RETURNS: The CPU type code as a INT32 or WTX_ERROR on error.
*/

INT32 wtxTargetCpuTypeGet 
    (
    HWTX hWtx	/* WTX API handle */
    )
    {
    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    if (hWtx->pTsInfo == NULL && wtxTsInfoGet (hWtx) == NULL)
	/* Error has already been dispatched, just return WTX_ERROR */
	return WTX_ERROR;
    
    return hWtx->pTsInfo->tgtInfo.rtInfo.cpuType;
    }

/*******************************************************************************
*
* wtxTargetHasFppGet - check if the target has a floating point processor
*
* This routine returns a boolean indicating whether the current target has a
* floating point processor available. Errors must be detected using
* wtxErrClear() and wtxErrGet().
*
* RETURNS: TRUE if it has a floating point processor or FALSE if not or if 
* there was an error.
*/

BOOL32 wtxTargetHasFppGet 
    (
    HWTX hWtx	/* WTX API handle */
    )
    {
    WTX_CHECK_CONNECTED (hWtx, FALSE);

    if (hWtx->pTsInfo == NULL && wtxTsInfoGet (hWtx) == NULL)
	/* Error has already been dispatched, just return WTX_ERROR */
	return FALSE;
    
    return ((hWtx->pTsInfo->tgtInfo.rtInfo.hasCoprocessor & WTX_FPP_MASK) >>
	    WTX_FPP_BITSHIFT);
    }
/*******************************************************************************
*
* wtxTargetHasAltivecGet - check if the target has an altivec unit
*
* This routine returns a boolean indicating whether the current target has a
* altivec coprocessor available. Errors must be detected using
* wtxErrClear() and wtxErrGet().
*
* RETURNS: TRUE if it has an Altivec unit  or FALSE if not or if 
* there was an error.
*/

BOOL32 wtxTargetHasAltivecGet 
    (
    HWTX hWtx	/* WTX API handle */
    )
    {
    WTX_CHECK_CONNECTED (hWtx, FALSE);

    if (hWtx->pTsInfo == NULL && wtxTsInfoGet (hWtx) == NULL)
	/* Error has already been dispatched, just return WTX_ERROR */
	return FALSE;
    
    return ((hWtx->pTsInfo->tgtInfo.rtInfo.hasCoprocessor & WTX_ALTIVEC_MASK) >>
	    WTX_ALTIVEC_BITSHIFT);
    }

/*******************************************************************************
*
* wtxTargetHasDspGet - check if the target has a DSP unit
*
* This routine returns a boolean indicating whether the current target has a
* DSP coprocessor available. Errors must be detected using
* wtxErrClear() and wtxErrGet().
*
* RETURNS: TRUE if it has an DSP unit  or FALSE if not or if 
* there was an error.
*/

BOOL32 wtxTargetHasDspGet 
    (
    HWTX hWtx	/* WTX API handle */
    )
    {
    WTX_CHECK_CONNECTED (hWtx, FALSE);

    if (hWtx->pTsInfo == NULL && wtxTsInfoGet (hWtx) == NULL)
	/* Error has already been dispatched, just return WTX_ERROR */
	return FALSE;
    
    return (hWtx->pTsInfo->tgtInfo.rtInfo.hasCoprocessor & WTX_DSP_MASK) >> WTX_DSP_BITSHIFT;

    }

/*******************************************************************************
*
* wtxTargetEndianGet - get the endianness of the target CPU
*
* This routine returns WTX_ENDIAN_BIG if the target has a CPU that uses
* big-endian byte storage ordering (the most significant byte first) or 
* WTX_ENDIAN_LITTLE if it uses little-endian ordering (the least significant
* byte first).  If an error occurs, the routine returns WTX_ENDIAN_UNKNOWN.
*
* EXPAND ../../../include/wtx.h WTX_ENDIAN_T
*
* RETURNS: The CPU endian value or WTX_ENDIAN_UNKNOWN.
*/

WTX_ENDIAN_T wtxTargetEndianGet 
    (
    HWTX hWtx	/* WTX API handle */
    )
    {
    WTX_CHECK_CONNECTED (hWtx, WTX_ENDIAN_UNKNOWN);

    if (hWtx->pTsInfo == NULL && wtxTsInfoGet (hWtx) == NULL)
	/* Error has already been dispatched, just return error value */
	return WTX_ENDIAN_UNKNOWN;
    
    switch (hWtx->pTsInfo->tgtInfo.rtInfo.endian)
	{
	case 1234:
	    return WTX_ENDIAN_BIG;
	case 4321:
	    return WTX_ENDIAN_LITTLE;
	default:
	    return WTX_ENDIAN_UNKNOWN;
	}
    }

/*******************************************************************************
*
* wtxTargetBootlineGet - get the target bootline information string
*
* This routine returns a string containing the target bootline.  The
* return value is a pointer to the string in temporary memory; the string
* must be copied if it is to be stored.
*
* RETURNS: The bootline string or NULL on error.  
*/

const char * wtxTargetBootlineGet 
    (
    HWTX hWtx	/* WTX API handle */
    )
    {
    WTX_CHECK_CONNECTED (hWtx, NULL);

    if (hWtx->pTsInfo == NULL && wtxTsInfoGet (hWtx) == NULL)
	/* Error has already been dispatched, just return NULL */
	return NULL;
    
    return hWtx->pTsInfo->tgtInfo.rtInfo.bootline;
    }

/*******************************************************************************
*
* wtxTargetBspNameGet - get the target board-support-package name string
*
* This routine returns a string containing the board-support-package name for
* the runtime of the current target.  The return value is a pointer to 
* the string in temporary memory; the string must be copied if it is to be 
* stored.
*
* RETURNS: The BSP name string or NULL on error.
*/

const char * wtxTargetBspNameGet 
    (
    HWTX hWtx	/* WTX API handle */
    )
    {
    WTX_CHECK_CONNECTED (hWtx, NULL);

    if (hWtx->pTsInfo == NULL && wtxTsInfoGet (hWtx) == NULL)
	/* Error has already been dispatched, just return NULL */
	return NULL;
    
    return hWtx->pTsInfo->tgtInfo.rtInfo.bspName;
    }

/*******************************************************************************
*
* wtxToolNameGet - return the name of the current tool
*
* This function gets a string representing the name of the current tool.
* The return value is a pointer to the string in temporary memory; the string
* must be copied if it is to be stored.
*
* RETURNS: The tool name string or NULL on error.
*/

char * wtxToolNameGet
    (
    HWTX        hWtx	/* WTX API handle */
    )
    {
    WTX_CHECK_CONNECTED (hWtx, NULL);

    if (hWtx->pToolDesc != NULL)
        return hWtx->pToolDesc->toolName;
    else
        return NULL;
    }

/******************************************************************************
*
* wtxTsVersionGet - return the Tornado version
*
* This function gets a string representing the Tornado version.
* The return value is a pointer to temporary memory and must be copied if it is
* to be stored.
*
* RETURNS: A pointer to the Tornado version string or NULL on error.
*/

char * wtxTsVersionGet
    (
    HWTX        hWtx	/* WTX API handle */
    )
    {
    WTX_CHECK_CONNECTED (hWtx, NULL);

    if (hWtx->pTsInfo == NULL && wtxTsInfoGet (hWtx) == NULL)
        /* Error has already been dispatched, just return NULL */
        return NULL;

    return hWtx->pTsInfo->version;
    }

/******************************************************************************
*
* wtxTsKill - kill the target server
*
* This routine kills the target server.  The client detaches from the
* target server at this time.  Another attachment can subsequently be
* made.
*
* RETURNS: WTX_OK or WTX_ERROR.
*
* SEE ALSO: WTX_OBJ_KILL, wtxToolAttach()
*/

STATUS wtxTsKill
    (
    HWTX	hWtx	/* WTX API handle */
    )
    {
    WTX_ERROR_T		callStat;	/* WTX RPC status */
    WTX_MSG_KILL_DESC	in;		/* WTX kill message */
    WTX_MSG_RESULT	out;		/* WTX result message */
    
    WTX_CHECK_CONNECTED (hWtx, 0);

    memset (&in, 0, sizeof (in));
    memset (&out, 0, sizeof (out));

    in.request = WTX_OBJ_KILL_DESTROY;
    callStat = exchange (hWtx, WTX_OBJ_KILL, &in, &out);

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    wtxExchangeFree (hWtx->server, WTX_OBJ_KILL, &out);

    toolCleanup(hWtx);

    return (WTX_OK);
    }

/******************************************************************************
*
* wtxTsRestart - restart the target server
*
* This routine restarts the target server.  The client detaches from the
* target server at this time.  Another attachment can subsequently be
* made.
*
* RETURNS: WTX_OK or WTX_ERROR.
*
* SEE ALSO: WTX_OBJ_KILL, wtxToolAttach()
*/

STATUS wtxTsRestart
    (
    HWTX	hWtx	/* WTX API handle */
    )
    {
    WTX_ERROR_T		callStat;	/* WTX RPC status */
    WTX_MSG_KILL_DESC	in;		/* WTX kill message */
    WTX_MSG_RESULT	out;		/* WTX result message */
    
    WTX_CHECK_CONNECTED (hWtx, 0);

    memset (&in, 0, sizeof (in));
    memset (&out, 0, sizeof (out));

    in.request = WTX_OBJ_KILL_RESTART;
    callStat = exchange (hWtx, WTX_OBJ_KILL, &in, &out);

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    wtxExchangeFree (hWtx->server, WTX_OBJ_KILL, &out);

    toolCleanup(hWtx);

    return (WTX_OK);
    }

/*******************************************************************************
*
* wtxUnregisterForEvent - ask the target server to not send events matching
* a regexp
*
* This routine registers which events are unwanted. Usually, if we want to
* filter events notification we have to give the regular expression to
* WTX_REGISTER_FOR_EVENT. A more simple way to do this is to be registered
* for all the events and to be registered for only those which are unwanted.
*
* RETURNS: WTX_OK or WTX_ERROR if exchange failed
*
* SEE ALSO: WTX_UNREGISTER_FOR_EVENT, wtxRegisterForEvent(), wtxEventGet(),
* wtxEventListGet().
*/

STATUS wtxUnregisterForEvent
    (
    HWTX         hWtx,          /* WTX API handle */
    char *       regExp         /* Regular expression to select events */
    )
    {
    WTX_ERROR_T                 callStat;
    WTX_MSG_EVENT_REG_DESC      in;
    WTX_MSG_RESULT              out;

    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    memset (&out, 0, sizeof (out));
    memset (&in, 0, sizeof (in));

    in.evtRegExp = (char *) regExp;

    callStat = exchange (hWtx, WTX_UNREGISTER_FOR_EVENT, &in, &out);

    if (callStat != WTX_ERR_NONE)
        WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    wtxExchangeFree (hWtx->server, WTX_UNREGISTER_FOR_EVENT, &out);

    return (WTX_OK);
    }

/******************************************************************************
*
* wtxFuncCall - call a function on the target
*
* This routine calls the function identified by WTX_CONTEXT_DESC.
* 
* EXPAND ../../../include/wtx.h WTX_CONTEXT_DESC
*
* Execution starts at `entry', with up to 10 integer arguments specified in
* the array `args[]'.  The return type is specified by setting `returnType'
* to WTX_RETURN_TYPE_INT or WTX_RETURN_TYPE_DOUBLE.
*
* NOTE: Because calling a function creates a task context on the target
* system, it cannot be used when the target agent is in external mode.
* 
* ERRORS:
* .iP WTX_ERR_API_INVALID_ARG 12
* <pContextDesc> is invalid.
*
* RETURNS: The ID of the function call or WTX_ERROR.  When the called
* function terminates, a CALL_RETURN event is generated which
* references this ID and contains the return value.
*
* SEE ALSO: WTX_FUNC_CALL, wtxEventGet(), wtxDirectCall(),
* .I API Reference Manual: WTX Protocol WTX_FUNC_CALL
*/

WTX_CONTEXT_ID_T wtxFuncCall
    (
    HWTX		hWtx,		/* WTX API handle */
    WTX_CONTEXT_DESC *  pContextDesc	/* pointer to call descriptor */
    )
    {
    WTX_ERROR_T			callStat;	/* status of RPC */
    WTX_MSG_CONTEXT_DESC	in;		/* params to WTX call */
    WTX_MSG_RESULT		out;		/* result of WTX call */
    WTX_CONTEXT_ID_T		callId;

    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    if (pContextDesc == NULL)
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_INVALID_ARG, WTX_ERROR);

    memset (&in, 0, sizeof (in));
    memset (&out, 0, sizeof (out));

    /* Fill in the context create descriptor message. */

    /*
     * XXX - must have WTX_CONTEXT_DESC fields identical to those
     * WTX_MSG_CONTEXT_DESC from contextType onwards. Awaits change to
     * message definitions on wtxmsg.h so this hack will go away.
     */

    memcpy (&in.contextType, pContextDesc, sizeof (WTX_CONTEXT_DESC));
    
    callStat = exchange (hWtx, WTX_FUNC_CALL, &in, &out);

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    callId = out.val.value_u.v_uint32; 

    wtxExchangeFree (hWtx->server, WTX_FUNC_CALL, &out);

    return callId;
    }

/******************************************************************************
*
* wtxDirectCall - call a function on the target within the agent
*
* This routine calls the function at <entry>, with up to 10 integer 
* arguments <arg0>... <arg9>, in the context of the target agent.  
*
* The return value of the function called on the target is returned at
* the address pointed to by <pRetVal>.
*
* NOTE: The called function is executed within the target agent allowing
* tools to call functions when the agent is running in both task and external
* mode.  This service should only be used to call simple functions that 
* return immediately in order to avoid locking access to the target for other
* tools.  It is preferable to use wtxFuncCall() to call functions that may take
* longer to return.  If the function called from wtxDirectCall() gets an 
* exception, the agent stops and communication with the target board is
* lost.
*
* CAVEAT: This service cannot be used to call functions that return a double.
* 
* RETURNS: WTX_OK or WTX_ERROR if cannot call the function or if <pRetVal>
* is NULL.  
*
* SEE ALSO: WTX_DIRECT_CALL, wtxFuncCall()
*/

STATUS wtxDirectCall
    (
    HWTX	hWtx,		/* WTX API handle */
    TGT_ADDR_T	entry,		/* function address */
    void *	pRetVal,	/* pointer to return value */
    TGT_ARG_T	arg0,		/* function arguments */
    TGT_ARG_T	arg1,	
    TGT_ARG_T	arg2,	
    TGT_ARG_T	arg3,	
    TGT_ARG_T	arg4,	
    TGT_ARG_T	arg5,	
    TGT_ARG_T	arg6,	
    TGT_ARG_T	arg7,	
    TGT_ARG_T	arg8,	
    TGT_ARG_T	arg9	
    )
    {
    WTX_ERROR_T			callStat;	/* status of RPC */
    WTX_MSG_CONTEXT_DESC	in;		/* params to WTX call */
    WTX_MSG_RESULT		out;		/* result of WTX call */

    /* check for invalid parameter */

    if (pRetVal == NULL)
	return (WTX_ERROR);

    /* check tool connection */

    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    memset (&in, 0, sizeof (in));
    memset (&out, 0, sizeof (out));

    /* Fill in the context create descriptor message */

    in.entry	= entry;
    in.args[0]	= arg0;
    in.args[1]	= arg1;
    in.args[2]	= arg2;
    in.args[3]	= arg3;
    in.args[4]	= arg4;
    in.args[5]	= arg5;
    in.args[6]	= arg6;
    in.args[7]	= arg7;
    in.args[8]	= arg8;
    in.args[9]	= arg9;
    
    /* perform the call */

    callStat = exchange (hWtx, WTX_DIRECT_CALL, &in, &out);

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    /* set target function return value */

    *(UINT32 *)pRetVal = out.val.value_u.v_uint32; 

    wtxExchangeFree (hWtx->server, WTX_DIRECT_CALL, &out);

    return WTX_OK;
    }

/*******************************************************************************
* 
* wtxTsInfoGet - get information about the target server, target, and its link
*
* This routine queries the target server to find out information about
* the currently connected target and target server.  The result is a pointer
* to internal memory that must not be freed by the user.  The value in
* memory may change after any subsequent WTX API calls so it must be copied 
* if the values are to be saved.
*
* EXPAND ../../../include/wtx.h WTX_TS_INFO
*
* where WTX_TGT_LINK_DESC is:
*
* EXPAND ../../../include/wtxmsg.h WTX_TGT_LINK_DESC
*
* and WTX_TGT_INFO is:
*
* EXPAND ../../../include/wtxmsg.h WTX_TGT_INFO
*
* WTX_TOOL_DESC is:
*
* EXPAND ../../../include/wtxmsg.h WTX_TOOL_DESC
*
* with WTX_AGENT_INFO:
*
* EXPAND ../../../include/wtxmsg.h WTX_AGENT_INFO
*
* and WTX_RT_INFO:
*
* EXPAND ../../../include/wtxmsg.h WTX_RT_INFO
*
* NOTE: This call actively queries the target server for information.
* Each successful call returns the latest information. 
*
* RETURNS: A pointer to the target server information structure or NULL on 
* error.
*
* SEE ALSO: WTX_TS_INFO_GET
*/

WTX_TS_INFO * wtxTsInfoGet 
    (
    HWTX		hWtx		/* WTX API handle */
    )
    {
    WTX_MSG_TS_INFO *	pOut;
    WTX_ERROR_T		callStat;	/* WTX status */
    UINT32		infoMsgVer = WTX_TS_INFO_GET_V2;

    WTX_CHECK_CONNECTED (hWtx, NULL);

    pOut = calloc (1, sizeof (WTX_MSG_TS_INFO));
    if (pOut == NULL)
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_MEMALLOC, NULL);

    /*
     * For description of WTX_TS_INFO_GET_V1 vs WTX_TS_INFO_GET_V2
     * and the two calls to exchange(), see comments in wtxmsg.h
     */

    callStat = exchange (hWtx, infoMsgVer, &hWtx->msgToolId, pOut);

    if (callStat != WTX_ERR_NONE)
	{
	infoMsgVer = WTX_TS_INFO_GET_V1;
	callStat = exchange (hWtx, infoMsgVer, &hWtx->msgToolId, pOut);
	}

    if (callStat != WTX_ERR_NONE)
	{
	free (pOut);		/* Free allocated message */
	WTX_ERROR_RETURN (hWtx, callStat, NULL);
	}

    /* Free the old target server info stored in the handle */
    if (hWtx->pTsInfo != NULL)
	{
	wtxResultFree (hWtx, hWtx->pTsInfo);
	}

    hWtx->pTsInfo = (WTX_TS_INFO *) &pOut->tgtLinkDesc;

    /* add pointer to the wtxResultFree () list */

    if (wtxFreeAdd (hWtx, (void *) &pOut->tgtLinkDesc, NULL,
		    pOut, infoMsgVer, hWtx->server, WTX_SVR_SERVER)
	!= WTX_OK)
	{
	WTX_ERROR_RETURN (hWtx, wtxErrGet (hWtx), NULL);
	}

    return (hWtx->pTsInfo);
    }

/*******************************************************************************
* 
* wtxTsLock - lock the connected target server for exclusive access
*
* This routine locks the target server that the client is connected to
* so it can only be used by this user and no others.  Other clients
* already connected can continue to use the target server 
* until they disconnect.  When a target server is locked the only
* operations another user may perform are connecting, getting the
* target server information, and disconnecting.
* 
* RETURNS: WTX_OK or WTX_ERROR.
*
* SEE ALSO: WTX_TS_LOCK, wtxTsUnlock()
*/

STATUS wtxTsLock 
    (
    HWTX		hWtx 		/* WTX API handle */
    )
    {
    WTX_ERROR_T		callStat;	/* WTX status */
    WTX_MSG_TS_LOCK	in;
    WTX_MSG_RESULT	out;

    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    memset (&in, 0, sizeof (in));
    memset (&out, 0, sizeof (out));

    in.lockType = 0;    /* ??? Is the lock type documented anywhere ??? */

    callStat = exchange (hWtx, WTX_TS_LOCK, &in, &out);

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    wtxExchangeFree (hWtx->server, WTX_TS_LOCK, &out);

    return (WTX_OK);
    }

/*******************************************************************************
* 
* wtxTsTimedLock - lock the connected target server a limited time
*
* This routine locks the target server that the client is connected to so it 
* can only be used by this user and no others.  Other clients already connected
* can continue to use the target server until they disconnect.  When a target 
* server is locked the only operations another user may perform are connecting,
* getting the target server information, and disconnecting.
* 
* The target server may be locked indefinitely (<seconds> = WTX_LOCK_FOREVER),
* otherwise if the target server must be locked only for a fixed time, then the
* duration must be set in <seconds>.
*
* RETURNS: WTX_OK or WTX_ERROR.
*
* SEE ALSO: WTX_TS_LOCK, wtxTsLock(), wtxTsUnlock()
*/

STATUS wtxTsTimedLock 
    (
    HWTX		hWtx,		/* WTX API handle */
    UINT32		seconds		/* duration of lock */
    )
    {
    WTX_ERROR_T		callStat;	/* WTX status */
    WTX_MSG_TS_LOCK	in;
    WTX_MSG_RESULT	out;

    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    memset (&in, 0, sizeof (in));
    memset (&out, 0, sizeof (out));

    if (seconds > 0)
        in.lockType = seconds;
    else
        in.lockType = 0;

    callStat = exchange (hWtx, WTX_TS_LOCK, &in, &out);

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    wtxExchangeFree (hWtx->server, WTX_TS_LOCK, &out);

    return (WTX_OK);
    }

/*******************************************************************************
* 
* wtxTsUnlock - unlock the connected target server
*
* This routine unlocks the target server that the client is connected to
* so it can be accessed by any user on its access list.  The unlock will
* only succeed if it is being unlocked by the same user that locked it.
*
* RETURNS: WTX_OK or WTX_ERROR.
*
* SEE ALSO: WTX_TS_UNLOCK, wtxTsLock()
*/

STATUS wtxTsUnlock 
    (
    HWTX		hWtx		/* WTX API handle */
    )
    {
    WTX_ERROR_T		callStat;	/* WTX status */
    WTX_MSG_TS_UNLOCK	in;
    WTX_MSG_RESULT	out;

    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    memset (&in, 0, sizeof (in));
    memset (&out, 0, sizeof (out));

    in.force = FALSE;	/* ??? Is this documented anywhere ??? */

    callStat = exchange (hWtx, WTX_TS_UNLOCK, &in, &out);

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    wtxExchangeFree (hWtx->server, WTX_TS_UNLOCK, &out);

    return (WTX_OK);
    }

/*******************************************************************************
*
* wtxVerify - verify that a WTX handle is valid for use
*
* This routine returns WTX_OK if the WTX handle <hWtx> is valid. If an
* invalid handle is passed to a WTX routine, the error
* WTX_ERR_API_INVALID_HANDLE occurs. A handle that has been released using
* wtxTerminate() is invalid.
*
* RETURNS: TRUE if the handle is valid or FALSE if not.
*
* SEE ALSO: wtxInitialize(), wtxTerminate()
*/

BOOL32 wtxVerify
    (
    HWTX	hWtx		/* WTX API handle */
    )
    {
    return (hWtx != NULL && hWtx->self == hWtx);
    }

/*******************************************************************************
* 
* wtxVioChanGet - get a virtual I/O channel number
*
* This routine returns a free virtual I/O channel number to be
* used for redirection of target I/O to the host.  The returned
* channel number is an INT32 between 1 and 255.  (Channel 0 is
* reserved for the default console.) When no longer required, the
* channel number is released using wtxVioChanRelease().
*
* RETURNS: The new channel number or WTX_ERROR on failure.
*
* SEE ALSO: WTX_VIO_CHAN_GET, wtxVioChanRelease()
*/
INT32 wtxVioChanGet
    (
    HWTX hWtx	/* WTX API handle */
    )
    {
    WTX_ERROR_T		callStat;
    WTX_MSG_RESULT	out;
    INT32		result;

    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);
    memset (&out, 0, sizeof (out));

    callStat = exchange (hWtx, WTX_VIO_CHAN_GET, &hWtx->msgToolId, &out);

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    result = out.val.value_u.v_int32;
    wtxExchangeFree (hWtx->server, WTX_VIO_CHAN_GET, &out);

    return result;
    }

/*******************************************************************************
*
* wtxVioChanRelease - release a virtual I/O channel
*
* This request releases a virtual I/O channel number previously obtained
* using wtxVioChanGet() and returns it to the free-channel-number pool of
* the target server.  Valid channel numbers are in the range 1 to 255.
*
* RETURNS: WTX_OK or WTX_ERROR.
*
* SEE ALSO: WTX_VIO_CHAN_RELEASE, wtxVioChanGet()
*/

STATUS wtxVioChanRelease 
    (
    HWTX hWtx,		/* WTX API handle */
    INT32 channel	/* the channel number to release */
    )
    {
    WTX_ERROR_T		callStat;
    WTX_MSG_PARAM	in;
    WTX_MSG_RESULT	out;

    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    memset (&in, 0, sizeof (in));
    memset (&out, 0, sizeof (out));

    in.param.valueType = V_INT32;
    in.param.value_u.v_int32 = channel;

    callStat = exchange (hWtx, WTX_VIO_CHAN_RELEASE, &in, &out);

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    wtxExchangeFree (hWtx->server, WTX_VIO_CHAN_RELEASE, &out);

    return (WTX_OK);
    }

/*******************************************************************************
*
* wtxVioCtl - perform a control operation on a virtual I/O channel
*
* This routine performs the special I/O operation specified by <request>
* on the virtual I/O channel <channel>. An operation-specific
* parameter is supplied to the operation by <arg>.
* 
* RETURNS: WTX_OK or WTX_ERROR.
*
* SEE ALSO: WTX_VIO_CTL, wtxVioChanGet()
*/

STATUS wtxVioCtl
    (
    HWTX	hWtx,		/* WTX API handle */
    INT32	channel,	/* channel to do control operation on */
    UINT32	request,	/* control operation to perform */
    UINT32	arg		/* arg for call */
    )
    {
    WTX_ERROR_T			callStat;	/* WTX status */
    WTX_MSG_VIO_CTL_DESC	in;
    WTX_MSG_RESULT		out;

    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    memset (&in, 0, sizeof (in));
    memset (&out, 0, sizeof (out));

    in.channel = channel;
    in.request = request;
    in.arg = arg;

    callStat = exchange (hWtx, WTX_VIO_CTL, &in, &out);

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    wtxExchangeFree (hWtx->server, WTX_VIO_CTL, &out);

    return (WTX_OK);
    }

/*******************************************************************************
*
* wtxVioFileList - list the files managed by the target server
*
* This routine returns a list of all the target-server-managed files along
* with information about each file including its name, state, and associated
* virtual I/O channel redirection.  The result is a pointer to a pointer to
* the first item in a linked list of file descriptors. It must be freed by
* the caller using wtxResultFree(). If the list is empty (no files are
* managed by the target server), the result points to a NULL value.
*
* EXPAND ../../../include/wtxmsg.h WTX_VIO_FILE_DESC
*
* RETURNS: A pointer to a pointer to the file list or NULL on error.
* 
* SEE ALSO: WTX_VIO_FILE_LIST, wtxFileOpen(), wtxFileClose()
*/

WTX_VIO_FILE_DESC ** wtxVioFileList 
    (
    HWTX hWtx		/* WTX API handle */
    )
    {
    WTX_ERROR_T			callStat;
    WTX_MSG_VIO_FILE_LIST *	pOut;

    WTX_CHECK_CONNECTED (hWtx, NULL);

    pOut = calloc (1, sizeof (WTX_MSG_VIO_FILE_LIST));
    if (pOut == NULL)
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_MEMALLOC, NULL);

    callStat = exchange (hWtx, WTX_VIO_FILE_LIST, &hWtx->msgToolId, pOut);

    if (callStat != WTX_ERR_NONE)
	{
	free (pOut);		/* Free allocated message */
	WTX_ERROR_RETURN (hWtx, callStat, NULL);
	}

    /* add pointer to the wtxResultFree() list */

    if (wtxFreeAdd (hWtx, (void *) &pOut->pVioFileList, NULL, pOut,
		    WTX_VIO_FILE_LIST, hWtx->server, WTX_SVR_SERVER) != WTX_OK)
	{
	WTX_ERROR_RETURN (hWtx, wtxErrGet (hWtx), NULL);
	}

    return &pOut->pVioFileList;
    }

/*******************************************************************************
*
* wtxVioWrite - write data to a VIO channel
*
* This routine writes at most <numBytes> of data starting at <pData>
* to the virtual I/O channel number <channel>.  The buffer size is 512 bytes;
* if <numBytes> is greater than 512, the extra bytes are discarded.
*
* RETURNS: The number of bytes actually written or WTX_ERROR.
*
* SEE ALSO: WTX_VIO_WRITE, wtxEventGet(), wtxVioChanGet()
*/

UINT32 wtxVioWrite
    (
    HWTX	hWtx,		/* WTX API handle */
    INT32	channel,	/* channel to write to */
    void *	pData,		/* pointer to data to write */
    UINT32	numBytes	/* number of bytes of data to write */
    )
    {
    WTX_ERROR_T			callStat;	/* WTX status */
    WTX_MSG_VIO_COPY_DESC	in;
    WTX_MSG_RESULT		out;
    UINT32			bytesWritten;

    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    memset (&in, 0, sizeof (in));
    memset (&out, 0, sizeof (out));

    in.channel = channel;
    in.maxBytes = numBytes;
    in.baseAddr = pData;

    callStat = exchange (hWtx, WTX_VIO_WRITE, &in, &out);

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    bytesWritten = out.val.value_u.v_uint32;
    wtxExchangeFree (hWtx->server, WTX_VIO_WRITE, &out);

    return bytesWritten;
    }

/*******************************************************************************
*
* wtxTargetAttach - reattach to the target
*
* This routine reattaches the client to a target that has been disconnected
* or for which the target server is waiting to connect. 
* 
* RETURNS: WTX_OK if the target is successfully attached or WTX_ERROR 
* if the target is still unavailable.
*
* SEE ALSO: WTX_TARGET_ATTACH
*/

STATUS wtxTargetAttach
    (
    HWTX	hWtx	/* WTX API handle */
    )
    {
    WTX_ERROR_T	callStat;
    WTX_MSG_RESULT	out;

    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    memset (&out, 0, sizeof (out));

    callStat = exchange (hWtx, WTX_TARGET_ATTACH, &hWtx->msgToolId, &out);

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    wtxExchangeFree (hWtx->server, WTX_TARGET_ATTACH, &out);

    return (WTX_OK);
    }

/*******************************************************************************
*
* wtxProbe - probe to see if the registry service is running
*
* This routine returns WTX_OK if the registry service is running. Otherwise
* it returns WTX_ERROR.
*
* RETURNS: WTX_OK or WTX_ERROR.
*/

STATUS wtxProbe 
    (
    HWTX hWtx	/* WTX API handle */
    )
    {
    WTX_ERROR_T callStat;

    WTX_CHECK_HANDLE (hWtx, WTX_ERROR);

    if (registryConnect (hWtx) != WTX_OK)
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_REGISTRY_UNREACHABLE, WTX_ERROR);

    callStat = wtxExchange (hWtx->registry, NULLPROC, NULL, NULL);

    /* close registry connection now since we don't need it anymore */
    registryDisconnect (hWtx);

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    return (WTX_OK);
    }

/*******************************************************************************
*
* wtxRegister - register the target server with the Tornado registry service
*
* This routine registers the named target server with the WTX registry daemon.
* If the specified name does not conflict with any other registered server,
* the registry allocates, initializes, and adds a WTX descriptor structure
* containing a unique RPC program number to its internal tables.  If
* specified, <name> must be unique for the whole system, and by convention
* it is a legal C identifier.  If <name> is NULL, a unique name is allocated
* by the registry and returned to the caller. Only one server can be
* registered per WTX handle.
*
* The return value is a pointer to internal data and must not be freed
* by the user.
*
* EXPAND ../../../include/wtxtypes.h WTX_DESC
*
* RETURNS: A pointer to a WTX_DESC or NULL if the registration failed.
*
* ERRORS:
* .iP WTX_ERR_API_SERVICE_ALREADY_REGISTERED 12
* The server has already registered itself.
* .iP WTX_ERR_API_REGISTRY_UNREACHABLE 
* The registry service is not reachable (in other words, the registry 
* daemon is not running).
* .iP WTX_ERR_API_MEMALLOC 
* There are insufficient resources to allocate a new WTX descriptor for 
* the server.
* .iP WTX_ERR_REGISTRY_NAME_CLASH
* Name already exists in Tornado registry database.
*
* SEE ALSO: WTX_REGISTER, wtxUnregister(), wtxInfo()
*/

WTX_DESC * wtxRegister
    (
    HWTX		hWtx,		/* WTX API handle                     */
    const char *	name,		/* service name, NULL to be named     */
    const char *	type,		/* service type, NULL for unspecified */
    const char *	key		/* unique service key                 */
    )
    {
    WTX_DESC *		pDesc = NULL;	/* copy of the retrned description    */
    WTX_ERROR_T		callStat;
    WTX_MSG_SVR_DESC	in;
    WTX_MSG_SVR_DESC *	pOut;

    if (hWtx->pSelfDesc != NULL)
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_SERVICE_ALREADY_REGISTERED, NULL);

    if (registryConnect (hWtx) != WTX_OK)
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_REGISTRY_UNREACHABLE, NULL);

    /* allocate a new descriptor */

    pOut = calloc (1, sizeof (WTX_MSG_SVR_DESC));
    if (pOut == NULL)
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_MEMALLOC, NULL);

    /* fill in our particulars */

    memset (&in, 0, sizeof (in));

    in.wtxSvrDesc.wpwrName = (char *) name;		/* send our name */
    in.wtxSvrDesc.wpwrType = (char *) type;		/* send our type */
    in.wtxSvrDesc.wpwrKey  = (char *) key;		/* send our key */

    callStat = exchange (hWtx, WTX_REGISTER, &in, pOut);

    if (callStat != WTX_ERR_NONE)
	{
	free (pOut);		/* free allocated message */
	WTX_ERROR_RETURN (hWtx, callStat, NULL);
	}

    /*
     * let's copy the returned structure field by field and add it to the
     * wtxResultFree List
     */

    if ((pDesc = wtxDescDuplicate ( hWtx, (WTX_DESC *) &pOut->wtxSvrDesc))
	 == NULL)
	{
	wtxExchangeFree (hWtx->registry, WTX_REGISTER, pOut);
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_MEMALLOC, NULL);
	}

    /* add pointer description to the wtxResultFree() list */

    if (wtxFreeAdd (hWtx, (void *) pDesc, (FUNCPTR) wtxDescFree,
		    NULL, 0, NULL, WTX_SVR_NONE)!= WTX_OK)
	{
	WTX_ERROR_RETURN (hWtx, wtxErrGet (hWtx), NULL);
	}

    /* close registry connection now since we don't need it anymore */

    wtxExchangeFree (hWtx->registry, WTX_REGISTER, pOut);
    free (pOut);
    registryDisconnect (hWtx);

    hWtx->pSelfDesc = pDesc;
    return (pDesc);
    }

/*******************************************************************************
*
* wtxDescFree - frees a WTX_DESC structure ans all its fields
*
* This routine frees the given WTX_DESC, and all the fields it contains.
*
* RETURN: N/A
*
* ERRNO: N/A
*
*/

LOCAL void wtxDescFree
    (
    WTX_DESC *	pDescToFree		/* WTX_DESC pointer to free */
    )
    {
    if (pDescToFree == NULL)
	goto error;

    if (pDescToFree->wpwrName != NULL)
	free (pDescToFree->wpwrName);
    if (pDescToFree->wpwrType != NULL)
	free (pDescToFree->wpwrType);
    if (pDescToFree->wpwrKey != NULL)
	free (pDescToFree->wpwrKey);

    free (pDescToFree);

error:
    return;
    }

/*******************************************************************************
*
* wtxUnregister - unregister the server with the Tornado registry
*
* This routine removes the specified descriptor from the Tornado-registry
* internal table. The name of the server is no longer reserved. Passing
* NULL for <name> means unregister the server already registered against
* the handle <hWtx> using wtxRegister().
*
* RETURNS: WTX_OK or WTX_ERROR if the server could not be unregistered.
*
* ERRORS:
* .iP WTX_ERR_API_SERVICE_NOT_REGISTERED 12
* The server is trying to unregister itself when it has not registered or
* has already unregistered.
* .iP WTX_ERR_API_REGISTRY_UNREACHABLE
* The registry service is not reachable (in other words, the registry 
* daemon is not running).
*
* SEE ALSO: WTX_UNREGISTER, wtxRegister()
*/

STATUS wtxUnregister 
    (
    HWTX	hWtx,		/* WTX API handle */
    const char *name		/* service to unregister, NULL for self */
    )
    {
    WTX_MSG_PARAM	in;
    WTX_MSG_RESULT	out;
    WTX_ERROR_T		callStat;
    
    if ((name == NULL) &&				/* self? */
        (hWtx->pSelfDesc == NULL || 
	 (name = hWtx->pSelfDesc->wpwrName) == NULL))

	/*  Unregistering self when not registered */

	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_SERVICE_NOT_REGISTERED, WTX_ERROR);

    if (registryConnect (hWtx) != WTX_OK)
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_REGISTRY_UNREACHABLE, WTX_ERROR);

    memset (&in, 0, sizeof (in));

    in.param.valueType = V_PCHAR;
    in.param.value_u.v_pchar = (char *) name;

    memset (&out, 0, sizeof (out));

    callStat = exchange (hWtx, WTX_UNREGISTER, &in, &out);

    /* close registry connection now since we don't need it anymore */

    registryDisconnect (hWtx);

    if (callStat != WTX_ERR_NONE)
	WTX_ERROR_RETURN (hWtx, callStat, WTX_ERROR);

    /* if unregistering ourself, destroy our descriptor */

    if (hWtx->pSelfDesc &&
	hWtx->pSelfDesc->wpwrName &&
	STREQ (hWtx->pSelfDesc->wpwrName, name))
	{
	wtxResultFree (hWtx, hWtx->pSelfDesc);
	hWtx->pSelfDesc = NULL;
	}

    wtxExchangeFree (hWtx->registry, WTX_UNREGISTER, &out);

    return (WTX_OK);
    }

/*******************************************************************************
*
* wtxInfo - allocate and return a descriptor for the named service
*
* This routine allocates and fills in a descriptor corresponding to the
* named service.  This routine always queries the WTX registry daemon for
* the named service and allocates and fills a WTX descriptor structure with
* the results.  The descriptor must be deallocated with wtxResultFree()
* when it is no longer needed. Passing NULL for <name> means to get
* information on the server already registered against the handle <hWtx>
* using wtxRegister().
*
* EXPAND ../../../include/wtxtypes.h WTX_DESC
*
* RETURNS: A pointer to a WTX_DESC or NULL if the service could not be found.
*
* ERRORS:
* .iP WTX_ERR_API_SERVICE_NOT_REGISTERED 12
* <name> is NULL and no service is registered against the handle.
* .iP WTX_ERR_API_REGISTRY_UNREACHABLE 
* The registry service is not reachable (in other words, the registry daemon
* is not running).
*
* SEE ALSO: WTX_INFO_GET, wtxInfoQ()
*/

WTX_DESC * wtxInfo 
    (
    HWTX	 hWtx,				/* WTX API handle          */
    const char * name				/* service name to lookup  */
    )
    {
    WTX_MSG_SVR_DESC *	pOut;			/* exchange returned value */
    WTX_MSG_PARAM	in;			/* input parameters desc   */
    WTX_ERROR_T		callStat;		/* exchange result values  */

    if ((name == NULL) &&				/* self? */
        (hWtx->pSelfDesc == NULL || 
	 (name = hWtx->pSelfDesc->wpwrName) == NULL))
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_SERVICE_NOT_REGISTERED, NULL);

    if (registryConnect (hWtx) != WTX_OK)
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_REGISTRY_UNREACHABLE, NULL);

    /* allocate and initialize registry response */

    pOut = (WTX_MSG_SVR_DESC *) calloc (1, sizeof (WTX_MSG_SVR_DESC));
    if (pOut == NULL)
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_MEMALLOC, NULL);

    memset (&in, 0, sizeof (in));

    in.param.valueType = V_PCHAR;
    in.param.value_u.v_pchar = (char *) name;

    /* ask daemon for information */

    callStat = exchange (hWtx, WTX_INFO_GET, &in, pOut);

    if (callStat != WTX_ERR_NONE)
	{
	free (pOut);				/* Free allocated message */
	WTX_ERROR_RETURN (hWtx, callStat, NULL);
	}

    /* add pointer to the wtxResultFree() list */

    if (wtxFreeAdd (hWtx, (void *) &pOut->wtxSvrDesc, NULL, pOut, WTX_INFO_GET,
		    hWtx->registry, WTX_SVR_REGISTRY) != WTX_OK)
	{
	WTX_ERROR_RETURN (hWtx, wtxErrGet (hWtx), NULL);
	}

    return ( (WTX_DESC *) &pOut->wtxSvrDesc);
    }

/*******************************************************************************
*
* wtxDescDuplicate - duplicates a WTX_DESC structure
*
* This routine just duplicates a WTX_DESC structure and all the fields it
* contains.
*
* RETURNS: a pointer on the duplicated WTX_DESC
*
* ERRNO:
* .iP
* WTX_ERR_API_MEMALLOC
* .lE
*/

LOCAL WTX_DESC * wtxDescDuplicate
    (
    HWTX	hWtx,			/* current WTX session handler */
    WTX_DESC *	pDesc			/* WTX_DESC to duplicate       */
    )
    {
    WTX_DESC *	pDuplicate = NULL;	/* duplicated WTX_DESC pointer */

    /* allocate the WTX_DESC itself */

    if ( (pDuplicate = (WTX_DESC *) calloc (1, sizeof (WTX_DESC))) == NULL)
	{
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_MEMALLOC, NULL);
	}

    /* item name memory allocation */

    if ((pDesc->wpwrName != NULL) &&
	((pDuplicate->wpwrName = strdup (pDesc->wpwrName)) == NULL))
	{
	free (pDuplicate);
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_MEMALLOC, NULL);
	}

    /* item type memory allocation */

    if ((pDesc->wpwrType != NULL) &&
	((pDuplicate->wpwrType = strdup (pDesc->wpwrType)) == NULL))
	{
	free (pDuplicate->wpwrName);
	free (pDuplicate);
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_MEMALLOC, NULL);
	}

    /* item key memory allocation */

    if ((pDesc->wpwrKey != NULL) &&
	((pDuplicate->wpwrKey = strdup (pDesc->wpwrKey)) == NULL))
	{
	free (pDuplicate->wpwrType);
	free (pDuplicate->wpwrName);
	free (pDuplicate);
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_MEMALLOC, NULL);
	}

    return (pDuplicate);
    }

/*******************************************************************************
*
* wtxInfoQ - return a list of all services registered with the Tornado registry
*
* This routine provides the caller with a table of descriptors corresponding 
* to all services registered with the Tornado registry.  An unsorted table
* of descriptors is allocated and returned.  There is no guarantee that 
* the table is valid by the time it is inspected.  Services may come and go
* at any time.
*
* EXPAND ../../../include/wtxtypes.h WTX_DESC_Q
*
* The table of descriptors should be deallocated with wtxResultFree().
*
* RETURNS: A pointer to a table of descriptors or NULL if error.
*
* ERRORS:
* .iP WTX_ERR_API_REGISTRY_UNREACHABLE 12
* The registry service is not reachable (in other words, the registry
* daemon is not running).
* .iP WTX_ERR_API_MEMALLOC 
* There are insufficient resources to allocate the WTX_DESC_Q structure for
* the result value.
* .iP WTX_ERR_REGISTRY_BAD_PATTERN
* One of the given patterns (<namePattern>, <typePattern> or <keyPattern>) is
* not a valid regular expression.
* .iP WTX_ERR_REGISTRY_NAME_NOT_FOUND
* Info required is not in the registry database.
*
* SEE ALSO: WTX_INFO_Q_GET, wtxInfo()
*/

WTX_DESC_Q * wtxInfoQ
    (
    HWTX		hWtx,			/* WTX API handle           */
    const char *	namePattern,		/* regexp to match svc name */
    const char *	typePattern,		/* regexp to match svc type */
    const char *	keyPattern		/* regexp to match svc key  */
    )
    {
    WTX_MSG_WTXREGD_PATTERN	in;		/* input pattern desc       */
    WTX_MSG_SVR_DESC_Q *	pOut = NULL;	/* output message           */
    WTX_ERROR_T			callStat;	/* exchange result values   */

    if (registryConnect (hWtx) != WTX_OK)
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_REGISTRY_UNREACHABLE, NULL);

    /* allocate and initialize registry response */

    pOut = calloc (1, sizeof (WTX_MSG_SVR_DESC_Q));
    if (pOut == NULL)
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_MEMALLOC, NULL);

    /* fill in patterns for query */

    in.name = (char *) namePattern;
    in.type = (char *) typePattern;
    in.key  = (char *) keyPattern;

    /* get list of registered services */

    callStat = exchange (hWtx, WTX_INFO_Q_GET, &in, pOut);

    if (callStat != WTX_ERR_NONE)
	{
	free (pOut);
	WTX_ERROR_RETURN (hWtx, callStat, NULL);
	}

    if (wtxFreeAdd (hWtx, (void *) &pOut->wtxSvrDescQ, NULL, pOut,
		    WTX_INFO_Q_GET, hWtx->registry, WTX_SVR_REGISTRY) != WTX_OK)
	{
	WTX_ERROR_RETURN (hWtx, wtxErrGet (hWtx), NULL);
	}

    return ( (WTX_DESC_Q *) &pOut->wtxSvrDescQ);
    }

/*******************************************************************************
*
* wtxEach - call a routine to examine each WTX-registered service
*
* This routine calls a user-supplied routine to examine each WTX-registered
* service; it calls the specified routine once for each service.  The
* routine should be declared as follows:
* 
* .CS
*   BOOL32 routine
*	(
*	WTX_DESC_Q *	pDesc,	/@ entry name @/
*	int          	arg	/@ arbitrary user-supplied arg @/
*	)
* .CE
* 
* The user-supplied routine should return TRUE if wtxEach() is
* to  continue  calling  it  for each entry, or FALSE if it is
* done and wtxEach() can exit.  
*
* Note that all descriptors are deallocated upon exit.
*
* RETURNS: WTX_OK or WTX_ERROR if the table of descriptors could not be 
* obtained.
* 
* ERRORS:
* .iP WTX_ERR_API_REGISTRY_UNREACHABLE 12 
* The registry service is not reachable (in other words, the registry daemon
* is not running).
* .iP WTX_ERR_API_MEMALLOC
* There are insufficient resources to allocate the WTX_DESC_Q structure for
* the result value.
*
* SEE ALSO: wtxInfoQ()
*/

STATUS wtxEach
    (
    HWTX	hWtx,		/* WTX API handle */
    const char *namePattern,	/* regular expression to match svc name */
    const char *typePattern,	/* regular expression to match svc type */
    const char *keyPattern,	/* regular expression to match svc key */
    FUNCPTR	routine,	/* func to call for each svc entry */
    void *	routineArg	/* arbitrary user-supplied arg */
    )
    {
    WTX_DESC_Q *	pWtxDescQ;
    WTX_DESC_Q *	pDescQ;

    pWtxDescQ = wtxInfoQ (hWtx, namePattern, typePattern, keyPattern);

    if (pWtxDescQ == NULL)
	/* Note: error has already been dispatched so just return */
	return  WTX_ERROR;

    pDescQ = pWtxDescQ;

    /* traverse descriptor queue and call the routine */

    while (pDescQ != NULL)
	{
	if (!((* routine) (pDescQ, routineArg)))
	    break;

	pDescQ = (WTX_DESC_Q *) pWtxDescQ->pNext;
	}

    wtxResultFree (hWtx, pWtxDescQ);

    return (WTX_OK);
    }

/*******************************************************************************
*
* wtxTimeoutSet - set the timeout for completion of WTX calls
*
* This routine changes the timeout value for making WTX API calls.
* When a call takes longer than the value specified by <msec> (in
* milliseconds), the call aborts with an error status and the
* error code is set to WTX_ERR_API_REQUEST_TIMED_OUT.
*
* NOTE: The timeout for WTX registry calls is unaffected.
*
* RETURNS: WTX_OK or WTX_ERROR.
*
* SEE ALSO: wtxTimeoutGet()
*/
STATUS	wtxTimeoutSet 
    (
    HWTX	hWtx,	/* WTX API handle */
    UINT32	msec	/* New timeout value in milliseconds */
    )
    {
    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    if (wtxExchangeControl (hWtx->server, 
			    WTX_EXCHANGE_CTL_TIMEOUT_SET, &msec) == WTX_OK)
	return WTX_OK;
    else
	WTX_ERROR_RETURN (hWtx, wtxExchangeErrGet (hWtx->server), WTX_ERROR);
    }

/*******************************************************************************
*
* wtxTimeoutGet - get the current timeout for completion of WTX calls
*
* This routine gets the current timeout value for making WTX API calls.
* The value is returned in the UINT32 pointed to by <pMsec>.
*
* RETURNS: WTX_OK or WTX_ERROR.
*
* SEE ALSO: wtxTimeoutSet()
*/
STATUS	wtxTimeoutGet 
    (
    HWTX	hWtx,	/* WTX API handle */
    UINT32 *	pMsec	/* Pointer to timeout value in milliseconds */
    )
    {
    WTX_CHECK_CONNECTED (hWtx, WTX_ERROR);

    if (pMsec == NULL)
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_INVALID_ARG, WTX_ERROR);

    if (wtxExchangeControl (hWtx->server, 
			    WTX_EXCHANGE_CTL_TIMEOUT_GET, pMsec) == WTX_OK)
	return WTX_OK;
    else
	WTX_ERROR_RETURN (hWtx, wtxExchangeErrGet (hWtx->server), WTX_ERROR);
    }

/*******************************************************************************
*
* exchange - wrapper for calling exchange mechanism
*
* This routine performs a WTX call selecting the correct exchange handle
* and doing some additional error checking.
*
* RETURNS: WTX_ERR_NONE or error code
*
* NOMANUAL
*/

LOCAL WTX_ERROR_T exchange
    (
    HWTX	hWtx,		/* handle to use for exchange */
    UINT32	svcNum,		/* service number to call */
    void *	pIn,		/* pointer to in (argument) data */
    void *	pOut		/* pointer to out (result) data */
    )
    {
    WTX_ERROR_T	retVal;		/* value to return */
    STATUS	status;		/* status of exchange call */
    WTX_XID 	xid;		/* exchange handle to use for call */

    /* Figure out which exchange handle to use based on the API service num */

    switch (svcNum)
	{
	case WTX_REGISTER:
	case WTX_UNREGISTER:
	case WTX_INFO_GET:
	case WTX_INFO_Q_GET:
	    xid = hWtx->registry;
	    break;

	default:
	    xid = hWtx->server;
	    break;
	}

    /* Fill in the objId field required by some API requests */

    ((WTX_MSG_DUMMY *) pIn)->wtxCore.objId = hWtx->msgToolId.wtxCore.objId;

    /* Do the call */

    status = wtxExchange (xid, svcNum, pIn, pOut);

    /*
     * Stamp the result with the service number so that it can be freed
     * by the free function without knowing its object type. The protVersion
     * field is used to store the address of the result to help identify
     * (not foolproof but fairly good) bogus pointers passed to free.
     */

    if (status == WTX_OK)
	{
	((WTX_MSG_DUMMY *) pOut)->wtxCore.objId = svcNum;
	((WTX_MSG_DUMMY *) pOut)->wtxCore.protVersion = (UINT32) pOut;
	return WTX_ERR_NONE;
	}

    /* Analyze the result and see if it requires any special action */

    switch (wtxExchangeErrGet (xid))
	{
	case WTX_ERR_EXCHANGE_TRANSPORT_DISCONNECT:
	    /*
	     * This exchange transport mechanism has broken down and
	     * disconnected us. The error is reported to the caller as
	     * WTX_ERR_API_TOOL_DISCONECTED or
	     * WTX_ERR_API_REGISTRY_UNREACHABLE (depending on the
	     * connection) so the user can tell there is no point in
	     * making further calls (which would return 
	     * WTX_ERR_API_NOT_CONNECTED 
	     */

	    if (xid == hWtx->registry)
		{
	        wtxExchangeDelete (xid);
	        wtxExchangeTerminate (xid);
		hWtx->registry = NULL;
		retVal = WTX_ERR_API_REGISTRY_UNREACHABLE;
		}
	    else
		{
    		toolCleanup (hWtx);
		retVal = WTX_ERR_API_TOOL_DISCONNECTED;
		}
	    break;

	case WTX_ERR_EXCHANGE_TIMEOUT:
	    retVal = WTX_ERR_API_REQUEST_TIMED_OUT;
	    break;

	case WTX_ERR_EXCHANGE:
	case WTX_ERR_EXCHANGE_INVALID_HANDLE:
	case WTX_ERR_EXCHANGE_DATA:
	case WTX_ERR_EXCHANGE_MEMALLOC:
	case WTX_ERR_EXCHANGE_NO_SERVER:
	case WTX_ERR_EXCHANGE_INVALID_ARG:
	case WTX_ERR_EXCHANGE_MARSHALPTR:
	case WTX_ERR_EXCHANGE_BAD_KEY:
	case WTX_ERR_EXCHANGE_REQUEST_UNSUPPORTED:
	case WTX_ERR_EXCHANGE_TRANSPORT_UNSUPPORTED:
	case WTX_ERR_EXCHANGE_TRANSPORT_ERROR:
	case WTX_ERR_EXCHANGE_NO_TRANSPORT:
	    retVal = WTX_ERR_API_REQUEST_FAILED;
	    break;

	default:
	    /* some other API error - return error unchanged */

	    retVal = wtxExchangeErrGet (xid);
	    break;
	}

    return retVal;
    }

/*******************************************************************************
*
* registryConnect - wrapper for setting up registry exchange
*
* This routine tries to set up the exchange handle for the registry
* and connect to the registry service.
*
* RETURNS: WTX_ERR_NONE or error code
*
* NOMANUAL
*/

LOCAL WTX_ERROR_T registryConnect
    (
    HWTX	hWtx			/* handle to connect to registry */
    )
    {
    BOOL	usePMap = FALSE;	/* use portmapper to connect ? */

    WTX_CHECK_HANDLE (hWtx, WTX_ERR_API_INVALID_HANDLE);

    if ((WTX_XID) hWtx->registry != NULL)
	return WTX_OK;

    /* first initialise the connection */

    if (wtxExchangeInitialize ((WTX_XID *) &hWtx->registry) != WTX_OK ||
	wtxExchangeInstall ((WTX_XID) hWtx->registry, 
			    wtxRpcExchangeCreate, 
			    wtxRpcExchangeDelete, 
			    wtxRpcExchange, 
			    wtxRpcExchangeFree, 
			    wtxRpcExchangeControl) != WTX_OK)
	{
	wtxExchangeTerminate ((WTX_XID) hWtx->registry);
	hWtx->registry = (WTX_XID) NULL;
	return WTX_ERROR;
	}

    /* now create th connection */
use_pmap:

    if (wtxExchangeCreate ((WTX_XID) hWtx->registry, NULL, usePMap) != WTX_OK)
	{
	if (! usePMap)
	    {
	    /* well, let's try with the port mapper ... */

	    usePMap = TRUE;
	    goto use_pmap;
	    }

	wtxExchangeTerminate ((WTX_XID) hWtx->registry);
	hWtx->registry = (WTX_XID) NULL;
	return WTX_ERROR;
	}

    return WTX_OK;
    }

/*******************************************************************************
*
* registryDisconnect - wrapper for closing registry exchange
*
* This routine terminates the exchange handle for the registry
* and clears the registry field in the WTX handle.
*
* RETURNS: WTX_ERR_NONE or error code
*
* NOMANUAL
*/

LOCAL void registryDisconnect
    (
    HWTX	hWtx	/* handle to disconnect registry */
    )
    {
    if (hWtx->registry == NULL)		/* check if not already disconnected */
	return; 

    wtxExchangeDelete (hWtx->registry);		/* close registry connection */
    wtxExchangeTerminate (hWtx->registry);	/* cleanup */
    hWtx->registry = NULL;			/* clear handle */

    return;
    }

/*******************************************************************************
*
* toolCleanup - do the cleanups necessary when a tool detaches
*
* RETURNS: N/A
*
* NOMANUAL
*/

LOCAL void toolCleanup
    (
    HWTX	hWtx  /* handle to connect to server */
    )
    {
    if (hWtx == NULL)
	return;

    /* Free up the target server info before deleting the exchange handle */
    if (hWtx->pTsInfo)
	{
	wtxResultFree (hWtx, hWtx->pTsInfo);
	hWtx->pTsInfo = NULL;
	}

    /* Free Target tool name */
    if (hWtx->targetToolName != NULL)
	{
	free (hWtx->targetToolName);
	hWtx->targetToolName = 0;
	}

    /* Free up the tool descriptor */
    if (hWtx->pToolDesc)
	{
	wtxResultFree (hWtx, hWtx->pToolDesc);
	hWtx->pToolDesc = NULL;
	}

    /*
     * as we exit the WTX session, all the pointers from the wtxResultFree()
     * pointers list should be freed.
     */

    if (hWtx->pWtxFreeList != NULL)
	{
	if (sllCount (hWtx->pWtxFreeList) > 0)
	    {
	    sllEach (hWtx->pWtxFreeList, wtxResultFreeServerListTerminate,
		     (int) &hWtx);
	    }
	/* don't delete the free list, it has to be done in wtxTerminate() */
	}

    if (hWtx->server != NULL)
	{
	wtxExchangeDelete (hWtx->server);
	wtxExchangeTerminate (hWtx->server); 
	}

    hWtx->server = NULL;
    }

/*******************************************************************************
*
* serverDescFree - de-allocate server descriptor
*
* RETURNS: N/A
*
* NOMANUAL
*/

LOCAL void serverDescFree
    (
    HWTX	hWtx  /* handle to connect to server */
    )
    {
    if ((hWtx != NULL) && (hWtx->pServerDesc != NULL))
	{
	free (hWtx->pServerDesc->wpwrName);
	free (hWtx->pServerDesc->wpwrKey);
	free (hWtx->pServerDesc->wpwrType);

	free (hWtx->pServerDesc);
	hWtx->pServerDesc = NULL;
	}
    }

#ifndef HOST
/*******************************************************************************
*
* stringDup - duplicates a sting
*
* This routine just duplicates a string, an strdup() call can not be made as far
* as the target has no strdup(), and wtx.c should be compiled on both host and
* target sides.
*
* RETURNS: A char pointer to the allocated/ duplicated string
*
* ERRORS:
* .iP
* WTX_ERR_API_MEM_ALLOC
* .LE
*
*/

LOCAL char * stringDup
    (
    const char *	strToDup		/* string to duplicate */
    )
    {
    char *		duplicatedStr = NULL;	/* duplicated string   */

    if (strToDup == NULL)
	goto error;

    if ( (duplicatedStr = (char *) calloc (strlen (strToDup) + 1,
					   sizeof (char))) == NULL)
	{
	goto error;
	}

    if ( strcpy (duplicatedStr, strToDup) == NULL)
	{
	free (duplicatedStr);
	duplicatedStr = NULL;
	}

error:
    return (duplicatedStr);
    }
#endif /* ! HOST */

/******************************************************************************
*
* wtxToolIdGet - return the tool identifier of the current tool
*
* This function returns the tool identifier associated with the given
* handle <hWtx>.
*
* RETURNS: The tool identifier or WTX_ERROR on error.
*/

UINT32 wtxToolIdGet
    (
    HWTX	hWtx		/* WTX API handle */
    )
    {

    WTX_CHECK_CONNECTED (hWtx, 0);

    if (hWtx->pToolDesc != NULL)
	return (hWtx->pToolDesc->id);
    else
	return (WTX_ERROR);
    }

/*******************************************************************************
*
* wtxMemDisassemble - get disassembled instructions matching the given address
*
* This routine gets a list of disassembled instructions matching given address.
* <startAddr> indicates where to begin disassembly, while <nInst> fixes the
* number of instructions to disassemble.
*
* If <endAddr> is set, <nInst> has no matter, and only instructions between
* <startAddr> and <endAddr> are returned.
*
* Formatting options are <printAddr> to print instruction's address, 
* <printOpcodes> to print instruction's opcodes, and <hexAddr> to turn off the
* "symbolic + offset" address representation.
*
* The returned structure is a WTX_DASM_INST_LIST described below :
*
* EXPAND ../../../include/wtxmsg.h WTX_DASM_INST_LIST
*
* Each disassembled instruction has the following format :
*
* .CS
*   {<symbolName>} <size> {<address>} {<opcodes>} {<disassembledInstruction>}
* .CE	
*
* The <symbolname> value is set to the symbol name corresponding to the current
* address, or to {} if no symbol name matches current instruction address.
*
* <size> represents the instruction size in bytes of memory
*
* <address> field is set only if `prependAddress' is set to TRUE. Else an empty
* list member is returned : {}
*
* <opcodes> of disassembled instructions are returned if the `prependOpcode',
* else an empty list member is returned : {}.
*
* The <disassembledInstruction> member is also quoted into curled braces to
* be easier to reach. The disassembled instruction contains the instruction
* mnemonic and adress, registers, ... as displayed by the board disassembler.
*
* NOTE :
* All the fields are always returned, even if empty, so a disassembled
* instruction always have the same number of members whatever are the options.
*
* NOTE : The instruction list allocated by wtxMemDisassemble should be freed by
* caller with wtxResultFree().
*
* RETURNS: A pointer to a list of disassembled instructions or NULL on error.
*
* EXAMPLES:
*
* Getting disassembled instuctions from address 12345 to address 15432, with
* instruction's address and instruction's opcode prepended :
*
* .CS
*   startAddress = 12345;	/@ first instruction's address         @/
*   endAddress = 15432;		/@ last instruction's address          @/
*   nInst = 0;			/@ unhandled when endAddress is set    @/
*   prependAddress = TRUE;	/@ instruction's address prepended     @/
*   prependOpcode = TRUE;	/@ opcodes joined to instruction       @/
*   hexAddr = FALSE;		/@ "symbolic + offset" mode turned on  @/
*
*   pWtxDasmInstList = wtxMemDisassemble (hWtx, startAddress, nInst, endAddress,
*                                         prependAddress, prependOpcode,
*                                         hexAddr);
*   ...
*
*   wtxResultFree (hWtx, pWtxDasmInstList);
* .CE
*
* Getting 25 instructions, starting from address 54321. Address in disassembled
* instruction are only hexadecimal (not represented by a symbol name + offset).
* No instruction's addresses, no opcodes.
*
* .CS
*   startAddress = 54321;	/@ first instruction's address         @/
*   endAddress = 0;		/@ get a known number of instructions  @/
*   nInst = 25;			/@ get 25 disassembled instructions    @/
*   prependAddress = FALSE;	/@ don't join addresses                @/
*   prependOpcode = FALSE;	/@ don't join opcodes                  @/
*   hexAddr = TRUE;		/@ "symbolic + offset" mode turned off @/
*
*   pWtxDasmInstList = wtxMemDisassemble (hWtx, startAddress, nInst, endAddress,
*                                         prependAddress, prependOpcode,
*                                         hexAddr);
*   ...
*
*   wtxResultFree (hWtx, pWtxDasmInstList);
* .CE
*
* SEE ALSO: WTX_MEM_DISASSEMBLE, wtxSymListGet(), wtxResultFree().
*/

WTX_DASM_INST_LIST * wtxMemDisassemble 
    (
    HWTX	hWtx,			/* WTX API handle                     */
    TGT_ADDR_T	startAddr,		/* Inst address to match              */
    UINT32	nInst,			/* number of instructions to get      */
    TGT_ADDR_T	endAddr,		/* Last address to match              */
    BOOL32	printAddr,		/* if instruction's address appended  */
    BOOL32	printOpcodes,		/* if instruction's opcodes appended  */
    BOOL32	hexAddr			/* for hex  adress representation     */
    )
    {
    WTX_MSG_DASM_DESC		in;		/* disassembling parameters   */
    WTX_MSG_DASM_INST_LIST * 	pOut;		/* disassembled message       */
    WTX_MSG_DASM_INST_LIST	out;		/* local disassembled message */
    WTX_ERROR_T			callStat;	/* error message              */
    UINT32			listSize = 0;	/* list size in bytes         */
    char *			instList = NULL;/* list of instructions       */
    BOOL32			firstExchange;	/* first exchange indicator   */

    /* check for connection */

    WTX_CHECK_CONNECTED (hWtx, NULL);

    pOut = (WTX_MSG_DASM_INST_LIST *)calloc (1, sizeof(WTX_MSG_DASM_INST_LIST));
    if (pOut == NULL)
	WTX_ERROR_RETURN (hWtx, WTX_ERR_API_MEMALLOC, NULL);

    /* initialize disassembling parameters */

    memset (&in, 0, sizeof (in));

    in.wtxCore.objId = hWtx->msgToolId.wtxCore.objId;

    in.startAddr = startAddr;
    in.endAddr = endAddr;

    if ((endAddr == 0) && (nInst == 0))
	in.nInst = N_DASM_INST_DEFAULT;
    else
	in.nInst = nInst;

    in.printAddr = printAddr;
    in.printOpcodes = printOpcodes;
    in.hexAddr = hexAddr;
    in.giveMeNext = FALSE;

    firstExchange = TRUE;

    do
	{
	memset (&out, 0, sizeof (out));

	/* call for target server routines */

	callStat = exchange (hWtx, WTX_MEM_DISASSEMBLE, &in, &out);

	if (callStat != WTX_ERR_NONE)
	    {
	    if (instList != NULL)
		free (instList);

            free (pOut);
	    WTX_ERROR_RETURN (hWtx, callStat, NULL);
	    }

        /* copy first part of message */
       
        if (firstExchange)
	    {
	    listSize = out.instList.listSize;
	    instList = (char *) calloc (listSize + 1, sizeof (char));
            firstExchange = FALSE;
            }

        strcat (instList, out.instList.pInst);

        /* ensure next message is to come */

	in.giveMeNext = out.moreToCome;

	wtxExchangeFree (hWtx->server, WTX_MEM_DISASSEMBLE, &out);
	} while (in.giveMeNext);

    /* Set up pOut so that it can be freed by wtxResultFree() */

    pOut->wtxCore.protVersion = (UINT32) pOut;
    pOut->wtxCore.objId = WTX_MEM_DISASSEMBLE;
 
    /* set the disassembled instructions data */

    pOut->instList.pInst = (char *)calloc (listSize + 1, sizeof (char));
    strcpy (pOut->instList.pInst, instList);
    pOut->instList.nextInst = out.instList.nextInst;
    pOut->instList.listSize = out.instList.listSize;

    free (instList);

    /* add pointer to the wtxResultFree() list */

    if (wtxFreeAdd (hWtx, (void *) &pOut->instList,
		    (FUNCPTR) wtxMemDisassembleFree, pOut, 0, NULL,
		    WTX_SVR_NONE) != WTX_OK)
	{
	WTX_ERROR_RETURN (hWtx, wtxErrGet (hWtx), NULL);
	}

    return (&pOut->instList);
    }

/*******************************************************************************
*
* wtxMemDisassembleFree - frees memory allocated by wtxMemDisassemble()
*
* As wtxMemDisasssmble() routine is not stateless, it allocates memory, and the
* RPC buffers are not holding the disassembled lists. This routine frees the
* memory allocated by the (sucessive ?) cals to wtxMemDisassemble without
* calling to the RPC stuff.
*
* RETURNS: WTX_OK or WTX_ERROR if free operation failed
*
* NOMANUAL
*/

LOCAL STATUS wtxMemDisassembleFree
    (
    WTX_DASM_INST_LIST *	pDasmInstList	/* disassembled list to free */
    )
    {
    if (pDasmInstList->pInst != NULL)
	 free (pDasmInstList->pInst);

    return (WTX_OK);
    }

#ifdef HOST

/*******************************************************************************
*
* wtxAsyncEventGet - get an event from the asynchronous event list
*
* When the asynchronous event notification is started without user defined
* function, the only way to get events is to use wtxAsyncEventGet.
* This routine polls the asynchronous event list for any events sent
* to this client tool. It only returns one event at a time.  The event
* descriptor returned is a string containing the primary event data,
* a length count of additional binary data and a pointer to the
* additional data. The memory allocated for the event-descriptor
* result must be freed by the user using wtxAsyncResultFree().
*
* NOTE: If the asynchronous event notification is started with a user defined
* function, then wtxAsyncEventGet() will always return a NULL pointer.
*
* EXPAND ../../../include/wtxmsg.h WTX_EVENT_DESC
*
* RETURN: A pointer to the descriptor for the event received or NULL.
*
* SEE ALSO: wtxAsyncNotifyEnable(), wtxAsyncNotifyDisable(), wtxAsyncResultFree(), wtxEventGet()
*/

WTX_EVENT_DESC * wtxAsyncEventGet
    (
    HWTX		hWtx	/* WTX API handle */
    )
    {
    WTX_EVENT_DESC *	pEventList = NULL;

    /* check for connection */
    WTX_CHECK_CONNECTED (hWtx, NULL);

    pEventList = (wtxAsyncEventDescGet(hWtx->asyncHandle));

    /* XXX - pai:
     * Note that if this mod is accepted, it changes the API
     * and breaks code which uses this routine as documented
     * in the comment header.  Expect runtime errors if the
     * following mod is compiled in.
     *
     * /@ add pointer to the wtxResultFree() list @/
     *
     * if (pEventList != NULL)
     *     {
     *     if (wtxFreeAdd (hWtx, (void *) pEventList,
     *                    (FUNCPTR) wtxAsyncResultFree_2, NULL, 0, NULL,
     *                    WTX_SVR_NONE) != WTX_OK)
     *         {
     *         WTX_ERROR_RETURN (hWtx, wtxErrGet (hWtx), NULL);
     *         }
     *     }
     */

    return (pEventList);
    }

/*******************************************************************************
*
* wtxAsyncResultFree - free memory used by a wtxAsyncEventGet() call result
*
* This routine frees the memory allocated for the result of a
* wtxAsyncEventGet() call. eventDesc must point to the WTX_EVENT_DESC
* structure that was returned by wtxAsyncEventGet() call.
*
* NOTE: result should be freed calling wtxResultFree() routine
*
* RETURNS: WTX_OK
*
* SEE ALSO: wtxAsyncEventGet(), wtxAsyncNotifyEnable(), wtxAsyncNotifyDisable()
*/
 
STATUS wtxAsyncResultFree
    (
    HWTX		hWtx,
    WTX_EVENT_DESC *	eventDesc	/* pointer to structure to free */
    )
    {
    /* if eventDesc is NULL: nothing to free */
    if (eventDesc != NULL)
	{
	/* is there additional data to free */
	if (eventDesc->addlDataLen != 0)
	    free (eventDesc->addlData);

	free (eventDesc->event);
	free (eventDesc);
	}

    return (WTX_OK);
    }

#if 0
/* XXX : fle : for SRP#67326 */
/*******************************************************************************
*
* wtxAsyncResultFree_2 - free memory used by a wtxAsyncEventGet() call result
*
* This routine is the routine that should be used by wtxResultFree(). It is
* different from the wtxAsyncResultFree which was also using an hWtx WTX
* handler.
*
* This routine frees the memory allocated for the result of a
* wtxAsyncEventGet() call. eventDesc must point to the WTX_EVENT_DESC
* structure that was returned by wtxAsyncEventGet() call.
*
* RETURNS: WTX_OK
*
* SEE ALSO: wtxAsyncEventGet(), wtxAsyncNotifyEnable(), wtxAsyncNotifyDisable()
* wtxResultFree()
*
* NOMANUAL
*/
 
LOCAL STATUS wtxAsyncResultFree_2
    (
    WTX_EVENT_DESC *	eventDesc	/* pointer to structure to free */
    )
    {
    /* if eventDesc is NULL: nothing to free */
    if (eventDesc != NULL)
	{
	/* is there additional data to free */
	if (eventDesc->addlDataLen != 0)
	    free (eventDesc->addlData);

	free (eventDesc->event);
	free (eventDesc);
	}

    return (WTX_OK);
    }

#endif /* 0 */
#endif /* HOST */


#ifdef HOST
/*******************************************************************************
*
* wtxDefaultDemanglerStyle - appropriate demangling style for target tool
*
* This routine returns an appropriate demangling style based on the current 
* target tool. For example if the kernel was built with the Diab toolchain,
* the routine returns DMGL_STYLE_DIAB. If the target is not connected,
* or the tool is not recognized, the routine defaults to returning the GNU 
* demangling style (DMGL_STYLE_GNU).
*
* Typically this routine will be used in conjunction with the demangle
* function:
*
* .CS
*   dmgl = demangle(symbolName, wtxDefaultDemanglerStyle(hWtx), demangleMode);
* .CE
* 
* RETURNS: a DEMANGLER_STYLE
*
* ERRORS: N/A
*
* INCLUDE FILES: demangle.h
*/

DEMANGLER_STYLE wtxDefaultDemanglerStyle
    (
    HWTX hWtx
    )
    {
    const char * tool = wtxTargetToolFamilyGet(hWtx);
    if (tool)
      {
	if (strcmp(tool, "diab") == 0)
	  return DMGL_STYLE_DIAB;
	/* add additional tools here */
      }
    /* default to GNU */
    return DMGL_STYLE_GNU;
    }

/*******************************************************************************
*
* wtxTargetToolFamilyGet - extract the tool family (e.g. "gnu" or "diab")
*
* RETURNS: a string
*
* ERRORS: N/A
*
*/

const char * wtxTargetToolFamilyGet
    (
    HWTX hWtx                           /* WTX API handle */
    )
    {
    static const char * toolFamilies[] = 
        {
	"gnu",
	"diab"
	};
    int i;        /* index into toolFamilies */
    const char * result = "gnu";
    const char * tool = wtxTargetToolNameGet(hWtx);
    /* fixme; we should query the target for TOOL_FAMILY
     * rather than relying on the tool being "gnu" or "diab" */
    for (i = 0; i != sizeof(toolFamilies) / sizeof(const char *); ++i)
        {
	  if (strstr(tool, toolFamilies[i]))
	      {
              result = toolFamilies[i];
	      break;
	      }
        }
    return result;
}
#endif /* HOST */

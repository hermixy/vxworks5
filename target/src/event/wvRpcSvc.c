/* wvServer.c - WindView RPC server */

/* Copyright 1994 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01f,09aug96,pr   moved object instrumentation-on back here (SPR #6998).
01e,07aug96,pr   moved object instrumentation to wvLib.c (SPR #6998).
01d,31mar94,smb  event logging is turned on after objects are inst.
01c,25mar94,smb  modified OBJECT_STATUS event logging from the gui.
01c,22feb94,smb  corrected Copyright date (SPR #2910)
01b,21jan94,maf  call to wvEvtLog() replaced with wvEvtLog{Enable,Disable}().
	   +c_s
01a,06dec93,c_s  written, based on output of rpcgen.  It differs from that 
		   output in these ways: (1) the entry point is vwSvc (),
		   rather than main; (2) results are freed; (3) the errno
		   is cleared after the portmap setup; (4) the code is 
		   in WRS style; (5) includes differ; (6) service routine
		   for EVT_LOG_CONTROL is implemented inline so that the
		   transport handle is available for computing the calling
		   host's IP address.
*/

/* includes */

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "errnoLib.h"
#include "rpc/rpc.h"
#include "rpc/pmap_clnt.h"
#include "rpcLib.h"
#include "arpa/inet.h"
#include "inetLib.h"
#include "wvRpc.h"
#include "wvLib.h"


/* forwards */

static void windview_1 ();

/*******************************************************************************
*
* wvSvc - RPC server entry point
*
* This task is spawned by wvServerInit to attach the WindView command service
* to the portmap and install the callback routines.  After this is done 
* the routine will enter svc_run (), never to return under normal 
* circumstances.
*
* SEE ALSO:
* NOMANUAL
*/

void wvSvc (void)

    {
    register SVCXPRT *transp;

    rpcTaskInit ();

    (void) pmap_unset (WINDVIEW, WINDVIEW_VERS_CURRENT);

    transp = svcudp_create (RPC_ANYSOCK);
    if (transp == NULL)
        {
	fprintf (stderr, "cannot create udp service.");
	exit (1);
        }
    if (!svc_register (transp, WINDVIEW, WINDVIEW_VERS_CURRENT, 
		       windview_1, IPPROTO_UDP))
        {
	fprintf (stderr, 
		 "unable to register (WINDVIEW, WINDVIEW_VERS_CURRENT, udp).");
	exit (1);
        }

    /* pmap_unset is expected to fail, leaving an errno in the task context.
       Since the error is unimportant we clear it here. */

    errnoSet (0);

    svc_run ();
    fprintf (stderr, "svc_run returned");
    exit (1);
    /* NOTREACHED */
    }

/*******************************************************************************
*
* windview_1 - RPC dispatch routine for version 1 of the WindView cmd protocol
*
* This routine is invoked by the RPC system when dispatching a request
* for the WindView command service.
*
* SEE ALSO:
* NOMANUAL
*/

static void windview_1
    (
    struct svc_req *rqstp,
    register SVCXPRT *transp
    )

    {
    union
        {
	char *wvproc_symtab_lookup_1_arg;
	taskSpawnRec wvproc_task_spawn_1_arg;
	callFuncRec wvproc_call_function_1_arg;
	evtLogRec wvproc_evt_log_control_1_arg;
    } argument;
    char *result;

    bool_t (*xdr_argument) (), (*xdr_result) ();
    char *(*local) ();
    char hostname [128];
    int zeroResult = 0;

    switch (rqstp->rq_proc)
        {
	case NULLPROC:
	    (void) svc_sendreply (transp, xdr_void, (char *) NULL);
	    return;

	case WVPROC_SYMTAB_LOOKUP:
	    xdr_argument = xdr_wrapstring;
	    xdr_result = xdr_u_long;
	    local = (char *(*) ()) wvproc_symtab_lookup_1;
	    break;

	case WVPROC_TASK_SPAWN:
	    xdr_argument = xdr_taskSpawnRec;
	    xdr_result = xdr_u_long;
	    local = (char *(*) ()) wvproc_task_spawn_1;
	    break;

	case WVPROC_CALL_FUNCTION:
	    xdr_argument = xdr_callFuncRec;
	    xdr_result = xdr_u_long;
	    local = (char *(*) ()) wvproc_call_function_1;
	    break;

	case WVPROC_EVT_LOG_CONTROL:
	    /* This entry point is handled specially because we'd like
	       to use the transport handle to find out the IP address of
	       the host that issued this request, and that's not usually
	       a paramter of the service routine. */

	    xdr_argument = xdr_evtLogRec;
	    xdr_result = xdr_u_long;

	    memset ((char *) &argument, 0, sizeof (argument));
	    if (!svc_getargs (transp, xdr_argument, &argument))
		{
		svcerr_decode (transp);
		return;
		}
	    
	    inet_ntoa_b (svc_getcaller (transp)->sin_addr, hostname);

	    wvHostInfoInit (hostname,
			    argument.wvproc_evt_log_control_1_arg.portNo);

	    if (argument.wvproc_evt_log_control_1_arg.state == TRUE)
                {
 
                if (argument.wvproc_evt_log_control_1_arg.mode == OBJECT_STATUS)
                    {
                    wvObjInstModeSet (INSTRUMENT_ON);
                    wvObjInst (1,0,INSTRUMENT_ON);
                    wvObjInst (2,0,INSTRUMENT_ON);
                    wvObjInst (3,0,INSTRUMENT_ON);
                    wvObjInst (4,0,INSTRUMENT_ON);
                    wvSigInst (INSTRUMENT_ON);
                    }
		wvEvtLogEnable (argument.wvproc_evt_log_control_1_arg.mode);
		}
	    else
		wvEvtLogDisable ();

	    if (!svc_sendreply (transp, xdr_result, (char *) &zeroResult))
		{
		svcerr_systemerr (transp);
		}
	    if (!svc_freeargs (transp, xdr_argument, &argument))
		{
		fprintf (stderr, "unable to free aguments");
		exit (1);
		}

	    return;

	default:
	    svcerr_noproc (transp);
	    return;
        }
    memset ((char *) &argument, 0, sizeof (argument));
    if (!svc_getargs (transp, xdr_argument, &argument))
        {
	svcerr_decode (transp);
	return;
        }
    result = (*local) (&argument, rqstp);
    if (result != NULL && !svc_sendreply (transp, xdr_result, result))
        {
	svcerr_systemerr (transp);
        }
    if (!svc_freeargs (transp, xdr_argument, &argument))
        {
	fprintf (stderr, "unable to free arguments");
	exit (1);
        }

    /* For reentrancy, the individual service routines return their results
       on the heap.  That memory is freed now. */

    if (result)
	{
	free (result);
	}
    
    return;
    }

/* clnt_perror.c - RPC client error print routines */

/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"
 

/*
 * Copyright (C) 1984, Sun Microsystems, Inc.
 *
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 *
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */

/*
modification history
--------------------
01k,15oct01,rae  merge from truestack ver 01l, base 01j (AE / 5_X)
01j,26may92,rrr  the tree shuffle
01i,04oct91,rrr  passed through the ansification filter
		  -changed includes to have absolute path from h/
		  -changed copyright notice
01h,08oct90,hjb   included "memLib.h".
01g,02oct90,hjb   cleanup and bug fix caused by rpc4.0 upgrade.
01f,10may90,dnw   changed taskModuleList->rpccreateerr to rpccreateerr macro
01e,19apr90,hjb   de-linted.
01d,27oct89,hjb   upgraded to 4.0
01c,05apr88,gae   changed fprintf() to printErr().
01b,11nov87,jlf   added wrs copyright, title, mod history, etc.
01a,01nov87,rdc   first VxWorks version
*/


#if !defined(lint) && defined(SCCSIDS)
/* static char sccsid[] = "@(#)clnt_perror.c 1.15 87/10/07 Copyr 1984 Sun Micro"; */
#endif

/*
 * clnt_perror.c
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 */

#include "vxWorks.h"
#include "rpc/rpctypes.h"
#include "rpc/xdr.h"
#include "rpc/auth.h"
#include "rpc/clnt.h"
#include "rpc/rpc_msg.h"
#include "rpc/rpcGbl.h"
#include "stdio.h"
#include "string.h"
#include "memLib.h"
#include "memPartLib.h"

static char *auth_errmsg();

/*
 * Print reply error info
 */
char *
clnt_sperror(rpch, s)
	CLIENT *rpch;
	char *s;
{
	struct rpc_err e;
	void clnt_perrno();
	char *err;
	char *str = taskRpcStatics->errBuf;
	char *strstart = str;

	if (str == NULL)
	    {
	    str = (char *) KHEAP_ALLOC(RPC_ERRBUF_SIZE);

	    if (str == NULL)
		{
		return (0);
		}

	    taskRpcStatics->errBuf = str;
	    strstart = str;
	    }

	CLNT_GETERR(rpch, &e);

	(void) sprintf(str, "%s: ", s);
	str += strlen(str);

	(void) strcpy(str, clnt_sperrno(e.re_status));
	str += strlen(str);

	switch (e.re_status) {
	case RPC_SUCCESS:
	case RPC_CANTENCODEARGS:
	case RPC_CANTDECODERES:
	case RPC_TIMEDOUT:
	case RPC_PROGUNAVAIL:
	case RPC_PROCUNAVAIL:
	case RPC_CANTDECODEARGS:
	case RPC_SYSTEMERROR:
	case RPC_UNKNOWNHOST:
	case RPC_UNKNOWNPROTO:
	case RPC_PMAPFAILURE:
	case RPC_PROGNOTREGISTERED:
	case RPC_FAILED:
		break;

	case RPC_CANTSEND:
	case RPC_CANTRECV:
		(void) sprintf(str, "; errno = %d", e.re_errno);
		str += strlen(str);
		break;

	case RPC_VERSMISMATCH:
		(void) sprintf(str,
			"; low version = %lu, high version = %lu",
			e.re_vers.low, e.re_vers.high);
		str += strlen(str);
		break;

	case RPC_AUTHERROR:
		err = auth_errmsg(e.re_why);
		(void) sprintf(str,"; why = ");
		str += strlen(str);
		if (err != NULL) {
			(void) sprintf(str, "%s",err);
		} else {
			(void) sprintf(str,
				"(unknown authentication error - %d)",
				(int) e.re_why);
		}
		str += strlen(str);
		break;

	case RPC_PROGVERSMISMATCH:
		(void) sprintf(str,
			"; low version = %lu, high version = %lu",
			e.re_vers.low, e.re_vers.high);
		str += strlen(str);
		break;

	default:	/* unknown */
		(void) sprintf(str,
			"; s1 = %lu, s2 = %lu",
			e.re_lb.s1, e.re_lb.s2);
		str += strlen(str);
		break;
	}
	(void) sprintf(str, "\n");
	return(strstart) ;
}

void
clnt_perror(rpch, s)
	CLIENT *rpch;
	char *s;
{
	printErr("%s", clnt_sperror(rpch, s));
}


struct rpc_errtab {
	enum clnt_stat status;
	char *message;
};

static struct rpc_errtab  rpc_errlist[] = {
	{ RPC_SUCCESS,
		"RPC: Success" },
	{ RPC_CANTENCODEARGS,
		"RPC: Can't encode arguments" },
	{ RPC_CANTDECODERES,
		"RPC: Can't decode result" },
	{ RPC_CANTSEND,
		"RPC: Unable to send" },
	{ RPC_CANTRECV,
		"RPC: Unable to receive" },
	{ RPC_TIMEDOUT,
		"RPC: Timed out" },
	{ RPC_VERSMISMATCH,
		"RPC: Incompatible versions of RPC" },
	{ RPC_AUTHERROR,
		"RPC: Authentication error" },
	{ RPC_PROGUNAVAIL,
		"RPC: Program unavailable" },
	{ RPC_PROGVERSMISMATCH,
		"RPC: Program/version mismatch" },
	{ RPC_PROCUNAVAIL,
		"RPC: Procedure unavailable" },
	{ RPC_CANTDECODEARGS,
		"RPC: Server can't decode arguments" },
	{ RPC_SYSTEMERROR,
		"RPC: Remote system error" },
	{ RPC_UNKNOWNHOST,
		"RPC: Unknown host" },
	{ RPC_UNKNOWNPROTO,
		"RPC: Unknown protocol" },
	{ RPC_PMAPFAILURE,
		"RPC: Port mapper failure" },
	{ RPC_PROGNOTREGISTERED,
		"RPC: Program not registered"},
	{ RPC_FAILED,
		"RPC: Failed (unspecified error)"}
};


/*
 * This interface for use by clntrpc
 */
char *
clnt_sperrno(stat)
	enum clnt_stat stat;
{
	int i;

	for (i = 0; i < sizeof(rpc_errlist)/sizeof(struct rpc_errtab); i++) {
		if (rpc_errlist[i].status == stat) {
			return (rpc_errlist[i].message);
		}
	}
	return ("RPC: (unknown error code)");
}

void
clnt_perrno(num)
	enum clnt_stat num;
{
	printErr("%s", clnt_sperrno(num));
}


char *
clnt_spcreateerror(s)
	char *s;
{
	char *str = taskRpcStatics->errBuf;

	if (str == NULL)
	    {
	    str = (char *) KHEAP_ALLOC(RPC_ERRBUF_SIZE);

	    if (str == NULL)
		{
		return (0);
		}

	    taskRpcStatics->errBuf = str;
	    }

	(void) sprintf(str, "%s: ", s);
	(void) strcat(str, clnt_sperrno(rpc_createerr.cf_stat));
	switch (rpc_createerr.cf_stat) {
	case RPC_PMAPFAILURE:
		(void) strcat(str, " - ");
		(void) strcat(str,
		    clnt_sperrno(rpc_createerr.cf_error.re_status));
		break;

	case RPC_SYSTEMERROR:
		(void) strcat(str, " - ");
		(void) sprintf(&str[strlen(str)], "Error %d",
		    rpc_createerr.cf_error.re_errno);
		break;

	default:
		break;
	}
	(void) strcat(str, "\n");
	return (str);
}

void
clnt_pcreateerror(s)
	char *s;
{
	printErr("%s", clnt_spcreateerror(s));
}

struct auth_errtab {
	enum auth_stat status;
	char *message;
};

static struct auth_errtab auth_errlist[] = {
	{ AUTH_OK,
		"Authentication OK" },
	{ AUTH_BADCRED,
		"Invalid client credential" },
	{ AUTH_REJECTEDCRED,
		"Server rejected credential" },
	{ AUTH_BADVERF,
		"Invalid client verifier" },
	{ AUTH_REJECTEDVERF,
		"Server rejected verifier" },
	{ AUTH_TOOWEAK,
		"Client credential too weak" },
	{ AUTH_INVALIDRESP,
		"Invalid server verifier" },
	{ AUTH_FAILED,
		"Failed (unspecified error)" },
};

static char *
auth_errmsg(stat)
	enum auth_stat stat;
{
	int i;

	for (i = 0; i < sizeof(auth_errlist)/sizeof(struct auth_errtab); i++) {
		if (auth_errlist[i].status == stat) {
			return(auth_errlist[i].message);
		}
	}
	return(NULL);
}

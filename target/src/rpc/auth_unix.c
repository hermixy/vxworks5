/* auth_unix.c - implements UNIX style authentication parameters */

/* Copyright 1984-2000 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
 * Copyright (c) 1987 Wind River Systems, Inc.
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
01q,23oct01,rae  fixed SPR #71008
01p,18apr00,ham  fixed compilation warnings.
01o,26may92,rrr  the tree shuffle
01n,04oct91,rrr  passed through the ansification filter
		  -changed includes to have absolute path from h/
		  -fixed #else and #endif
		  -changed copyright notice
01m,25oct90,dnw   removed include of utime.h.
01l,15jul90,dnw   coerced mem_alloc() to (char*) where necessary
01k,11may90,yao   added missing modification history (01i) for the last checkin.
01i,09may90,yao   deleted declaration of malloc.
01h,19apr90,hjb   de-linted.
01g,27oct89,hjb   upgraded to 4.0
01f,04jun88,llk   got rid of "bizarre behavior" message in _create and _refresh.
01e,05apr88,gae   changed fprintf() to printErr().
01d,04jan88,rdc   kludged the date in authunix_create for nfs.
01c,11nov87,jlf   added wrs copyright, title, mod history, etc.
01b,06nov87,dnw   changed conditionals on KERNEL to DONT_PRINT_ERRORS.
01a,01nov87,rdc   first VxWorks version
*/


/*
 * auth_unix.c, Implements UNIX style authentication parameters.
 *
 * The system is very weak.  The client uses no encryption for it's
 * credentials and only sends null verifiers.  The server sends backs
 * null verifiers or optionally a verifier that suggests a new short hand
 * for the credentials.
 *
 */

#include "rpc/rpctypes.h"
#include "rpc/xdr.h"
#include "rpc/auth.h"
#include "rpc/auth_unix.h"
#include "vxWorks.h"
#include "memLib.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

IMPORT bool_t xdr_opaque_auth ();

/*
 * Unix authenticator operations vector
 */
LOCAL void	authunix_nextverf();			/* 4.0 */
LOCAL bool_t	authunix_marshal();			/* 4.0 */
LOCAL bool_t	authunix_validate();			/* 4.0 */
LOCAL bool_t	authunix_refresh();			/* 4.0 */
LOCAL void	authunix_destroy();			/* 4.0 */

static struct auth_ops auth_unix_ops = {
	authunix_nextverf,
	authunix_marshal,
	authunix_validate,
	authunix_refresh,
	authunix_destroy
};

/*
 * This struct is pointed to by the ah_private field of an auth_handle.
 */
struct audata {
	struct opaque_auth	au_origcred;	/* original credentials */
	struct opaque_auth	au_shcred;	/* short hand cred */
	u_long			au_shfaults;	/* short hand cache faults */
        char			au_marshed[MAX_AUTH_BYTES];
	u_int			au_mpos;	/* xdr pos at end of marshed */
};
#define	AUTH_PRIVATE(auth)	((struct audata *)auth->ah_private)

LOCAL void marshal_new_auth();			/* 4.0 */


/*
 * Create a unix style authenticator.
 * Returns an auth handle with the given stuff in it.
 */
AUTH *
authunix_create(machname, uid, gid, len, aup_gids)
	char *machname;
	int uid;
	int gid;
	register int len;
	int *aup_gids;
{
	struct authunix_parms aup;
	char mymem[MAX_AUTH_BYTES];
	/*struct timeval now;*/
	XDR xdrs;
	register AUTH *auth;
	register struct audata *au;

	/*
	 * Allocate and set up auth handle
	 */
	auth = (AUTH *)mem_alloc(sizeof(*auth));
#ifndef DONT_PRINT_ERRORS
	if (auth == NULL) {
		printErr ("authunix_create: out of memory\n");
		return (NULL);
	}
#endif
	au = (struct audata *)mem_alloc(sizeof(*au));
#ifndef DONT_PRINT_ERRORS
	if (au == NULL) {
		printErr ("authunix_create: out of memory\n");
		return (NULL);
	}
#endif
	auth->ah_ops = &auth_unix_ops;
	auth->ah_private = (caddr_t)au;
	auth->ah_verf = au->au_shcred = _null_auth;
	au->au_shfaults = 0;

	/*
	 * fill in param struct from the given params
	 */

	/* WARNING: gettimeofday unimplemented - be wary of bizarre behavior. */

	/* XXX (void)gettimeofday(&now,  (struct timezone *)0); */
	/* XXX aup.aup_time = now.tv_sec; */

	aup.aup_time = 10000;
	aup.aup_machname = machname;
	aup.aup_uid = uid;
	aup.aup_gid = gid;
	aup.aup_len = (u_int)len;
	aup.aup_gids = aup_gids;

	/*
	 * Serialize the parameters into origcred
	 */
	xdrmem_create(&xdrs, mymem, MAX_AUTH_BYTES, XDR_ENCODE);
	if (! xdr_authunix_parms(&xdrs, &aup))
		abort();
	au->au_origcred.oa_length = len = XDR_GETPOS(&xdrs);
	au->au_origcred.oa_flavor = AUTH_UNIX;
#ifdef DONT_PRINT_ERRORS
	au->au_origcred.oa_base = (char *) mem_alloc(len);
#else
	if ((au->au_origcred.oa_base = (char *) mem_alloc((unsigned) len)) ==
									NULL) {
		printErr ("authunix_create: out of memory\n");
		return (NULL);
	}
#endif
	bcopy(mymem, au->au_origcred.oa_base, len);

	/*
	 * set auth handle to reflect new cred.
	 */
	auth->ah_cred = au->au_origcred;
	marshal_new_auth(auth);
	return (auth);
}


/* XXX We're not going to support this - it's too hard. */
#if FALSE
/*
 * Returns an auth handle with parameters determined by doing lots of
 * syscalls.
 */
AUTH *
authunix_create_default()
{
	register int len;
	char machname[MAX_MACHINE_NAME + 1];
	register int uid;
	register int gid;
	int gids[NGRPS];

	if (gethostname(machname, MAX_MACHINE_NAME) == -1)
		abort();
	machname[MAX_MACHINE_NAME] = 0;
	uid = geteuid();
	gid = getegid();
	if ((len = getgroups(NGRPS, gids)) < 0)
		abort();
	return (authunix_create(machname, uid, gid, len, gids));
}

#endif	/* FALSE */

/*
 * authunix operations
 */

/* ARGSUSED0 */
LOCAL void			/* 4.0 */
authunix_nextverf(auth)
	AUTH *auth;
{
	/* no action necessary */
}

LOCAL bool_t			/* 4.0 */
authunix_marshal(auth, xdrs)
	AUTH *auth;
	XDR *xdrs;
{
	register struct audata *au = AUTH_PRIVATE(auth);

	return (XDR_PUTBYTES(xdrs, au->au_marshed, au->au_mpos));
}

LOCAL bool_t			/* 4.0 */
authunix_validate(auth, verf)
	register AUTH *auth;
	struct opaque_auth *verf;
{
	register struct audata *au;
	XDR xdrs;

	if (verf->oa_flavor == AUTH_SHORT) {
		au = AUTH_PRIVATE(auth);
		xdrmem_create(&xdrs, verf->oa_base, verf->oa_length, XDR_DECODE);

		if (au->au_shcred.oa_base != NULL) {
			mem_free(au->au_shcred.oa_base,
			    au->au_shcred.oa_length);
			au->au_shcred.oa_base = NULL;
		}
		if (xdr_opaque_auth(&xdrs, &au->au_shcred)) {
			auth->ah_cred = au->au_shcred;
		} else {
			xdrs.x_op = XDR_FREE;
			(void)xdr_opaque_auth(&xdrs, &au->au_shcred);
			au->au_shcred.oa_base = NULL;
			auth->ah_cred = au->au_origcred;
		}
		marshal_new_auth(auth);
	}
	return (TRUE);
}

LOCAL bool_t			/* 4.0 */
authunix_refresh(auth)
	register AUTH *auth;
{
	register struct audata *au = AUTH_PRIVATE(auth);
	struct authunix_parms aup;
	struct timeval now;
	XDR xdrs;
	register int stat;

	if (auth->ah_cred.oa_base == au->au_origcred.oa_base) {
		/* there is no hope.  Punt */
		return (FALSE);
	}
	au->au_shfaults ++;

	/* first deserialize the creds back into a struct authunix_parms */
	aup.aup_machname = NULL;
	aup.aup_gids = (int *)NULL;
	xdrmem_create(&xdrs, au->au_origcred.oa_base,
	    au->au_origcred.oa_length, XDR_DECODE);
	stat = xdr_authunix_parms(&xdrs, &aup);
	if (! stat)
		goto done;

	/* update the time and serialize in place */

	/* WARNING: gettimeofday unimplemented - be wary of bizarre behavior. */

	/* XXX (void)gettimeofday(&now, (struct timezone *)0); */
	aup.aup_time = now.tv_sec;
	xdrs.x_op = XDR_ENCODE;
	XDR_SETPOS(&xdrs, 0);
	stat = xdr_authunix_parms(&xdrs, &aup);
	if (! stat)
		goto done;
	auth->ah_cred = au->au_origcred;
	marshal_new_auth(auth);
done:
	/* free the struct authunix_parms created by deserializing */
	xdrs.x_op = XDR_FREE;
	(void)xdr_authunix_parms(&xdrs, &aup);
	XDR_DESTROY(&xdrs);
	return (stat);
}

LOCAL void			/* 4.0 */
authunix_destroy(auth)
	register AUTH *auth;
{
	register struct audata *au = AUTH_PRIVATE(auth);

	mem_free(au->au_origcred.oa_base, au->au_origcred.oa_length);

	if (au->au_shcred.oa_base != NULL)
		mem_free(au->au_shcred.oa_base, au->au_shcred.oa_length);

	mem_free(auth->ah_private, sizeof(struct audata));

	if (auth->ah_verf.oa_base != NULL)
		mem_free(auth->ah_verf.oa_base, auth->ah_verf.oa_length);

	mem_free((caddr_t)auth, sizeof(*auth));
}

/*
 * Marshals (pre-serializes) an auth struct.
 * sets private data, au_marshed and au_mpos
 */
LOCAL void			/* 4.0 */
marshal_new_auth(auth)
	register AUTH *auth;
{
	XDR		xdr_stream;
	register XDR	*xdrs = &xdr_stream;
	register struct audata *au = AUTH_PRIVATE(auth);

	xdrmem_create(xdrs, au->au_marshed, MAX_AUTH_BYTES, XDR_ENCODE);
	if ((! xdr_opaque_auth(xdrs, &(auth->ah_cred))) ||
	    (! xdr_opaque_auth(xdrs, &(auth->ah_verf)))) {
		perror("auth_none.c - Fatal marshalling problem");
	} else {
		au->au_mpos = XDR_GETPOS(xdrs);
	}
	XDR_DESTROY(xdrs);
}

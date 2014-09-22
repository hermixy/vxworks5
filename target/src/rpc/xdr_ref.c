/* xdr_ref.c - XDR primitives for pointers */

/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01j,05nov01,vvv  fixed compilation warnings
01i,18apr00,ham  fixed compilation warnings.
01h,26may92,rrr  the tree shuffle
01g,04oct91,rrr  passed through the ansification filter
		  -changed includes to have absolute path from h/
		  -changed copyright notice
01f,26jun90,hjb  removed sccsid[].
01e,11may90,yao	 added include file memLib.h.
01d,05apr88,gae  changed fprintf() to printErr().
01c,11nov87,jlf  added wrs copyright, title, mod history, etc.
		 changed name from xdr_reference.c to xdr_ref.c, to
		      work under sys-V.
01b,06nov87,dnw  changed conditionals on KERNEL to DONT_PRINT_ERRORS.
01a,01nov87,rdc  first VxWorks version
*/

/*
 * xdr_reference.c, Generic XDR routines impelmentation.
 *
 * Copyright (c) 1987 Wind River Systems, Inc.
 * Copyright (C) 1984, Sun Microsystems, Inc.
 *
 * These are the "non-trivial" xdr primitives used to serialize and de-serialize
 * "pointers".  See xdr.h for more info on the interface to xdr.
 */

#ifndef lint
/* static char sccsid[] = "@(#)xdr_reference.c 1.1 86/09/24 Copyr 1984 Sun Micro"; */
#endif

#include "vxWorks.h"
#include "memLib.h"
#include "rpc/rpctypes.h"
#include "rpc/xdr.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#define LASTUNSIGNED	((u_int)0-1)

/*
 * XDR an indirect pointer
 * xdr_reference is for recursively translating a structure that is
 * referenced by a pointer inside the structure that is currently being
 * translated.  pp references a pointer to storage. If *pp is null
 * the  necessary storage is allocated.
 * size is the sizeof the referneced structure.
 * proc is the routine to handle the referenced structure.
 */
bool_t
xdr_reference(xdrs, pp, size, proc)
	register XDR *xdrs;
	caddr_t *pp;		/* the pointer to work on */
	u_int size;		/* size of the object pointed to */
	xdrproc_t proc;		/* xdr routine to handle the object */
{
	register caddr_t loc = *pp;
	register bool_t stat;

	if (loc == NULL)
		switch (xdrs->x_op) {
		case XDR_FREE:
			return (TRUE);

		case XDR_DECODE:
			*pp = loc = (caddr_t) mem_alloc(size);
			if (loc == NULL) {
				printErr ("xdr_reference: out of memory\n");
				return (FALSE);
			}
			bzero ((char *)loc, (int)size);
			break;
		default: /* XDR_ENCODE */
                        break;
	}

	stat = (*proc)(xdrs, loc, LASTUNSIGNED);

	if (xdrs->x_op == XDR_FREE) {
		mem_free(loc, size);
		*pp = NULL;
	}
	return (stat);
}


/*
 * xdr_pointer():
 *
 * XDR a pointer to a possibly recursive data structure. This
 * differs with xdr_reference in that it can serialize/deserialiaze
 * trees correctly.
 *
 *  What's sent is actually a union:
 *
 *  union object_pointer switch (boolean b) {
 *  case TRUE: object_data data;
 *  case FALSE: void nothing;
 *  }
 *
 * > objpp: Pointer to the pointer to the object.
 * > obj_size: size of the object.
 * > xdr_obj: routine to XDR an object.
 *
 */
bool_t
xdr_pointer(xdrs,objpp,obj_size,xdr_obj)
	register XDR *xdrs;
	char **objpp;
	u_int obj_size;
	xdrproc_t xdr_obj;
{

	bool_t more_data;

	more_data = (*objpp != NULL);
	if (! xdr_bool(xdrs,&more_data)) {
		return(FALSE);
	}
	if (! more_data) {
		*objpp = NULL;
		return(TRUE);
	}
	return(xdr_reference(xdrs,objpp,obj_size,xdr_obj));
}


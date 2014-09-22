/* xdr_array.c - XDR primatives for arrays */

/* Copyright 1984-1992 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01g,26may92,rrr  the tree shuffle
01f,04oct91,rrr  passed through the ansification filter
		  -changed includes to have absolute path from h/
		  -changed copyright notice
01e,26jun90,hjb  removed sccsid[].
01d,11may90,yao	 added include file memLib.h,
		 typecasted nodesize to int in routine xdr_array.
01c,11nov87,jlf  added wrs copyright, title, mod history, etc.
01b,06nov87,dnw  changed conditionals on KERNEL to DONT_PRINT_ERRORS.
01a,01nov87,rdc  first VxWorks version
*/

#ifndef lint
/* static char sccsid[] = "@(#)xdr_array.c 1.1 86/09/24 Copyr 1984 Sun Micro"; */
#endif

/*
 * Copyright (c) 1987 Wind River Systems, Inc.
 * Copyright (C) 1984, Sun Microsystems, Inc.
 *
 * These are the "non-trivial" xdr primitives used to serialize and de-serialize
 * arrays.  See xdr.h for more info on the interface to xdr.
 */

#include "vxWorks.h"
#include "memLib.h"
#include "rpc/rpctypes.h"
#include "rpc/xdr.h"
#include "private/funcBindP.h"
#include "stdlib.h"
#include "string.h"

#define PRINTERR if (_func_printErr != NULL) _func_printErr

#define LASTUNSIGNED	((u_int)0-1)


/*
 * XDR an array of arbitrary elements
 * *addrp is a pointer to the array, *sizep is the number of elements.
 * If addrp is NULL (*sizep * elsize) bytes are allocated.
 * elsize is the size (in bytes) of each element, and elproc is the
 * xdr procedure to call to handle each element of the array.
 */
bool_t
xdr_array(xdrs, addrp, sizep, maxsize, elsize, elproc)
	register XDR *xdrs;
	caddr_t *addrp;		/* array pointer */
	u_int *sizep;		/* number of elements */
	u_int maxsize;		/* max numberof elements */
	u_int elsize;		/* size in bytes of each element */
	xdrproc_t elproc;	/* xdr routine to handle each element */
{
	register u_int i;
	register caddr_t target = *addrp;
	register u_int c;  /* the actual element count */
	register bool_t stat = TRUE;
	register u_int nodesize;

	/* like strings, arrays are really counted arrays */
	if (! xdr_u_int(xdrs, sizep)) {
#ifndef DONT_PRINT_ERRORS
		PRINTERR("xdr_array: size FAILED\n");
#endif
		return (FALSE);
	}
	c = *sizep;
	if ((c > maxsize) && (xdrs->x_op != XDR_FREE)) {
#ifndef DONT_PRINT_ERRORS
		PRINTERR("xdr_array: bad size FAILED\n");
#endif
		return (FALSE);
	}
	nodesize = c * elsize;

	/*
	 * if we are deserializing, we may need to allocate an array.
	 * We also save time by checking for a null array if we are freeing.
	 */
	if (target == NULL)
		switch (xdrs->x_op) {
		case XDR_DECODE:
			if (c == 0)
				return (TRUE);
			*addrp = target = (caddr_t) mem_alloc(nodesize);
#ifndef DONT_PRINT_ERRORS
			if (target == NULL) {
				PRINTERR("xdr_array: out of memory\n");
				return (FALSE);
			}
#endif
			bzero(target, (int)nodesize);
			break;

		case XDR_FREE:
			return (TRUE);

		case XDR_ENCODE:
			break;
	}

	/*
	 * now we xdr each element of array
	 */
	for (i = 0; (i < c) && stat; i++) {
		stat = (*elproc)(xdrs, target, LASTUNSIGNED);
		target += elsize;
	}

	/*
	 * the array may need freeing
	 */
	if (xdrs->x_op == XDR_FREE) {
		mem_free(*addrp, nodesize);
		*addrp = NULL;
	}
	return (stat);
}

/*
 * xdr_vector():
 *
 * XDR a fixed length array. Unlike variable-length arrays,
 * the storage of fixed length arrays is static and unfreeable.
 * > basep: base of the array
 * > size: size of the array
 * > elemsize: size of each element
 * > xdr_elem: routine to XDR each element
 */
bool_t
xdr_vector(xdrs, basep, nelem, elemsize, xdr_elem)
	register XDR *xdrs;
	register char *basep;
	register u_int nelem;
	register u_int elemsize;
	register xdrproc_t xdr_elem;
{
	register u_int i;
	register char *elptr;

	elptr = basep;
	for (i = 0; i < nelem; i++) {
		if (! (*xdr_elem)(xdrs, elptr, LASTUNSIGNED)) {
			return(FALSE);
		}
		elptr += elemsize;
	}
	return(TRUE);
}



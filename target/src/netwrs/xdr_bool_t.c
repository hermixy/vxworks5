/* xdr_bool_t.c  - xdr routine */

/* Copyright 1984-1992 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01c,26may92,rrr  the tree shuffle
01b,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -changed copyright notice
01a,19apr88,llk  created.
*/

/*
DESCRIPTION
This module contains the eXternal Data Representation (XDR) routine
for bool_t's.
*/

#include "rpc/rpc.h"

bool_t
xdr_bool_t
    (
        XDR *xdrs,
        bool_t *objp
    )
{
    return (xdr_bool (xdrs, objp));
}

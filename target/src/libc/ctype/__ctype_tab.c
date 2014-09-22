/* ctype - ctype library routines */

/* Copyright 1992 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,20sep92,smb  documentation additions.
01b,04jul92,smb  added vxWorks.h and extra documentation
01a,24mar92,smb  written
*/

/*
DESCRIPTION
This table contains character attributes for all ASCII-coded characters.
A character's integer value is used as an index into the
table where the character's attributes are or'd together.

INCLUDE FILE: ctype.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "ctype.h"

static const unsigned char __ctype_tab[257] =
    {
    /* 0 */ 	0,				/* dummy */
    /* 80 */	_C_B,				/* nul */
    /* 80 */	_C_B,				/* soh */
    /* 80 */	_C_B,				/* stx */
    /* 80 */	_C_B,				/* etx */
    /* 80 */	_C_B,				/* eot */
    /* 80 */	_C_B,				/* enq */
    /* 80 */	_C_B,				/* ack */
    /* 80 */	_C_B,				/* bel */
    /* 80 */	_C_B,				/* bs */
    /* 20 */	_C_CONTROL,			/* ht */
    /* 20 */	_C_CONTROL,			/* nl */
    /* 20 */	_C_CONTROL,			/* vt */
    /* 20 */	_C_CONTROL,			/* np */
    /* 20 */	_C_CONTROL,			/* cr */
    /* 80 */	_C_B,				/* so */
    /* 80 */	_C_B,				/* si */
    /* 80 */	_C_B,				/* dle */
    /* 80 */	_C_B,				/* dc1 */
    /* 80 */	_C_B,				/* dc2 */
    /* 80 */	_C_B,				/* dc3 */
    /* 80 */	_C_B,				/* dc4 */
    /* 80 */	_C_B,				/* nak */
    /* 80 */	_C_B,				/* syn */
    /* 80 */	_C_B,				/* etb */
    /* 80 */	_C_B,				/* can */
    /* 80 */	_C_B,				/* em */
    /* 80 */	_C_B,				/* sub */
    /* 80 */	_C_B,				/* esc */
    /* 80 */	_C_B,				/* fs */
    /* 80 */	_C_B,				/* gs */
    /* 80 */	_C_B,				/* rs */
    /* 80 */	_C_B,				/* us */
    /* 08 */	_C_WHITE_SPACE,			/* space */
    /* 10 */	_C_PUNCT,			/* ! */
    /* 10 */	_C_PUNCT,			/* " */
    /* 10 */	_C_PUNCT,			/* # */
    /* 10 */	_C_PUNCT,			/* $ */
    /* 10 */	_C_PUNCT,			/* % */
    /* 10 */	_C_PUNCT,			/* & */
    /* 10 */	_C_PUNCT,			/* ' */
    /* 10 */	_C_PUNCT,			/* ( */
    /* 10 */	_C_PUNCT,			/* ) */
    /* 10 */	_C_PUNCT,			/* * */
    /* 10 */	_C_PUNCT,			/* + */
    /* 10 */	_C_PUNCT,			/* , */
    /* 10 */	_C_PUNCT,			/* - */
    /* 10 */	_C_PUNCT,			/* . */
    /* 10 */	_C_PUNCT,			/* / */
    /* 44 */	_C_NUMBER | _C_HEX_NUMBER,	/* 0 */
    /* 44 */	_C_NUMBER | _C_HEX_NUMBER,	/* 1 */
    /* 44 */	_C_NUMBER | _C_HEX_NUMBER,	/* 2 */
    /* 44 */	_C_NUMBER | _C_HEX_NUMBER,	/* 3 */
    /* 44 */	_C_NUMBER | _C_HEX_NUMBER,	/* 4 */
    /* 44 */	_C_NUMBER | _C_HEX_NUMBER,	/* 5 */
    /* 44 */	_C_NUMBER | _C_HEX_NUMBER,	/* 6 */
    /* 44 */	_C_NUMBER | _C_HEX_NUMBER,	/* 7 */
    /* 44 */	_C_NUMBER | _C_HEX_NUMBER,	/* 8 */
    /* 44 */	_C_NUMBER | _C_HEX_NUMBER,	/* 9 */
    /* 10 */	_C_PUNCT,			/* : */
    /* 10 */	_C_PUNCT,			/* ; */
    /* 10 */	_C_PUNCT,			/* < */
    /* 10 */	_C_PUNCT,			/* = */
    /* 10 */	_C_PUNCT,			/* > */
    /* 10 */	_C_PUNCT,			/* ? */
    /* 10 */	_C_PUNCT,			/* @ */
    /* 41 */	_C_UPPER | _C_HEX_NUMBER,	/* A */
    /* 41 */	_C_UPPER | _C_HEX_NUMBER,	/* B */
    /* 41 */	_C_UPPER | _C_HEX_NUMBER,	/* C */
    /* 41 */	_C_UPPER | _C_HEX_NUMBER,	/* D */
    /* 41 */	_C_UPPER | _C_HEX_NUMBER,	/* E */
    /* 41 */	_C_UPPER | _C_HEX_NUMBER,	/* F */
    /* 01 */	_C_UPPER,			/* G */
    /* 01 */	_C_UPPER,			/* H */
    /* 01 */	_C_UPPER,			/* I */
    /* 01 */	_C_UPPER,			/* J */
    /* 01 */	_C_UPPER,			/* K */
    /* 01 */	_C_UPPER,			/* L */
    /* 01 */	_C_UPPER,			/* M */
    /* 01 */	_C_UPPER,			/* N */
    /* 01 */	_C_UPPER,			/* O */
    /* 01 */	_C_UPPER,			/* P */
    /* 01 */	_C_UPPER,			/* Q */
    /* 01 */	_C_UPPER,			/* R */
    /* 01 */	_C_UPPER,			/* S */
    /* 01 */	_C_UPPER,			/* T */
    /* 01 */	_C_UPPER,			/* U */
    /* 01 */	_C_UPPER,			/* V */
    /* 01 */	_C_UPPER,			/* W */
    /* 01 */	_C_UPPER,			/* X */
    /* 01 */	_C_UPPER,			/* Y */
    /* 01 */	_C_UPPER,			/* Z */
    /* 10 */	_C_PUNCT,			/* [ */
    /* 10 */	_C_PUNCT,			/* \ */
    /* 10 */	_C_PUNCT,			/* ] */
    /* 10 */	_C_PUNCT,			/* ^ */
    /* 10 */	_C_PUNCT,			/* _ */
    /* 10 */	_C_PUNCT,			/* ` */
    /* 42 */	_C_LOWER | _C_HEX_NUMBER,	/* a */
    /* 42 */	_C_LOWER | _C_HEX_NUMBER,	/* b */
    /* 42 */	_C_LOWER | _C_HEX_NUMBER,	/* c */
    /* 42 */	_C_LOWER | _C_HEX_NUMBER,	/* d */
    /* 42 */	_C_LOWER | _C_HEX_NUMBER,	/* e */
    /* 42 */	_C_LOWER | _C_HEX_NUMBER,	/* f */
    /* 02 */	_C_LOWER,			/* g */
    /* 02 */	_C_LOWER,			/* h */
    /* 02 */	_C_LOWER,			/* i */
    /* 02 */	_C_LOWER,			/* j */
    /* 02 */	_C_LOWER,			/* k */
    /* 02 */	_C_LOWER,			/* l */
    /* 02 */	_C_LOWER,			/* m */
    /* 02 */	_C_LOWER,			/* n */
    /* 02 */	_C_LOWER,			/* o */
    /* 02 */	_C_LOWER,			/* p */
    /* 02 */	_C_LOWER,			/* q */
    /* 02 */	_C_LOWER,			/* r */
    /* 02 */	_C_LOWER,			/* s */
    /* 02 */	_C_LOWER,			/* t */
    /* 02 */	_C_LOWER,			/* u */
    /* 02 */	_C_LOWER,			/* v */
    /* 02 */	_C_LOWER,			/* w */
    /* 02 */	_C_LOWER,			/* x */
    /* 02 */	_C_LOWER,			/* y */
    /* 02 */	_C_LOWER,			/* z */
    /* 10 */	_C_PUNCT,			/* { */
    /* 10 */	_C_PUNCT,			/* | */
    /* 10 */	_C_PUNCT,			/* } */
    /* 10 */	_C_PUNCT,			/* ~ */
    /* 80 */	_C_B				/* del */
    };	

const unsigned char *__ctype = &__ctype_tab[1];


/* localeconv.c - ANSI locale */

/* Copyright 1992-1995 Wind River Systems, Inc. */

/*
modification history
--------------------
01f,11feb95,jdi  doc tweaks.
01e,14mar93,jdi  fixed typo.
01d,07feb93,jdi  documentation cleanup for 5.1.
01c,20sep92,smb  documentation additions
01b,12jul92,smb  changed definition of localeconv for MIPS cpp.
01a,08jul92,smb  written.
*/

/*
DESCRIPTION
 
INCLUDE FILE: locale.h limits.h
 
SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "locale.h"
#include "limits.h"

/* locals */

static char null[] = "";

struct lconv __locale = 
    {
    /* LC_NUMERIC */
    ".", 		/* decimal_point */
    null, 		/* thousands_sep */
    null, 		/* grouping */
    /* LC_MONETARY */
    null, 		/* int_curr_symbol */
    null, 		/* currency_symbol */
    null, 		/* mon_decimal_point */
    null, 		/* mon_thousands_sep */
    null, 		/* mon_grouping */
    null, 		/* positive_sign */
    null,		/* negative_sign */
    CHAR_MAX, 		/* int_frac_digits */
    CHAR_MAX, 		/* frac_digits */
    CHAR_MAX, 		/* p_cs_precedes */
    CHAR_MAX,		/* p_sep_by_space */
    CHAR_MAX, 		/* n_cs_precedes */
    CHAR_MAX, 		/* n_sep_by_space */
    CHAR_MAX, 		/* p_sign_posn */
    CHAR_MAX 		/* n_sign_posn */
    };


#undef localeconv
/*******************************************************************************
*
* localeconv - set the components of an object with type `lconv' (ANSI)
*
* This routine sets the components of an object with type `struct lconv'
* with values appropriate for the formatting of numeric quantities
* (monetary and otherwise) according to the rules of the current locale.
*
* The members of the structure with type `char *' are pointers to strings
* any of which (except `decimal_point') can point to "" to indicate that
* the value is not available in the current locale or is of zero length.
* The members with type `char' are nonnegative numbers, any of which can be
* CHAR_MAX to indicate that the value is not available in the current locale.
* The members include the following:
* .iP "char *decimal_point" "" 3
* The decimal-point character used to format nonmonetary quantities.
* .iP "char *thousands_sep"
* The character used to separate groups of digits before the
* decimal-point character in formatted nonmonetary quantities.
* .iP "char *grouping"
* A string whose elements indicate the size of each group of
* digits in formatted nonmonetary quantities.
* .iP "char *int_curr_symbol"
* The international currency symbol applicable to the current
* locale. The first three characters contain the alphabetic international
* currency symbol in accordance with those specified in ISO 4217:1987.
* The fourth character (immediately preceding the null character) is the
* character used to separate the international currency symbol from
* the monetary quantity.
* .iP "char *currency_symbol"
* The local currency symbol applicable to the current locale.
* .iP "char *mon_decimal_point"
* The decimal-point used to format monetary quantities.
* .iP "char *mon_thousands_sep"
* The separator for groups of digits before the decimal-point in
* formatted monetary quantities.
* .iP "char *mon_grouping"
* A string whose elements indicate the size of each group of digits in
* formatted monetary quantities.
* .iP "char *positive_sign"
* The string used to indicate a nonnegative-valued formatted monetary
* quantity.
* .iP "char *negative_sign"
* The string used to indicate a negative-valued formatted monetary
* quantity.
* .iP "char int_frac_digits"
* The number of fractional digits (those after the decimal-point)
* to be displayed in an internationally formatted monetary quantity.
* .iP "char frac_digits"
* The number of fractional digits (those after the decimal-point)
* to be displayed in a formatted monetary quantity.
* .iP "char p_cs_precedes"
* Set to 1 or 0 if the `currency_symbol' respectively precedes or
* succeeds the value for a nonnegative formatted monetary quantity.
* .iP "char p_sep_by_space"
* Set to 1 or 0 if the `currency_symbol' respectively is or is not
* separated by a space from the value for a nonnegative formatted
* monetary quantity.
* .iP "char n_cs_precedes"
* Set to 1 or 0 if the `currency_symbol' respectively precedes or
* succeeds the value for a negative formatted monetary quantity.
* .iP "char n_sep_by_space"
* Set to 1 or 0 if the `currency_symbol' respectively is or is not
* separated by a space from the value for a negative formatted monetary
* quantity.
* .iP "char p_sign_posn"
* Set to a value indicating the positioning of the `positive_sign'
* for a nonnegative formatted monetary quantity.
* .iP "char n_sign_posn"
* Set to a value indicating the positioning of the `negative_sign'
* for a negative formatted monetary quantity.
*
* .LP
* The elements of `grouping' and `mon_grouping' are interpreted according
* to the following:
* .iP "CHAR_MAX" "" 1
* No further grouping is to be performed.
* .iP "0"
* The previous element is to be repeatedly used for the remainder of the digits.
* .iP "other"
* The integer value is the number of the digits that comprise the current
* group.  The next element is examined to determined the size of the next
* group of digits before the current group.
*
* .LP
* The values of `p_sign_posn' and `n_sign_posn' are interpreted according to
* the following:
* .iP "0"
* Parentheses surround the quantity and `currency_symbol'.
* .iP "1"
* The sign string precedes the quantity and `currency_symbol'.
* .iP "2"
* The sign string succeeds the quantity and `currency_symbol'.
* .iP "3"
* The sign string immediately precedes the `currency_symbol'.
* .iP "4"
* The sign string immediately succeeds the `currency_symbol'.
*
* .LP
* The implementation behaves as if no library function calls localeconv().
*
* The localeconv() routine returns a pointer to the filled-in object.  The
* structure pointed to by the return value is not modified by the
* program, but may be overwritten by a subsequent call to localeconv().
* In addition, calls to setlocale() with categories LC_ALL, LC_MONETARY,
* or LC_NUMERIC may overwrite the contents of the structure.
*
* INCLUDE FILES: locale.h, limits.h
*
* RETURNS: A pointer to the structure `lconv'.
*/

struct lconv *localeconv (void)
    {
    return (&__locale);
    }

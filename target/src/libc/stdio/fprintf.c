/* fprintf.c - print to a file. stdio.h */

/* Copyright 1992-1995 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,24jan95,rhp  doc: avoid 'L' in fprintf(), no long doubles 
                 in VxWorks (see SPR#3886)
01d,19jul94,dvs  doc tweak (SPR #2512).
01c,05mar93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions
01a,29jul92,jcf  Added OBJ_VERIFY
	    smb  taken from UCB stdio
*/

/*
DESCRIPTION
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.

INCLUDE FILE: stdio.h, stdarg.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "stdarg.h"
#include "fioLib.h"
#include "objLib.h"
#include "private/stdioP.h"


/******************************************************************************
*
* fprintf - write a formatted string to a stream (ANSI)
*
* This routine writes output to a specified stream under control of the string
* <fmt>. The string <fmt> contains ordinary characters, which are written
* unchanged, plus conversion specifications, which cause the arguments that
* follow <fmt> to be converted and printed as part of the formatted string.
*
* The number of arguments for the format is arbitrary, but they must
* correspond to the conversion specifications in <fmt>.  If there are
* insufficient arguments, the behavior is undefined.  If the format is
* exhausted while arguments remain, the excess arguments are evaluated but
* otherwise ignored.  The routine returns when the end of the format string
* is encountered.
*
* The format is a multibyte character sequence, beginning and ending in its
* initial shift state.  The format is composed of zero or more directives:
* ordinary multibyte characters (not `%') that are copied unchanged to the
* output stream; and conversion specification, each of which results in
* fetching zero or more subsequent arguments.  Each conversion specification
* is introduced by the `%' character.  After the `%', the following appear in
* sequence:
* .iP "" 4
* Zero or more flags (in any order) that modify the meaning of the 
* conversion specification.
* .iP
* An optional minimum field width.  If the converted value has fewer
* characters than the field width, it will be padded with spaces (by
* default) on the left (or right, if the left adjustment flag, 
* described later, has been given) to the field width. The field
* width takes the form of an asterisk (`*') (described later) or a decimal
* integer.
* .iP
* An optional precision that gives the minimum number of digits to
* appear for the `d', `i', `o', `u', `x', and `X' conversions, the number of 
* digits to appear after the decimal-point character for `e', `E', and `f'
* conversions, the maximum number of significant digits for the `g' and
* `G' conversions, or the maximum number of characters to be written 
* from a string in the `s' conversion.  The precision takes the form of a 
* period (`.') followed either by an asterisk (`*') (described later) or by
* an optional decimal integer; if only the period is specified, the 
* precision is taken as zero.  If a precision appears with any other
* conversion specifier, the behavior is undefined.
* .iP
* An optional `h' specifying that a following `d', `i', `o', `u', `x', and
* `X' conversion specifier applies to a `short int' or `unsigned short int'
* argument (the argument will have been promoted according to the integral
* promotions, and its value converted to `short int' or
* `unsigned short int' before printing); an optional `h' specifying that a
* following `n' conversion specifier applies to a pointer to a `short int'
* argument; an optional `l' (el) specifying that a following `d', `i', `o',
* `u', `x', and `X' conversion specifier applies to a `long int' or
* `unsigned long int' argument; or an optional `l' specifying that a following
* `n' conversion specifier applies to a pointer to a `long int'
* argument.  If an `h' or `l' appears with any other conversion
* specifier, the behavior is undefined.
*
* \&WARNING: ANSI C also specifies an optional `L' in some of the same
* contexts as `l' above, corresponding to a `long double' argument.
* However, the current release of the VxWorks libraries does not support 
* `long double' data; using the optional `L' gives unpredictable results.
* .iP
* A character that specifies the type of conversion to be applied.
* .LP
*
* As noted above, a field width, or precision, or both, can be indicated by
* an asterisk (`*').  In this case, an `int' argument supplies the field width
* or precision.  The arguments specifying field width, or precision, or both,
* should appear (in that order) before the argument (if any) to be converted.
* A negative field width argument is taken as a `-' flag followed by a positive
* field width.  A negative precision argument is taken as if the precision
* were omitted.
*
* The flag characters and their meanings are:
* .iP `-'
* The result of the conversion will be left-justified within the field.
* (it will be right-justified if this flag is not specified.)
* .iP `+'
* The result of a signed conversion will always begin with a plus or 
* minus sign.  (It will begin with a sign only when a negative value
* is converted if this flag is not specified.)
* .iP `space'
* If the first character of a signed conversion is not a sign, or 
* if a signed conversion results in no characters, a space will be 
* prefixed to the result.  If the `space' and `+' flags both appear, the
* `space' flag will be ignored.
* .iP `#'
* The result is to be converted to an "alternate form."  For `o' conversion
* it increases the precision to force the first digit of the result to be a
* zero.  For `x' (or `X') conversion, a non-zero result will have "0x" (or
* "0X") prefixed to it.  For `e', `E', `f', `g', and `G' conversions, the
* result will always contain a decimal-point character, even if no digits
* follow it.  (Normally, a decimal-point character appears in the result of
* these conversions only if no digit follows it).  For `g' and `G'
* conversions, trailing zeros will not be removed from the result.  For
* other conversions, the behavior is undefined.
* .iP `0'
* For `d', `i', `o', `u', `x', `X', `e', `E', `f', `g', and `G' conversions,
* leading zeros (following any indication of sign or base) are used to pad
* to the field width; no space padding is performed.  If the `0' and `-'
* flags both appear, the `0' flag will be ignored.  For `d', `i', `o', `u',
* `x', and `X' conversions, if a precision is specified, the `0' flag will
* be ignored.  For other conversions, the behavior is undefined.
* .LP
*
* The conversion specifiers and their meanings are:
* .iP "`d', `i'"
* The `int' argument is converted to signed decimal in the style
* `[-]dddd'.  The precision specifies the minimum number of digits 
* to appear; if the value being converted can be represented in
* fewer digits, it will be expanded with leading zeros.  The
* default precision is 1.  The result of converting a zero value
* with a precision of zero is no characters.
* .iP "`o', `u', `x', `X'"
* The `unsigned int' argument is converted to unsigned octal (`o'),
* unsigned decimal (`u'), or unsigned hexadecimal notation (`x' or `X')
* in the style `dddd'; the letters abcdef are used for `x' conversion
* and the letters ABCDEF for `X' conversion.  The precision specifies
* the minimum number of digits to appear; if the value being 
* converted can be represented in fewer digits, it will be 
* expanded with leading zeros.  The default precision is 1.  The
* result of converting a zero value with a precision of zero is 
* no characters.
* .iP `f'
* The `double' argument is converted to decimal notation in the 
* style `[-]ddd.ddd', where the number of digits after the decimal
* point character is equal to the precision specification.  If the
* precision is missing, it is taken as 6; if the precision is zero
* and the `#' flag is not specified, no decimal-point character
* appears.  If a decimal-point character appears, at least one 
* digit appears before it.  The value is rounded to the appropriate
* number of digits.
* .iP "`e', `E'"
* The `double' argument is converted in the style `[-]d.ddde+/-dd',
* where there is one digit before the decimal-point character 
* (which is non-zero if the argument is non-zero) and the number
* of digits after it is equal to the precision; if the precision
* is missing, it is taken as 6; if the precision is zero and the 
* `#' flag is not specified, no decimal-point character appears.  The
* value is rounded to the appropriate number of digits.  The `E'
* conversion specifier will produce a number with `E' instead of `e'
* introducing the exponent.  The exponent always contains at least
* two digits.  If the value is zero, the exponent is zero.
* .iP "`g', `G'"
* The `double' argument is converted in style `f' or `e' (or in style 
* `E' in the case of a `G' conversion specifier), with the precision
* specifying the number of significant digits.  If the precision
* is zero, it is taken as 1.  The style used depends on the 
* value converted; style `e' (or `E') will be used only if the 
* exponent resulting from such a conversion is less than -4 or 
* greater than or equal to the precision.  Trailing zeros are
* removed from the fractional portion of the result; a decimal-point
* character appears only if it is followed by a digit.
* .iP `c'
* The `int' argument is converted to an `unsigned char', and the 
* resulting character is written.
* .iP `s'
* The argument should be a pointer to an array of character type.
* Characters from the array are written up to (but not including)
* a terminating null character; if the precision is specified,
* no more than that many characters are written.  If the precision
* is not specified or is greater than the size of the array, the
* array will contain a null character.
* .iP `p'
* The argument should be a pointer to `void'.  The value of the 
* pointer is converted to a sequence of printable characters,
* in hexadecimal representation (prefixed with "0x").
* .iP `n'
* The argument should be a pointer to an integer into which
* the number of characters written to the output stream
* so far by this call to fprintf() is written.  No argument is converted.
* .iP `%'
* A `%' is written.  No argument is converted.  The complete
* conversion specification is %%.
* .LP
*
* If a conversion specification is invalid, the behavior is undefined.
*
* If any argument is, or points to, a union or an aggregate (except for an 
* array of character type using `s' conversion, or a pointer using `p' 
* conversion), the behavior is undefined.
*
* In no case does a non-existent or small field width cause truncation of a
* field if the result of a conversion is wider than the field width, the
* field is expanded to contain the conversion result.
*
* INCLUDE FILES: stdio.h 
*
* RETURNS:
* The number of characters written, or a negative value if an
* output error occurs.
*
* SEE ALSO: printf()
*/

int fprintf
    (
    FILE *	  fp,	/* stream to write to */
    const char *  fmt,	/* format string */
    ...			/* optional arguments to format string */
    )
    {
    int		  ret;
    va_list	  vaList;

    if (OBJ_VERIFY (fp, fpClassId) != OK)
	return (ERROR);

    va_start (vaList, fmt);

    ret = vfprintf (fp, fmt, vaList);

    va_end (vaList);

    return (ret);
    }

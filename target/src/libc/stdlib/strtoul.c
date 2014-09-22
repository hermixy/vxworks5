/* strtoul.c - strtoul file for stdlib  */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01d,08feb93,jdi  documentation cleanup for 5.1.
01c,21sep92,smb  tweaks for mg
01b,20sep92,smb  documentation additions.
01a,10jul92,smb  documented.
*/

/*
DESCRIPTION
 * Copyright (c) 1990 Regents of the University of California.
 * All rights reserved.
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

INCLUDE FILES: stdlib.h, limits.h, ctype.h, errno.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "limits.h"
#include "ctype.h"
#include "errno.h"
#include "stdlib.h"

/*****************************************************************************
*
* strtoul - convert a string to an unsigned long integer (ANSI)
*
* This routine converts the initial portion of a string <nptr> to
* `unsigned long int' representation.  First, it decomposes the input
* string into three parts:  an initial, possibly empty, sequence of
* white-space characters (as specified by isspace()); a subject sequence
* resembling an unsigned integer represented in some radix determined by
* the value <base>; and a final string of one or more unrecognized
* characters, including the terminating null character of the input string.
* Then, it attempts to convert the subject sequence to an unsigned integer,
* and returns the result.
*
* If the value of <base> is zero, the expected form of the subject sequence
* is that of an integer constant, optionally preceded by a plus or minus
* sign, but not including an integer suffix.  If the value of <base> is
* between 2 and 36, the expected form of the subject sequence is a sequence
* of letters and digits representing an integer with the radix specified by
* letters from a (or A) through z (or Z) which are ascribed the values 10 to
* 35; only letters whose ascribed values are less than <base> are
* premitted.  If the value of <base> is 16, the characters 0x or 0X may
* optionally precede the sequence of letters and digits, following the sign
* if present.
*
* The subject sequence is defined as the longest initial subsequence of the
* input string, starting with the first non-white-space character, that is
* of the expected form.  The subject sequence contains no characters if the
* input string is empty or consists entirely of white space, or if the first
* non-white-space character is other than a sign or a permissible letter or
* digit.
*
* If the subject sequence has the expected form and the value of <base> is
* zero, the sequence of characters starting with the first digit is
* interpreted as an integer constant.  If the subject sequence has the
* expected form and the value of <base> is between 2 and 36, it is used as
* the <base> for conversion, ascribing to each letter its value as given
* above.  If the subject sequence begins with a minus sign, the value
* resulting from the conversion is negated.  A pointer to the final string
* is stored in the object pointed to by <endptr>, provided that <endptr> is
* not a null pointer.
*
* In other than the "C" locale, additional implementation-defined subject
* sequence forms may be accepted.  VxWorks supports only the "C" locale;
* it assumes that the upper- and lower-case alphabets and digits are
* each contiguous.
*
* If the subject sequence is empty or does not have the expected form, no
* conversion is performed; the value of <nptr> is stored in the object
* pointed to by <endptr>, provided that <endptr> is not a null pointer.
*
* INCLUDE FILES: stdlib.h 
*
* RETURNS:
* The converted value, if any.  If no conversion could be performed it
* returns zero.  If the correct value is outside the range of representable
* values, it returns ULONG_MAX, and stores the value of the macro ERANGE in
* <errno>.
*/

ulong_t strtoul
    (
    const char *     nptr,	/* string to convert */
    char **          endptr,	/* ptr to final string */
    FAST int         base	/* radix */
    )
    {
    FAST const char *s = nptr;
    FAST ulong_t     acc;
    FAST int         c;
    FAST ulong_t     cutoff;
    FAST int         neg = 0;
    FAST int 	     any;
    FAST int 	     cutlim;

    /*
     * See strtol for comments as to the logic used.
     */
    do 
        {
    	c = *s++;
        } while (isspace (c));

    if (c == '-') 
        {
    	neg = 1;
    	c = *s++;
        } 
    else if (c == '+')
    	c = *s++;

    if (((base == 0) || (base == 16)) &&
        (c == '0') && 
        ((*s == 'x') || (*s == 'X'))) 
        {
    	c = s[1];
    	s += 2;
    	base = 16;
        }

    if (base == 0)
    	base = (c == '0' ? 8 : 10);

    cutoff = (ulong_t)ULONG_MAX / (ulong_t)base;
    cutlim = (ulong_t)ULONG_MAX % (ulong_t)base;

    for (acc = 0, any = 0;; c = *s++) 
        {
    	if (isdigit (c))
    	    c -= '0';
    	else if (isalpha (c))
    	    c -= (isupper (c) ? 'A' - 10 : 'a' - 10);
    	else
    	    break;

    	if (c >= base)
    	    break;

    	if ((any < 0) || (acc > cutoff) || (acc == cutoff) && (c > cutlim))
    	    any = -1;
    	else 
            {
    	    any = 1;
    	    acc *= base;
    	    acc += c;
    	    }
        }

    if (any < 0) 
        {
    	acc = ULONG_MAX;
    	errno = ERANGE;
        } 
    else if (neg)
    	acc = -acc;

    if (endptr != 0)
    	*endptr = (any ? CHAR_FROM_CONST (s - 1) : CHAR_FROM_CONST (nptr));

    return (acc);
    }

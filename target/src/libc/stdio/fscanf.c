/* fscanf.c - scan a file. stdio.h */

/* Copyright 1992-1995 Wind River Systems, Inc. */

/*
modification history
--------------------
01d,24jan95,rhp  doc: avoid 'L' in fscanf(), no long doubles 
                 in VxWorks (see SPR#3886)
01c,05mar93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions
01a,29jul92,jcf  Added OBJ_VERIFY
	   +smb  taken from UCB stdio
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
#include "fioLib.h"
#include "stdarg.h"
#include "objLib.h"
#include "private/stdioP.h"


/******************************************************************************
*
* fscanf - read and convert characters from a stream (ANSI)
* 
* This routine reads characters from a specified stream, and interprets them
* according to format specifications in the string <fmt>, which specifies
* the admissible input sequences and how they are to be converted for
* assignment, using subsequent arguments as pointers to the objects to
* receive the converted input.
*
* If there are insufficient arguments for the format, the behavior is
* undefined.  If the format is exhausted while arguments remain, the excess
* arguments are evaluated but are otherwise ignored.
*
* The format is a multibyte character sequence, beginning and ending in
* its initial shift state.  The format is composed of zero or more directives:
* one or more white-space characters; an ordinary multibyte character (neither
* `%' nor a white-space character); or a conversion specification.  Each
* conversion specification is introduced by the `%' character.  After the `%',
* the following appear in sequence:
*
* .iP "" 4
* An optional assignment-suppressing character `*'.
* .iP
* An optional non-zero decimal integer that specifies the maximum field 
* width.
* .iP
* An optional `h' or `l' (el) indicating the size of the receiving 
* object.  The conversion specifiers `d', `i', and `n' should be preceded by
* `h' if the corresponding argument is a pointer to `short int' rather
* than a pointer to `int', or by `l' if it is a pointer to `long int'.
* Similarly, the conversion specifiers `o', `u', and `x' shall be preceded
* by `h' if the corresponding argument is a pointer to `unsigned short int'
* rather than a pointer to `unsigned int', or by `l' if it is a pointer to
* `unsigned long int'.  Finally, the conversion specifiers `e', `f', and `g' 
* shall be preceded by `l' if the corresponding argument is a pointer to
* `double' rather than a pointer to `float'.  If an `h' or `l' appears
* with any other conversion specifier, the behavior is undefined.
*
* \&WARNING: ANSI C also specifies an optional `L' in some of the same
* contexts as `l' above, corresponding to a `long double *' argument.
* However, the current release of the VxWorks libraries does not support 
* `long double' data; using the optional `L' gives unpredictable results.
* .iP
* A character that specifies the type of conversion to be applied.  The
* valid conversion specifiers are described below.
* .LP
*
* The fscanf() routine executes each directive of the format in turn.  If a 
* directive fails, as detailed below, fscanf() returns.  Failures
* are described as input failures (due to the unavailability of input
* characters), or matching failures (due to inappropriate input).
* 
* A directive composed of white-space character(s) is executed by reading
* input up to the first non-white-space character (which remains unread),
* or until no more characters can be read.
*
* A directive that is an ordinary multibyte character is executed by reading
* the next characters of the stream.  If one of the characters differs from
* one comprising the directive, the directive fails, and the differing and
* subsequent characters remain unread.
*
* A directive that is a conversion specification defines a set of matching
* input sequences, as described below for each specifier.  A conversion
* specification is executed in the following steps:
*
* Input white-space characters (as specified by the isspace() function) are 
* skipped, unless the specification includes a `[', `c', or `n' specifier.
*
* An input item is read from the stream, unless the specification includes
* an `n' specifier.  An input item is defined as the longest matching
* sequence of input characters, unless that exceeds a specified field width,
* in which case it is the initial subsequence of that length in the
* sequence.  The first character, if any, after the input item remains
* unread.  If the length of the input item is zero, the execution of the
* directive fails:  this condition is a matching failure, unless an error
* prevented input from the stream, in which case it is an input failure.
*
* Except in the case of a `%' specifier, the input item is converted to a
* type appropriate to the conversion specifier.  If the input item is not a
* matching sequence, the execution of the directive fails:  this condition
* is a matching failure.  Unless assignment suppression was indicated by a
* `*', the result of the conversion is placed in the object pointed to by
* the first argument following the <fmt> argument that has not already
* received a conversion result.  If this object does not have an appropriate
* type, or if the result of the conversion cannot be represented in the
* space provided, the behavior is undefined.
*
* The following conversion specifiers are valid:
*
* .iP `d'
* Matches an optionally signed decimal integer whose format is
* the same as expected for the subject sequence of the strtol()
* function with the value 10 for the <base> argument.  The 
* corresponding argument should be a pointer to `int'.
* .iP `i'
* Matches an optionally signed integer, whose format is the
* same as expected for the subject sequence of the strtol()
* function with the value 0 for the <base> argument.  The 
* corresponding argument should be a pointer to `int'.
* .iP `o'
* Matches an optionally signed octal integer, whose format is the
* same as expected for the subject sequence of the strtoul()
* function with the value 8 for the <base> argument.  The
* corresponding argument should be a pointer to `unsigned int'.
* .iP `u'
* Matches an optionally signed decimal integer, whose format is 
* the same as expected for the subject sequence of the strtoul()
* function with the value 10 for the <base> argument.  The
* corresponding argument should be a pointer to `unsigned int'.
* .iP `x'
* Matches an optionally signed hexadecimal integer, whose format is
* the same as expected for the subject sequence of the strtoul()
* function with the value 16 for the <base> argument.  The
* corresponding argument should be a pointer to `unsigned int'.
* .iP "`e', `f', `g'"
* Match an optionally signed floating-point number, whose format
* is the same as expected for the subject string of the strtod()
* function.  The corresponding argument should be a pointer to `float'.
* .iP `s'
* Matches a sequence of non-white-space characters.  The 
* corresponding argument should be a pointer to the initial
* character of an array large enough to accept the sequence
* and a terminating null character, which will be added 
* automatically.
* .iP `['
* Matches a non-empty sequence of characters from a set of 
* expected characters (the `scanset').  The corresponding argument
* should be a pointer to the initial character of an array large
* enough to accept the sequence and a terminating null character,
* which is added automatically.  The conversion specifier
* includes all subsequent character in the format string, up to
* and including the matching right bracket (`]').  The characters
* between the brackets (the `scanlist') comprise the scanset,
* unless the character after the left bracket is a circumflex (`^')
* in which case the scanset contains all characters that do not
* appear in the scanlist between the circumflex and the right
* bracket.  If the conversion specifier begins with "[]" or "[^]", the
* right bracket character is in the scanlist and the next 
* right bracket character is the matching right bracket that ends
* the specification; otherwise the first right bracket character
* is the one that ends the specification.
* .iP `c'
* Matches a sequence of characters of the number specified by the
* field width (1 if no field width is present in the directive).
* The corresponding argument should be a pointer to the initial 
* character of an array large enough to accept the sequence.
* No null character is added.
* .iP `p'
* Matches an implementation-defined set of sequences, which should be
* the same as the set of sequences that may be produced by the %p
* conversion of the fprintf() function.  The corresponding argument
* should be a pointer to a pointer to `void'.  VxWorks defines its
* pointer input field to be consistent with pointers written by the
* fprintf() function ("0x" hexadecimal notation).  If the input item is
* a value converted earlier during the same program execution, the
* pointer that results should compare equal to that value; otherwise
* the behavior of the %p conversion is undefined.
* .iP `n'
* No input is consumed.  The corresponding argument should be a pointer to
* `int' into which the number of characters read from the input stream so
* far by this call to fscanf() is written.  Execution of a %n directive does
* not increment the assignment count returned when fscanf() completes
* execution.
* .iP `%'
* Matches a single `%'; no conversion or assignment occurs.  The
* complete conversion specification is %%.
* .LP
*
* If a conversion specification is invalid, the behavior is undefined.
*
* The conversion specifiers `E', `G', and `X' are also valid and behave the
* same as `e', `g', and `x', respectively.
*
* If end-of-file is encountered during input, conversion is terminated.  If 
* end-of-file occurs before any characters matching the current directive
* have been read (other than leading white space, where permitted), execution
* of the current directive terminates with an input failure; otherwise, unless
* execution of the current directive is terminated with a matching failure,
* execution of the following directive (if any) is terminated with an input
* failure.
*
* If conversion terminates on a conflicting input character, the offending
* input character is left unread in the input stream.  Trailing white space
* (including new-line characters) is left unread unless matched by a
* directive.  The success of literal matches and suppressed assignments is
* not directly determinable other than via the %n directive.
*
* INCLUDE FILES: stdio.h 
*
* RETURNS:
* The number of input items assigned, which can be fewer than provided for,
* or even zero, in the event of an early matching failure; or EOF if an
* input failure occurs before any conversion.
*
* SEE ALSO: scanf(), sscanf()
*/

int fscanf
    (
    FILE *	  fp,	/* stream to read from */
    char const *  fmt,	/* format string */
    ...			/* arguments to format string */
    ) 
    {
    int     nArgs;
    int     unget;
    va_list vaList;	/* vararg list */

    if (OBJ_VERIFY (fp, fpClassId) != OK)
	return (EOF);

    va_start (vaList, fmt);
    nArgs = fioScanV (fmt, fgetc, (int) fp, &unget, vaList);
    va_end (vaList);

    if (unget != -1)
	ungetc (unget, fp);

    return (nArgs);
    }

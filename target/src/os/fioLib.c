/* fioLib.c - formatted I/O library */

/* Copyright 1984-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
06c,24jul02,tpr  Remove sarcastic comment of fioFormatV.
06b,14mar02,pfl  changed long long arg support to use 'll' format
06a,21dec01,an   enabled long long arg support for printf/scanf using 'L' format flag 
05z,17dec01,max  Simple change just to make WRS doctool happy
05y,20jun01,kab  Code review cleanup
05x,19apr01,max  printf & scanf support for ALTIVEC vector data types
05w,14jun99,pai  removed blocking call to printErr() in printExc() (SPR 22735).
05v,14mar99,jdi  doc: removed refs to config.h and/or configAll.h (SPR 25663).
05v,05sep97,dgp  doc: fix SPR 9258, remove NOTE from sprintf()
05u,04oct95,kvk  fix for PPC architecture varargs float problem.
05t,21sep95,jdi  doc tweak of man page for fioLibInit().
05s,14may95,p_m  added fioLibInit().
05r,03feb95,jdi  doc format tweaks.
05q,24jan95,rhp  doc: avoid 'L' in printf() and sscanf(), no long doubles 
                 in VxWorks (see SPR#3886)
05p,13jan95,rhp  make self-refs in sscanf man page refer to sscanf, not fscanf
05o,12jan95,rhp  fix RETURNS section of fioRdString() man page (SPR#2511)
05n,21nov94,kdl  made scanCharSet() NOMANUAL.
05m,19jul94,dvs  doc tweak - added stdio.h to list of include files (SPR #2391).
05l,02dec93,pad  temporary fixed fioFormatV() to avoid a cc29k problem.
05k,17aug94,ism  fixed problem with assignment suppression (SPR#2562)
			-fixed problem with [] not failing when no characters are scanned
			-(SPR#3561)
05j,21oct93,jmm  fixed return value for scanCharSet
05i,10mar93,jdi  more documentation cleanup for 5.1.
05h,22jan93,jdi  documentation cleanup for 5.1.
05g,13nov92,dnw  added include of taskLib.h
05f,29oct92,jcf  bumped BUF to 400 to format real big numbers.
05e,01oct92,jcf  fixed printExc().
05d,02aug92,jcf  moved printExc() to here.
05c,30jul92,kdl  prevent printing leading zeroes for "Inf", "Nan".
05b,30jul92,jcf  redone for the new stdio library.
           +smb
05a,26may92,rrr  the tree shuffle
04z,10feb92,kdl  fixed scanf %n problems (SPR 1172).
04y,29jan92,kdl  made fioRdString return EOF in case of read error (SPR 1142);
		 changed copyright date.
04x,10dec91,gae  removed some ANSI warnings.
04w,04dec91,rrr  removed VARARG_OK, no longer needed with ansi c.
04v,19nov91,rrr  shut up some ansi warnings.
04u,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -changed VOID to void
		  -changed copyright notice
04t,14jul91,del  removed va_end in fioScanV.
04s,09jun91,del  integrated Intel's mods to interface to floatLib.c.
04r,18may91,gae  fixed sscanf/varargs for 960.
		 redid conditionals with VARARG_OK.
04q,05may91,jdi	 documentation tweak.
04p,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by dnw.
04o,24mar91,del  made functions work with gnu960 tools for I960 architecture.
04n,06feb91,jaa	 documentation cleanup.
04m,05oct90,dnw  documentation
04l,18sep90,kdl  removed forward declarations for digtobin and fioToUpper.
04k,08aug90,dnw  changed to call floatFormat with &vaList instead of vaList.
		 changed to call floatFormat with sizeof(buffer)-prefix instead
		   of sizeof(buffer)
04j,04jun90,dnw  extensive rewrite/reorg of printf/scanf family of rtns:
		   - consolidation and clean-up,
		   - adherence to ANSI definitions for format semantics,
		   - use varargs correctly,
		   - removed all floating pt refs.
		 BUG FIXES:
		   spr 501: fixed printf precision bug
		   spr 640: printf, fdprintf, printErr return num chars printed
		   spr 754: vararg routines no longer limited to 16 args
		   spr 766: deleted obsolete fioStd{In,Out,Err} routines
		   spr 767: scanf, sscanf, fscanf return EOF properly
		   unreported: printf with both "left justify" & "0 fill" flags
			   would 0 fill to the right! ("5000" instead of "5   ")
		 ANSI STANDARD CHANGES, *NOT* COMPATIBLE WITH 4.0.2:
		   - scanf %EFG now same as %efg (no longer means "double"!)
		   - sprintf returns number of chars in buffer NOT INCLUDING
		       EOS (4.0.2 included EOS in count)
		   - printf precision parameter only has effect on %s and %efg
		 ANSI STANDARD CHANGES, BACKWARD COMPATIBLE:
		   - added vprintf, vfprintf, vsprintf vararg routines
		   - printf now supports %i %p %n
		   - scanf now supports %i %p %n %% %X
		   - scanf now allows optional leading "0x"
		 OTHER CHANGES:
		   - deleted unused internal routines fioToUpper() & digtobin()
		   - fioRead() doesn't set S_fioLib_UNEXPECTED_EOF anymore
		   - changes to allow void be defined as void
04i,07mar90,jdi  documentation cleanup.
04h,09aug89,gae  did varargs stuff right.
    06jul89,ecs  updated to track 4.0.2.
    07sep88,ecs  varargs.
		 added include of varargs.h.
04g,19aug88,gae  documentation.
04f,13aug88,gae  fixed bug in convert() for E and F.
04e,30jun88,gae  fixed bug in sscanf () and convert() for number specifier.
04d,30may88,dnw  changed to v4 names.
04c,28may88,dnw  removed NOT_GENERIC stuff.
		 removed obsolete rtns: bcdtoi,itobcd,atoi,atow,atos,bprintf
		 removed obsolete: fioSet{In,Out,Err},
		 removed skipHexSpecifier to bootConfig.
		 removed skipSpace to bootConfig and bootLib.
		 made itob NOMANUAL.
		 made digtobin LOCAL.
04b,21apr88,gae  added conversion of 'g' for scanf().
04a,02apr88,gae  rewrote convert and support routines to work with stdio.
		 changed fprintf() to fdprintf().
		 changed fio{Std,Set}{In,Out,Err}() to use new 0,1,2 I/O scheme.
		 made putbuf() local.
03i,16mar88,gae  fixed bug in convert() to return FALSE for invalid specifier.
	         fixed fatal bug in atos() of not handling unterminated table.
		 made atos() return ERROR when given invalid table.
		 sscanf can now do flag:       "<*>".
		 sscanf can now do specifiers: "E|F|c|u|o".
		 printf can now do flags:      "<+><sp><#>".
		 printf can now do specifiers: "u|E|G|X".
		 added nindex() and fioToUpper().
03h,16feb88,llk  fixed sscanf so that it handles double precision floats.
03g,10dec87,gae  changed some VARARGS routines to have standard "args"
		   parameter for parsing by C interface generator.
03f,16nov87,jcf  added skipHexSpecifier to aid in hex parsing
03e,05nov87,jlf  documentation
03d,12oct87,jlf  added [...] strings to scanf.
03c,01aug87,gae  moved floating-point support to fltLib.c.
		 added fioFltInstall().
03b,31jul87,gae  replaced homebrew atof() with the real thing.
		 renamed gcvtb() to gcvt().  fixed bug in ftob().
03a,07may87,jlf  added floating point support, with #ifdef's.
	   +gae	 This involved: %e and %f to format() and convert().
		 New routines: ecvtb, fcvtb, gcvt, atof, atoe, ftob, etob,
		 frexp, ldexp, and modf.
02x,04apr87,dnw  fixed bug in format() that limited fields to 128 characters.
02w,23mar87,jlf  documentation.
02v,17jan87,gae	 made timeConvert and format handle 12 hour clock time code.
02u,20dec86,dnw  changed to not get include files from default directories.
02t,10oct86,dnw  documentation.
02s,04sep86,jlf  documentation.
02r,29jul86,llk  documentation changes
02q,27jul86,llk  added standard error file descriptor (std_err)
		 added fioSetErr, fioStdErr, printErr
02p,01jul86,jlf  minor documentation
02o,27mar86,ecs  changed timeConvert to check for legality of time entered.
		 changed timeDecode to return integer rather than bcd.
02n,11mar86,jlf  changed GENERIC stuff to NOT_GENERIC.
*/

/*
DESCRIPTION
This library provides the basic formatting and scanning I/O functions.  It
includes some routines from the ANSI-compliant printf()/scanf()
family of routines.  It also includes several utility routines.

If the floating-point format specifications `e', `E', `f', `g', and `G' are
to be used with these routines, the routine floatInit() must be called
first.  If the configuration macro INCLUDE_FLOATING_POINT is defined,
floatInit() is called by the root task, usrRoot(), in usrConfig.c.

These routines do not use the buffered I/O facilities provided by the
standard I/O facility.  Thus, they can be invoked even if the standard I/O
package has not been included.  This includes printf(), which in most UNIX
systems is part of the buffered standard I/O facilities.  Because printf()
is so commonly used, it has been implemented as an unbuffered I/O function.
This allows minimal formatted I/O to be achieved without the overhead of
the entire standard I/O package.  For more information, see the manual
entry for ansiStdio.

INCLUDE FILES: fioLib.h, stdio.h

SEE ALSO: ansiStdio, floatLib,
.pG "I/O System"
*/

#include "vxWorks.h"
#include "ctype.h"
#include "errno.h"
#include "ioLib.h"
#include "string.h"
#include "stdarg.h"
#include "limits.h"
#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "fioLib.h"
#include "intLib.h"
#include "sysLib.h"
#include "qLib.h"
#include "taskLib.h"
#include "private/kernelLibP.h"
#include "private/vmLibP.h"
#include "private/floatioP.h"

/* Macros for converting digits to letters and vice versa */

#define	BUF		400		/* buffer for %dfg etc */
#define	PADSIZE		16		/* pad chunk size */

#define	to_digit(c)	((c) - '0')
#define is_digit(c)	((unsigned)to_digit(c) <= 9)
#define	to_char(n)	((n) + '0')

#define	PAD(howmany, with) 					\
    { 								\
    if ((n = (howmany)) > 0) 					\
	{ 							\
	while (n > PADSIZE)					\
	    {							\
	    if ((*outRoutine) (with, PADSIZE, outarg) != OK)	\
		return (ERROR);					\
	    n -= PADSIZE; 					\
	    }							\
	if ((*outRoutine) (with, n, outarg) != OK)		\
	    return (ERROR);					\
	}							\
    }

/* to extend shorts, signed and unsigned arg extraction methods are needed */
#define	SARG()	((doLongLongInt) ? (long long) va_arg(vaList, long long) : \
		 (doLongInt) ? (long long)(long)va_arg(vaList, long) : \
		 (doShortInt) ? (long long)(short)va_arg(vaList, int) : \
		 (long long)(int) va_arg(vaList, int))

#define	UARG()	((doLongLongInt) ? (unsigned long long) va_arg(vaList, unsigned long long) : 	\
	 (doLongInt) ? (unsigned long long)(ulong_t)va_arg(vaList,ulong_t):\
	 (doShortInt) ? (unsigned long long)(ushort_t)va_arg(vaList,int):\
	 (unsigned long long)(uint_t) va_arg(vaList, uint_t))


#define GET_CHAR(ch, ix)        ((ix)++, (ch) = (* getRtn) (getArg, -1))

#define ll 		1		/* long long format */ 

/* globals */

/* The fieldSzIncludeSign indicates whether the sign should be included
 * in the precision of a number.
 */

BOOL fieldSzIncludeSign = TRUE;


/* locals */

LOCAL FUNCPTR fioFltFormatRtn;
LOCAL FUNCPTR fioFltScanRtn;

/* Choose PADSIZE to trade efficiency vs size.  If larger printf fields occur
 * frequently, increase PADSIZE (and make the initialisers below longer).
 */

LOCAL char blanks[PADSIZE] =
    {
    ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '
    };

LOCAL char zeroes[PADSIZE] =
    {
    '0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0'
    };


/* forwards */

LOCAL int    getbuf (char ** str);
LOCAL STATUS putbuf (char *inbuf, int length, char ** outptr);
LOCAL STATUS printbuf (char *buf, int nbytes, int fd);

/*LOCAL*/ BOOL scanField (const char **ppFmt, void *pReturn, FUNCPTR getRtn, 
		      int getArg, BOOL *pSuppress, int *pCh, int *pNchars);
LOCAL BOOL scanChar (char *pResult, int fieldwidth, FUNCPTR getRtn, 
		     int getArg, int *pCh, int *pNchars);
LOCAL BOOL scanString (char *pResult, int fieldwidth, FUNCPTR getRtn, 
		       int getArg, int *pCh, int *pNchars);
/*LOCAL*/ BOOL scanCharSet (char *pResult, int fieldwidth, 
			const char *charsAllowed, FUNCPTR getRtn, 
			int getArg, int *pCh, int *pNchars);
LOCAL BOOL scanNum (int base, void *pReturn, int returnSpec, int fieldwidth,
		    FUNCPTR getRtn, int getArg, int *pCh, int *pNchars);

/*******************************************************************************
*
* fioLibInit - initialize the formatted I/O support library
*
* This routine initializes the formatted I/O support library.  It should be
* called once in usrRoot() when formatted I/O functions such as printf() and
* scanf() are used.
*
* RETURNS: N/A
*/

void fioLibInit (void)

    {
    _func_printErr = (FUNCPTR) printErr;
    }

/*******************************************************************************
*
* fioFltInstall - install routines to print and scan floating-point numbers
*
* This routine is a hook for floatLib to install its special floating-point
* formatting & scanning routines.  It should only be called by floatInit().
*
* NOMANUAL
*/

void fioFltInstall
    (
    FUNCPTR formatRtn,          /* routine to format floats for output */
    FUNCPTR scanRtn             /* routine to scan input for floats */
    )
    {
    fioFltFormatRtn = formatRtn;
    fioFltScanRtn   = scanRtn;
    }

/*******************************************************************************
*
* printf - write a formatted string to the standard output stream (ANSI)
*
* This routine writes output to standard output under control of the string
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
* promotions, and its value converted to `short int' or `unsigned short int'
* before printing); an optional `h' specifying that a following `n' 
* conversion specifier applies to a pointer to a `short int' argument.  An
* optional `l' (ell) specifying that a following `d', `i', `o', `u', `x', and
* `X' conversion specifier applies to a `long int' or `unsigned long int'
* argument; or an optional `l' specifying that a following `n' conversion
* specifier applies to a pointer to a `long int' argument.  An optional `ll' 
* (ell-ell) specifying that a following `d', `i', `o', `u', `x', and
* `X' conversion specifier applies to a `long long int' or `unsigned long 
* long int' argument; or an optional `ll' specifying that a following `n' 
* conversion specifier applies to a pointer to a `long long int' argument.  
* If a `h', `l' or `ll' appears with any other conversion specifier, the 
* behavior is undefined.
* .iP
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
* "0X") prefixed to it.  For `e', `E', `f', `g', and `g' conversions, the
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
* INCLUDE FILES: fioLib.h 
*
* RETURNS:
* The number of characters written, or a negative value if an
* output error occurs.
*
* SEE ALSO: fprintf(),
* .I "American National Standard for Information Systems -"
* .I "Programming Language - C, ANSI X3.159-1989: Input/Output (stdio.h)"
*
* VARARGS1
*/

int printf
    (
    const char *  fmt,	/* format string to write */
    ...      		/* optional arguments to format string */
    )
    {
    va_list vaList;	/* traverses argument list */
    int nChars;

    va_start (vaList, fmt);
    nChars = fioFormatV (fmt, vaList, printbuf, 1);
    va_end (vaList);

    return (nChars);
    }

/*******************************************************************************
*
* printErr - write a formatted string to the standard error stream
*
* This routine writes a formatted string to standard error.  Its function and
* syntax are otherwise identical to printf().
*
* RETURNS: The number of characters output, or ERROR if there is an error
* during output.
*
* SEE ALSO: printf()
*
* VARARGS1
*/

int printErr
    (
    const char *  fmt, 	/* format string to write */
    ...      		/* optional arguments to format */
    )
    {
    va_list vaList;	/* traverses argument list */
    int nChars;

    va_start (vaList, fmt);
    nChars = fioFormatV (fmt, vaList, printbuf, 2);
    va_end (vaList);

    return (nChars);
    }

/*******************************************************************************
*
* printExc - print error message
*
* If at interrupt level or other invalid/fatal state, then "print"
* into System Exception Message area.
*
* NOMANUAL
*/

void printExc (fmt, arg1, arg2, arg3, arg4, arg5)
    char *fmt;	/* format string */
    int arg1;
    int arg2;
    int arg3;
    int arg4;
    int arg5;

    {
    UINT	state;
    int		pageSize;
    char *	pageAddr;

    if ((INT_CONTEXT ()) || (Q_FIRST (&activeQHead) == NULL))
	{
	/* Exception happened during exception processing, or before
	 * any task could be initialized. Tack the message onto the end
	 * of a well-known location. 
	 */

	/* see if we need to write enable the memory */

	if (vmLibInfo.vmLibInstalled)
	    {
	    pageSize = VM_PAGE_SIZE_GET();
	    pageAddr = (char *) ((UINT) sysExcMsg / pageSize * pageSize);

	    if ((VM_STATE_GET (NULL, (void *) pageAddr, &state) != ERROR) &&
		((state & VM_STATE_MASK_WRITABLE) == VM_STATE_WRITABLE_NOT))
		{
		TASK_SAFE();			/* safe from deletion */

		VM_STATE_SET (NULL, pageAddr, pageSize, VM_STATE_MASK_WRITABLE,
			      VM_STATE_WRITABLE);

		sysExcMsg += sprintf (sysExcMsg,fmt,arg1,arg2,arg3,arg4,arg5);

		VM_STATE_SET (NULL, pageAddr, pageSize, VM_STATE_MASK_WRITABLE,
			      VM_STATE_WRITABLE_NOT);

		TASK_UNSAFE();			/* unsafe from deletion */
		return;
		}
	    }

	sysExcMsg += sprintf (sysExcMsg, fmt, arg1, arg2, arg3, arg4, arg5);
	}
    else
        {
        /* queue printErr() so that we do not block here (SPR 22735) */

        excJobAdd ((VOIDFUNCPTR)printErr, (int)fmt, arg1,arg2,arg3,arg4,arg5);
        }
    }

/*******************************************************************************
*
* fdprintf - write a formatted string to a file descriptor
*
* This routine writes a formatted string to a specified file descriptor.  Its
* function and syntax are otherwise identical to printf().
*
* RETURNS: The number of characters output, or ERROR if there is an error
* during output.
*
* SEE ALSO: printf()
*
* VARARGS2
*/

int fdprintf
    (
    int fd,		/* file descriptor to write to */
    const char *  fmt,	/* format string to write */
    ...			/* optional arguments to format */
    )
    {
    va_list vaList;	/* traverses argument list */
    int nChars;

    va_start (vaList, fmt);
    nChars = fioFormatV (fmt, vaList, printbuf, fd);
    va_end (vaList);

    return (nChars);
    }

/*******************************************************************************
*
* sprintf - write a formatted string to a buffer (ANSI)
*
* This routine copies a formatted string to a specified buffer, which is
* null-terminated.  Its function and syntax are otherwise identical
* to printf().
*
* RETURNS:
* The number of characters copied to <buffer>, not including the NULL 
* terminator.
*
* SEE ALSO: printf(),
* .I "American National Standard for Information Systems -"
* .I "Programming Language - C, ANSI X3.159-1989: Input/Output (stdio.h)"
*
* VARARGS2
*/

int sprintf
    (
    char *  buffer,	/* buffer to write to */
    const char *  fmt,	/* format string */
    ...			/* optional arguments to format */
    )
    {
    va_list	vaList;		/* traverses argument list */
    int		nChars;

    va_start (vaList, fmt);
    nChars = fioFormatV (fmt, vaList, putbuf, (int) &buffer);
    va_end (vaList);

    *buffer = EOS;

    return (nChars);
    }

/*******************************************************************************
*
* vprintf - write a string formatted with a variable argument list to standard output (ANSI)
*
* This routine prints a string formatted with a variable argument list to
* standard output.  It is identical to printf(), except that it takes
* the variable arguments to be formatted as a list <vaList> of type `va_list'
* rather than as in-line arguments.
*
* RETURNS: The number of characters output, or ERROR if there is an error
* during output.
*
* SEE ALSO: printf(),
* .I "American National Standard for Information Systems -"
* .I "Programming Language - C, ANSI X3.159-1989: Input/Output (stdio.h)"
*/

int vprintf
    (
    const char *  fmt,		/* format string to write */
    va_list	  vaList	/* arguments to format */
    )
    {
    return (fioFormatV (fmt, vaList, printbuf, 1));
    }
/*******************************************************************************
*
* vfdprintf - write a string formatted with a variable argument list to a file descriptor
*
* This routine prints a string formatted with a variable argument list to a
* specified file descriptor.  It is identical to fdprintf(), except
* that it takes the variable arguments to be formatted as a list <vaList> of
* type `va_list' rather than as in-line arguments.
*
* RETURNS: The number of characters output, or ERROR if there is an error
* during output.
*
* SEE ALSO: fdprintf()
*/

int vfdprintf
    (
    int		  fd,		/* file descriptor to print to */
    const char *  fmt,		/* format string for print */
    va_list	  vaList	/* optional arguments to format */
    )
    {
    return (fioFormatV (fmt, vaList, printbuf, fd));
    }
/*******************************************************************************
*
* vsprintf - write a string formatted with a variable argument list to a buffer (ANSI)
*
* This routine copies a string formatted with a variable argument list to
* a specified buffer.  This routine is identical to sprintf(), except that it
* takes the variable arguments to be formatted as a list <vaList> of type
* `va_list' rather than as in-line arguments.
*
* RETURNS: 
* The number of characters copied to <buffer>, not including the NULL 
* terminator.
*
* SEE ALSO: sprintf(),
* .I "American National Standard for Information Systems -"
* .I "Programming Language - C, ANSI X3.159-1989: Input/Output (stdio.h)"
*/

int vsprintf
    (
    char *	  buffer,	/* buffer to write to */
    const char *  fmt,		/* format string */
    va_list	  vaList	/* optional arguments to format */
    )
    {
    int nChars;

    nChars = fioFormatV (fmt, vaList, putbuf, (int) &buffer);
    *buffer = EOS;

    return (nChars);
    }

#ifdef _WRS_ALTIVEC_SUPPORT
typedef union {
      __vector unsigned long 	vul;
      float 			f32[4];
      unsigned long 		u32[4];
      unsigned short 		u16[8];
      unsigned char 		u8[16];
      signed long 		s32[4];
      signed short 		s16[8];
      signed char 		s8[16];
    } VECTOR;
#endif /* _WRS_ALTIVEC_SUPPORT */
/*******************************************************************************
*
* fioFormatV - convert a format string
*
* This routine is used by the printf() family of routines to handle the
* actual conversion of a format string.  The first argument is a format
* string, as described in the entry for printf().  The second argument is a
* variable argument list <vaList> that was previously established.
*
* As the format string is processed, the result will be passed to the output
* routine whose address is passed as the third parameter, <outRoutine>.
* This output routine may output the result to a device, or put it in a
* buffer.  In addition to the buffer and length to output, the fourth
* argument, <outarg>, will be passed through as the third parameter to the
* output routine.  This parameter could be a file descriptor, a buffer 
* address, or any other value that can be passed in an "int".
*
* The output routine should be declared as follows:
* .CS
*     STATUS outRoutine
*         (
*         char *buffer, /@ buffer passed to routine            @/
*         int  nchars,  /@ length of buffer                    @/
*         int  outarg   /@ arbitrary arg passed to fmt routine @/
*         )
* .CE
* The output routine should return OK if successful, or ERROR if unsuccessful.
*
* RETURNS:
* The number of characters output, or ERROR if the output routine 
* returned ERROR.
*
* INTERNAL
* Warning, this routine is extremely complex and its integrity is easily
* destroyed. Do not change this code without absolute understanding of all
* ramifications and consequences. 
*/
int fioFormatV
    (
    FAST const char *fmt, 	/* format string */
    va_list	vaList,       	/* pointer to varargs list */
    FUNCPTR	outRoutine,   	/* handler for args as they're formatted */
    int		outarg          /* argument to routine */
    )
    {
    FAST int	ch;		/* character from fmt */
    FAST int	n;		/* handy integer (short term usage) */
    FAST char *	cp;		/* handy char pointer (short term usage) */
    int		width;		/* width from format (%8d), or 0 */
    char	sign;		/* sign prefix (' ', '+', '-', or \0) */
    unsigned long long	
	        ulongLongVal; 	/* unsigned 64 bit arguments %[diouxX] */
    int		prec;		/* precision from format (%.3d), or -1 */
    int		oldprec;	/* old precision from format (%.3d), or -1 */
    int		dprec;		/* a copy of prec if [diouxX], 0 otherwise */
    int		fpprec;		/* `extra' floating precision in [eEfgG] */
    int		size;		/* size of converted field or string */
    int		fieldsz;	/* field size expanded by sign, etc */
    int		realsz;		/* field size expanded by dprec */

#ifdef _WRS_ALTIVEC_SUPPORT
    FAST int	i;		/* handy integer (short term usage) */
    FAST char *	vp;		/* handy char pointer (short term usage) */
    char	Separator;	/* separator for vector elements */
    char	C_Separator;	/* separator for char vector elements */
    VECTOR	vec;		/* vector argument */
    BOOL	doVector;	/* AltiVec vector */
#endif /* _WRS_ALTIVEC_SUPPORT */
    char	FMT[20];	/* To collect fmt info */
    FAST char *	Collect;	/* char pointer to FMT */

    BOOL	doLongInt;	/* long integer */
    BOOL	doLongLongInt;	/* long long integer - 64 bit */
    BOOL	doShortInt;	/* short integer */
    BOOL	doAlt;		/* alternate form */
    BOOL	doLAdjust;	/* left adjustment */
    BOOL	doZeroPad;	/* zero (as opposed to blank) pad */
    BOOL	doHexPrefix;	/* add 0x or 0X prefix */
    BOOL	doSign;		/* change sign to '-' */
    char	buf[BUF];	/* space for %c, %[diouxX], %[eEfgG] */
#if (CPU_FAMILY != AM29XXX)
    char	ox[2];		/* space for 0x hex-prefix */
#else
    char	ox[3];		/* space for 0x hex-prefix */
#endif /*(CPU_FAMILY != AM29XXX)*/
    char *	xdigs = NULL;	/* digits for [xX] conversion */
    int		ret = 0;	/* return value accumulator */
    enum {OCT, DEC, HEX} base;	/* base for [diouxX] conversion */


    FOREVER		/* Scan the format for conversions (`%' character) */
	{
	for (cp = CHAR_FROM_CONST(fmt);((ch=*fmt) != EOS)&&(ch != '%'); fmt++)
	    ;

	if ((n = fmt - cp) != 0)
	    {
	    if ((*outRoutine) (cp, n, outarg) != OK)
		return (ERROR);

	    ret += n;
	    }

	if (ch == EOS)
	    return (ret);		/* return total length */

	fmt++;				/* skip over '%' */
#ifdef _WRS_ALTIVEC_SUPPORT
	Separator	= ' ';		/* the default separator for vector elements */
	C_Separator	= EOS;		/* the default separator for char vector elements */
	doVector	= FALSE;	/* no vector modifier */
#endif /* _WRS_ALTIVEC_SUPPORT */
	
	*FMT		= EOS;
	Collect		= FMT;
	doLongInt	= FALSE;	/* long integer */
	doLongLongInt	= FALSE;	/* 64 bit integer */
	doShortInt	= FALSE;	/* short integer */
	doAlt		= FALSE;	/* alternate form */
	doLAdjust	= FALSE;	/* left adjustment */
	doZeroPad	= FALSE;	/* zero (as opposed to blank) pad */
	doHexPrefix	= FALSE;	/* add 0x or 0X prefix */
	doSign		= FALSE;	/* change sign to '-' */
	dprec		= 0;
	fpprec		= 0;
	width		= 0;
	prec		= -1;
	oldprec		= -1;
	sign		= EOS;

#define get_CHAR  (ch = *Collect++ = *fmt++)

#ifdef _WRS_ALTIVEC_SUPPORT
#define SET_VECTOR_FMT(VFMT,NO)						\
  do									\
    {									\
	char * to;							\
									\
	vec.vul =  va_arg (vaList,__vector unsigned long);		\
	cp = buf;							\
									\
	*Collect = EOS;							\
	i = (NO);							\
	to = VFMT = (char *) malloc (i*20);				\
	while(i-- > 0) {						\
									\
		char * from = FMT;					\
									\
		*to++ = '%';						\
		while (*to++ = *from++);				\
		*(char*)((int)(to)-1) = Separator;			\
	}								\
	*(--to) = EOS;							\
    }									\
  while (0)

#define RESET_VECTOR_FMT(VFMT)			\
						\
			size = strlen(cp);	\
			sign = EOS;		\
						\
			free(VFMT)
#endif /* _WRS_ALTIVEC_SUPPORT */

rflag:	
get_CHAR;
reswitch:	
	switch (ch) 
	    {
	    case ' ':
		    /* If the space and + flags both appear, the space
		     * flag will be ignored. -- ANSI X3J11
		     */
		    if (!sign)
			sign = ' ';
		    goto rflag;
#ifdef _WRS_ALTIVEC_SUPPORT
	    case ',':
	    case ';':
	    case ':':
	    case '_':
		    Collect--;
	    	    Separator = C_Separator = ch;
		    goto rflag;
#endif /* _WRS_ALTIVEC_SUPPORT */
	    case '#':
		    doAlt = TRUE;
		    goto rflag;

	    case '*':
		    /* A negative field width argument is taken as a
		     * flag followed by a positive field width.
		     *	-- ANSI X3J11
		     * They don't exclude field widths read from args.
		     */
		    if ((width = va_arg(vaList, int)) >= 0)
			goto rflag;

		    width = -width;			/* FALLTHROUGH */

	    case '-':
		    doLAdjust = TRUE;
		    goto rflag;

	    case '+':
		    sign = '+';
		    goto rflag;

	    case '.':
		    get_CHAR;
		    if ( ch == '*' ) 
			{
			n = va_arg(vaList, int);
			prec = (n < 0) ? -1 : n;
			goto rflag;
			}

		    n = 0;
		    while (is_digit(ch)) 
			{
			n = 10 * n + to_digit(ch);
			get_CHAR;
			}
		    prec = n < 0 ? -1 : n;
		    goto reswitch;

	    case '0':
		    /* Note that 0 is taken as a flag, not as the
		     * beginning of a field width. -- ANSI X3J11
		     */
		    doZeroPad = TRUE;
		    goto rflag;

	    case '1':
	    case '2':
	    case '3':
	    case '4':
	    case '5':
	    case '6':
	    case '7':
	    case '8':
	    case '9':
		    n = 0;
		    do 
			{
			n = 10 * n + to_digit(ch);
			get_CHAR;
			} while (is_digit(ch));

		    width = n;
		    goto reswitch;

	    case 'h':
		    doShortInt = TRUE;
		    goto rflag;

	    case 'l':
		    get_CHAR;
		    if ( ch == 'l' )
			{
			doLongLongInt = TRUE;
			goto rflag;
			}
		    else
			{
		        doLongInt = TRUE;
		        goto reswitch;	
			}

#ifdef _WRS_ALTIVEC_SUPPORT
	    case 'v':
		    Collect--;
		    doVector = TRUE;
		    goto rflag;
#endif /* _WRS_ALTIVEC_SUPPORT */

	    case 'c':
#ifdef _WRS_ALTIVEC_SUPPORT
		    if (doVector)
		    {
			vec.vul =  va_arg (vaList,__vector unsigned long);	
			cp = buf;
			vp = (unsigned char *)&vec.u8;
			i = 15;
			
			while(i-- > 0) {
			
				*cp++ = *vp++;
				if (C_Separator) *cp++ = C_Separator;
			}
			
			*cp++ = *vp++;

			cp = buf;
			size = 16 + (C_Separator ? 15:0);
			sign = EOS;
		    }
		    else 
		    {
#endif /* _WRS_ALTIVEC_SUPPORT */
			*(cp = buf) = va_arg(vaList, int);
			size = 1;
			sign = EOS;
#ifdef _WRS_ALTIVEC_SUPPORT 
		    }
#endif /* _WRS_ALTIVEC_SUPPORT */
		    break;

	    case 'D':
		    doLongInt = TRUE; 			/* FALLTHROUGH */

	    case 'd':
	    case 'i':
#ifdef _WRS_ALTIVEC_SUPPORT
		    if (doVector)
		    {
			SET_VECTOR_FMT(vp,doShortInt?8:4);

			if (doShortInt)
				sprintf(cp,vp,vec.s16[0],vec.s16[1],vec.s16[2],vec.s16[3],
					      vec.s16[4],vec.s16[5],vec.s16[6],vec.s16[7]);
			else
				sprintf(cp,vp,vec.s32[0],vec.s32[1],vec.s32[2],vec.s32[3]);
			
			RESET_VECTOR_FMT(vp);
			break;
		    }
#endif /* _WRS_ALTIVEC_SUPPORT */

		    ulongLongVal = SARG();
		    if ((long long)ulongLongVal < 0) 
			{
			ulongLongVal = -ulongLongVal;
			sign = '-';
			}
		    base = DEC;
		    goto number;

	    case 'n':
		    /* ret is int, so effectively %lln = %ln */
		    if (doLongLongInt)
			*va_arg(vaList, long long *) = (long long) ret;
		    else if (doLongInt)
			*va_arg(vaList, long *) = ret;
		    else if (doShortInt)
			*va_arg(vaList, short *) = ret;
		    else
			*va_arg(vaList, int *) = ret;
		    continue;				/* no output */

	    case 'O':
		    doLongInt = TRUE;			/* FALLTHROUGH */

	    case 'o':
#ifdef _WRS_ALTIVEC_SUPPORT
		    if (doVector)
		    {
			SET_VECTOR_FMT(vp,doShortInt?8:4);

			if (doShortInt)
				sprintf(cp,vp,vec.s16[0],vec.s16[1],vec.s16[2],vec.s16[3],
					      vec.s16[4],vec.s16[5],vec.s16[6],vec.s16[7]);
			else
				sprintf(cp,vp,vec.s32[0],vec.s32[1],vec.s32[2],vec.s32[3]);
			
			RESET_VECTOR_FMT(vp);
			break;
		    }
#endif /* _WRS_ALTIVEC_SUPPORT */
		    ulongLongVal = UARG();
		    base = OCT;
		    goto nosign;

	    case 'p':
		    /* The argument shall be a pointer to void.  The
		     * value of the pointer is converted to a sequence
		     * of printable characters, in an implementation
		     * defined manner. -- ANSI X3J11
		     */
#ifdef _WRS_ALTIVEC_SUPPORT
		    if (doVector)
		    {
			SET_VECTOR_FMT(vp,4);

			sprintf(cp,vp,vec.u32[0],vec.u32[1],vec.u32[2],vec.u32[3]);
			
			RESET_VECTOR_FMT(vp);
			break;
		    }
#endif /* _WRS_ALTIVEC_SUPPORT */
		    ulongLongVal = (unsigned long long) (unsigned int)
			           va_arg(vaList, void *);/* NOSTRICT */
		    base	= HEX;
		    xdigs	= "0123456789abcdef";
		    doHexPrefix = TRUE;
		    ch		= 'x';
		    goto nosign;

	    case 's':
		    if ((cp = va_arg(vaList, char *)) == NULL)
			cp = "(null)";

		    if (prec >= 0) 
			{
			/* can't use strlen; can only look for the
			 * NUL in the first `prec' characters, and
			 * strlen() will go further.
			 */

			char *p = (char *)memchr(cp, 0, prec);

			if (p != NULL) 
			    {
			    size = p - cp;
			    if (size > prec)
				size = prec;
			    }
			else
			    size = prec;
			}
		    else
			size = strlen(cp);

		    sign = EOS;
		    break;

	    case 'U':
		    doLongInt = TRUE;			/* FALLTHROUGH */

	    case 'u':
#ifdef _WRS_ALTIVEC_SUPPORT
		    if (doVector)
		    {
			SET_VECTOR_FMT(vp,doShortInt?8:4);

			if (doShortInt)
				sprintf(cp,vp,vec.u16[0],vec.u16[1],vec.u16[2],vec.u16[3],
					      vec.u16[4],vec.u16[5],vec.u16[6],vec.u16[7]);
			else
				sprintf(cp,vp,vec.u32[0],vec.u32[1],vec.u32[2],vec.u32[3]);
			
			RESET_VECTOR_FMT(vp);
			break;
		    }
#endif /* _WRS_ALTIVEC_SUPPORT */
		    ulongLongVal = UARG();
		    base = DEC;
		    goto nosign;

	    case 'X':
		    xdigs = "0123456789ABCDEF";
		    goto hex;

	    case 'x':
		    xdigs = "0123456789abcdef";

hex:
#ifdef _WRS_ALTIVEC_SUPPORT
		    if (doVector)
		    {
			SET_VECTOR_FMT(vp,doShortInt?8:4);

			if (doShortInt)
				sprintf(cp,vp,vec.s16[0],vec.s16[1],vec.s16[2],vec.s16[3],
					      vec.s16[4],vec.s16[5],vec.s16[6],vec.s16[7]);
			else
				sprintf(cp,vp,vec.s32[0],vec.s32[1],vec.s32[2],vec.s32[3]);
			
			RESET_VECTOR_FMT(vp);
			break;
		    }
#endif /* _WRS_ALTIVEC_SUPPORT */
		    ulongLongVal = UARG();
		    base = HEX;

		    /* leading 0x/X only if non-zero */

		    if (doAlt && (ulongLongVal != 0))
			doHexPrefix = TRUE;

		    /* unsigned conversions */
nosign:		    sign = EOS;

		    /* ... diouXx conversions ... if a precision is
		     * specified, the 0 flag will be ignored. -- ANSI X3J11
		     */

number:		    if ((dprec = prec) >= 0)
			doZeroPad = FALSE;

		    /* The result of converting a zero value with an
		     * explicit precision of zero is no characters. 
		     * -- ANSI X3J11
		     */
		    cp = buf + BUF;
		    if ((ulongLongVal != 0) || (prec != 0)) 
			{
			/* unsigned mod is hard, and unsigned mod
			 * by a constant is easier than that by
			 * a variable; hence this switch.
			 */
			switch (base) 
			    {
			    case OCT:
				do 
				    {
				    *--cp = to_char(ulongLongVal & 7);
				    ulongLongVal >>= 3;
				    } while (ulongLongVal);

				/* handle octal leading 0 */

				if (doAlt && (*cp != '0'))
				    *--cp = '0';
				break;

			    case DEC:
				/* many numbers are 1 digit */

				while (ulongLongVal >= 10) 
				    {
				    *--cp = to_char(ulongLongVal % 10);
				    ulongLongVal /= 10;
				    }

				*--cp = to_char(ulongLongVal);
				break;

			    case HEX:
				do 
				    {
				    *--cp = xdigs[ulongLongVal & 15];
				    ulongLongVal >>= 4;
				    } while (ulongLongVal);
				break;

			    default:
				cp = "bug in vfprintf: bad base";
				size = strlen(cp);
				goto skipsize;
			    }
			}

		    size = buf + BUF - cp;
skipsize:
		    break;

	    case 'L':
		    /* NOT IMPLEMENTED */
		    goto rflag;

	    case 'e':
	    case 'E':
	    case 'f':
	    case 'g':
	    case 'G':
#ifdef _WRS_ALTIVEC_SUPPORT
		    if (doVector)
		    {
			SET_VECTOR_FMT(vp,4);

			sprintf(cp,vp,vec.f32[0],vec.f32[1],vec.f32[2],vec.f32[3]);
			
			RESET_VECTOR_FMT(vp);
			break;
		    }
#endif /* _WRS_ALTIVEC_SUPPORT */
		    if (fioFltFormatRtn != NULL)
			{
			oldprec = prec;		/* in case of strange float */

			if (prec > MAXFRACT) 	/* do realistic precision */
			    {
			    if (((ch != 'g') && (ch != 'G')) || doAlt)
				fpprec = prec - MAXFRACT;
			    prec = MAXFRACT;	/* they asked for it! */
			    }
			else if (prec == -1)
			    prec = 6;		/* ANSI default precision */

			cp  = buf;		/* where to fill in result */
			*cp = EOS;		/* EOS terminate just in case */
#if ((CPU_FAMILY != I960) && (CPU_FAMILY != PPC))
			size = (*fioFltFormatRtn) (&vaList,prec,doAlt,ch,
						   &doSign,cp,buf+sizeof(buf));
#else
			size = (*fioFltFormatRtn) (vaList, prec, doAlt, ch,
						   &doSign,cp,buf+sizeof(buf));
#endif
			if ((int)size < 0)	/* strange value (Nan,Inf,..) */
			    {
			    size = -size;	/* get string length */
			    prec = oldprec;	/* old precision (not default)*/

			    doZeroPad = FALSE;	/* don't pad with zeroes */
			    if (doSign)		/* is strange value signed? */
				sign = '-';
			    }
			else
			    {
			    if (doSign)		
				sign = '-';

			    if (*cp == EOS)
				cp++;
			    }
			break;
			}
		    /* FALLTHROUGH if no floating point format routine */

	    default:			/* "%?" prints ?, unless ? is NULL */
		    if (ch == EOS)
			return (ret);

		    /* pretend it was %c with argument ch */

		    cp   = buf;
		    *cp  = ch;
		    size = 1;
		    sign = EOS;
		    break;
	    }

	/* All reasonable formats wind up here.  At this point,
	 * `cp' points to a string which (if not doLAdjust)
	 * should be padded out to `width' places.  If
	 * doZeroPad, it should first be prefixed by any
	 * sign or other prefix; otherwise, it should be blank
	 * padded before the prefix is emitted.  After any
	 * left-hand padding and prefixing, emit zeroes
	 * required by a decimal [diouxX] precision, then print
	 * the string proper, then emit zeroes required by any
	 * leftover floating precision; finally, if doLAdjust,
	 * pad with blanks.
	 */

	/*
	 * compute actual size, so we know how much to pad.
	 * fieldsz excludes decimal prec; realsz includes it
	 */

	fieldsz = size + fpprec;

	if (sign)
	    {
	    fieldsz++;
	    if (fieldSzIncludeSign)
	        dprec++; 
	    }
	else if (doHexPrefix)
	    fieldsz += 2;

	realsz = (dprec > fieldsz) ? dprec : fieldsz;

	/* right-adjusting blank padding */

	if (!doLAdjust && !doZeroPad)
	    PAD(width - realsz, blanks);

	/* prefix */

	if (sign)
	    {
	    if ((*outRoutine) (&sign, 1, outarg) != OK)
		return (ERROR);
	    }
	else if (doHexPrefix) 
	    {
	    ox[0] = '0';
	    ox[1] = ch;
	    if ((*outRoutine) (ox, 2, outarg) != OK)
		return (ERROR);
	    }

	/* right-adjusting zero padding */

	if (!doLAdjust && doZeroPad)
	    PAD(width - realsz, zeroes);

	/* leading zeroes from decimal precision */

	PAD(dprec - fieldsz, zeroes);

	/* the string or number proper */

	if ((*outRoutine) (cp, size, outarg) != OK)
	    return (ERROR);

	/* trailing floating point zeroes */

	PAD(fpprec, zeroes);

	/* left-adjusting padding (always blank) */

	if (doLAdjust)
	    PAD(width - realsz, blanks);

	/* finally, adjust ret */

	ret += (width > realsz) ? width : realsz;
	}
    }

/*******************************************************************************
*
* putbuf - put characters in a buffer
*
* This routine is a support routine for sprintf().
* This routine copies length bytes from source to destination, leaving the
* destination buffer pointer pointing at byte following block copied.
*/

LOCAL STATUS putbuf
    (
    char *inbuf,                /* pointer to source buffer */
    int length,                 /* number of bytes to copy */
    char **outptr               /* pointer to destination buffer */
    )
    {
    bcopy (inbuf, *outptr, length);
    *outptr += length;

    return (OK);
    }

/*******************************************************************************
*
* printbuf - printf() support routine: print characters in a buffer
*/

LOCAL STATUS printbuf
    (
    char *buf,
    int nbytes,
    int fd
    )
    {
    return (write (fd, buf, nbytes) == nbytes ? OK : ERROR);
    }

/*******************************************************************************
*
* fioRead - read a buffer
*
* This routine repeatedly calls the routine read() until <maxbytes> have
* been read into <buffer>.  If EOF is reached, the number of bytes read
* will be less than <maxbytes>.
*
* RETURNS:
* The number of bytes read, or ERROR if there is an error during the read
* operation.
*
* SEE ALSO: read()
*/

int fioRead
    (
    int		fd,		/* file descriptor of file to read */
    char *	buffer,		/* buffer to receive input */
    int		maxbytes	/* maximum number of bytes to read */
    )
    {
    int original_maxbytes = maxbytes;
    int nbytes;

    while (maxbytes > 0)
	{
	nbytes = read (fd, buffer, maxbytes);

	if (nbytes < 0)
	    return (ERROR);

	if (nbytes == 0)
	    return (original_maxbytes - maxbytes);

	maxbytes -= nbytes;
	buffer   += nbytes;
	}

    return (original_maxbytes);
    }

/*******************************************************************************
*
* fioRdString - read a string from a file
*
* This routine puts a line of input into <string>.  The specified input
* file descriptor is read until <maxbytes>, an EOF, an EOS, or a newline
* character is reached.  A newline character or EOF is replaced with EOS,
* unless <maxbytes> characters have been read.
*
* RETURNS:
* The length of the string read, including the terminating EOS;
* or EOF if a read error occurred or end-of-file occurred without
* reading any other character.
*/

int fioRdString
    (
    int fd,             /* fd of device to read */
    FAST char string[], /* buffer to receive input */
    int maxbytes        /* max no. of chars to read */
    )
    {
    char c;
    FAST int nchars = 0;
    FAST int i = 0;

    /* read characters until
     *    1) specified line limit is reached,
     *    2) end-of-file is reached,
     *    3) EOS character is read,
     *    4) newline character is read,
     * or 5) read error occurs.
     */

    while (i < maxbytes)
	{
	nchars = read (fd, &c, 1);
	if (nchars == ERROR)			/* if can't read file */
	    break;

	/* check for newline terminator */

	if ((nchars != 1) || (c == '\n') || (c == EOS))
	    {
	    string [i++] = EOS;
	    break;
	    }

	string [i++] = c;
	}


    if (nchars == ERROR  || ((nchars == 0) && (i == 1)))
	i = EOF;

    return (i);
    }

/*******************************************************************************
*
* sscanf - read and convert characters from an ASCII string (ANSI)
*
* This routine reads characters from the string <str>, interprets them
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
* An optional `h', `l' (ell) or `ll' (ell-ell) indicating the size of the 
* receiving object.  The conversion specifiers `d', `i', and `n' should 
* be preceded by `h' if the corresponding argument is a pointer to `short 
* int' rather than a pointer to `int', or by `l' if it is a pointer to 
* `long int', or by `ll' if it is a pointer to `long long int'.  Similarly, 
* the conversion specifiers `o', `u', and `x' shall be preceded by `h' if 
* the corresponding argument is a pointer to `unsigned short int' rather 
* than a pointer to `unsigned int', or by `l' if it is a pointer to 
* `unsigned long int', or by `ll' if it is a pointer to `unsigned long 
* long int'.  Finally, the conversion specifiers `e', `f', and `g' shall 
* be preceded by `l' if the corresponding argument is a pointer to 
* `double' rather than a pointer to `float'.  If a `h', `l' or `ll' 
* appears with any other conversion specifier, the behavior is undefined.
* .iP
* \&WARNING: ANSI C also specifies an optional `L' in some of the same
* contexts as `l' above, corresponding to a `long double *' argument.
* However, the current release of the VxWorks libraries does not support 
* `long double' data; using the optional `L' gives unpredictable results.
* .iP
* A character that specifies the type of conversion to be applied.  The
* valid conversion specifiers are described below.
* .LP
*
* The sscanf() routine executes each directive of the format in turn.  If a 
* directive fails, as detailed below, sscanf() returns.  Failures
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
* far by this call to sscanf() is written.  Execution of a %n directive does
* not increment the assignment count returned when sscanf() completes
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
* INCLUDE FILES: fioLib.h 
*
* RETURNS:
* The number of input items assigned, which can be fewer than provided for,
* or even zero, in the event of an early matching failure; or EOF if an
* input failure occurs before any conversion.
*
* SEE ALSO: fscanf(), scanf(),
* .I "American National Standard for Information Systems -"
* .I "Programming Language - C, ANSI X3.159-1989: Input/Output (stdio.h)"
*
* VARARGS2
*/

int sscanf
    (
    const char *  str,	/* string to scan */
    const char *  fmt,	/* format string */
    ...      		/* optional arguments to format string */
    )
    {
    int		nArgs;
    int		unget;
    va_list	vaList;			/* vararg list */

    va_start (vaList, fmt);
    nArgs = fioScanV (fmt, getbuf, (int) &str, &unget, vaList);
    va_end (vaList);

    return (nArgs);
    }

/********************************************************************************
* getbuf - sscanf() support routine: get next character in string
*
* RETURNS: Next character or EOF if empty.
*/

LOCAL int getbuf
    (
    char **str
    )
    {
    int c = (int)(**str);

    if (c == EOS)
        return (EOF);

    ++*str;

    return (c);
    }

/******************************************************************************
*
* fioScanV - scan processor
*
* NOMANUAL
*/

int fioScanV
    (
    const char *fmt,            /* the format string */
    FUNCPTR     getRtn,         /* routine to get next character */
    int         getArg,         /* argument to be passed to get */
    int *       pUnget,         /* where to return unget char (-1 = no unget) */
    va_list     vaList          /* vararg list */
    )
    {
    BOOL        suppress;                       /* assignment was suppressed */
    void *      pArg;
    int         ch;
    BOOL        countArg;                       /* FALSE for %n, not counted */
    BOOL        quit            = FALSE;
    int         argsAssigned    = 0;
    int         nchars          = 0;
 
    pArg = va_arg (vaList, void *);     /* get first pointer arg */
 
    ch = (* getRtn) (getArg);           /* get first char */
 
 
    while (!quit && (*fmt != EOS))
        {
        countArg = TRUE;                /* default = count arg assignment */
 
        switch (*fmt)
            {
            case ' ':
            case '\n':
            case '\t':                  /* skip white space in string */
                ++fmt;
                while (isspace (ch))
                    GET_CHAR (ch, nchars);
                break;
 
            case '%':                   /* conversion specification */
                ++fmt;
 
                /* NOTE: if next char is '%' (i.e. "%%") then a normal match
                 * of '%' is required: FALL THROUGH to default case
                 */
                if (*fmt != '%')
                    {

                    /* Skip white space except for 'c', 'n', '[' specifiers.
                     * Suppress including %n fields in returned count.
                     */
 

                    switch (*fmt)
                        {
                        case 'c':
                        case '[':
                            break;

                        case '*':  /* ignore the '*' for purposes of white
			            * space removal                        */
			    switch(*(fmt+1))
			        {
				case 'c':
				case '[':
				    break;

                        	default:
                            	    while (isspace (ch))
                                	GET_CHAR (ch, nchars);
				}
			    break;
								
                        case 'n':
                            countArg = FALSE;           /* we don't count %n */
                            break;
 
                        case 'h':                       /* %hn, %ln, %Ln, %lln */
			case 'l':
			case 'L':
			    if ((*(fmt + 1) == 'n') || 
			        ((*fmt == 'l') && (*(fmt + 1) == 'l') && (*(fmt + 2) == 'n'))) 
                                {
			        countArg = FALSE;
			        break;
			        }

                        default:
                            while (isspace (ch))
                                GET_CHAR (ch, nchars);
                        }
 
 
                    /* Quit if at end of input string, unless spec is %[hl]n */
 
                    if (ch == EOF)                      /* if at end of input */                        {
                        switch (*fmt)
                            {
                            case 'n':
                                break;                  /* %n is still valid */
 
                            case 'h':                   /* also %hn, %ln, %lln */
                            case 'l':
			    case 'L':
				if ((*(fmt + 1) == 'n') || 
			            ((*fmt == 'l') && (*(fmt + 1) == 'l') && (*(fmt + 2) == 'n'))) 
                                    {
			            break;		/* keep going */
			            }
                                /* FALL THROUGH */
 
                            default:
                                quit = TRUE;
                                break;
                            }
                        }
 
                    /* Scan field in input string */
 
                    if (scanField (&fmt, pArg, getRtn, getArg, &suppress, &ch,
                                   &nchars))
                        {
                        /* successful scan */

                        if (!suppress)
                            {
                            if (countArg)               /* unless arg was %n */
                                ++argsAssigned;         /*  increment count */
 
                            pArg = va_arg (vaList, void *);     /* next arg */
                            }
                        }
                    else
                        quit = TRUE;         	/* unsuccessful */
                    break;
                    }
 
                /* NOTE: else of above if FALLS THROUGH to default below! */
 
            default:                   		/* character to match */
                if (ch == *fmt)
                    {
                    GET_CHAR (ch, nchars);      /* advance past matched char */
                    ++fmt;                      /* advance past matched char */
                    }
                else
                    quit = TRUE;                /* match failed */
                break;
            }
        }
 
    *pUnget = ch;
 
    if ((argsAssigned == 0) && (ch == EOF))
        return (EOF);
 
    return (argsAssigned);
    }
/********************************************************************************
* scanField - assign value to argument according to format
*
* RETURNS:
* TRUE if conversion successful, otherwise FALSE;
* <pSuppress> is set TRUE if assignment was suppressed.
*
* NOMANUAL
*/
 
/*LOCAL*/ BOOL scanField
    (
    const char **ppFmt,          /* conversion specification string */
    FAST void *  pReturn,        /* where to set result */
    FUNCPTR      getRtn,
    int          getArg,
    BOOL *       pSuppress,      /* set to TRUE if argptr was not assigned */
    int *        pCh,
    int *        pNchars
    )
    {
    FAST int    fieldwidth = 0;         /* maximum number of chars to convert.
                                         * default 0 implies no limit */
    int         returnSpec = 0;         /* 0, 'h', 'l', ll */
    FAST const char *pFmt = *ppFmt;     /* ptr to current char in fmt string */
    FAST int    base;
#ifdef _WRS_ALTIVEC_SUPPORT
    VECTOR	vec;			/* vector value accumulator */
    BOOL	doVector	= FALSE;/* AltiVec vector */
    char	Separator	= ' ';	/* separator for vector elements */
    char	C_Separator	= EOS;	/* separator for char vector elements */
    FAST int    ch, ix;			/* handy variables for GET_CHAR */
    char 	*vp;			/* handy char pointer (short term usage) */
    FAST int	i=4;			/* handy integer (short term usage) */

    while(i--) vec.s32[i] = 0;
#endif /* _WRS_ALTIVEC_SUPPORT */
    /* check for assignment suppression */

    *pSuppress = (*pFmt == '*');
    if (*pSuppress)
        {
        ++pFmt;
        pReturn = NULL;
        }
 
    /* check for specification of maximum fieldwidth */
 
    while (isdigit ((int)*pFmt))
        fieldwidth = 10 * fieldwidth + (*pFmt++ - '0');

#ifdef _WRS_ALTIVEC_SUPPORT
    /* check for separator for vector value */

    switch (*pFmt)
        {
        case ',':
        case ';':
        case ':':
        case '_':
            Separator = C_Separator = *pFmt++;
            break;
        }
#endif /* _WRS_ALTIVEC_SUPPORT */

    /* check for specification of size of returned value */
 
    switch (*pFmt)
        {
        case 'h':
	case 'L':
	    returnSpec = *pFmt++;

#ifdef _WRS_ALTIVEC_SUPPORT            
            if (*pFmt == 'v') { pFmt++; doVector = TRUE; }
#endif /* _WRS_ALTIVEC_SUPPORT */

	    break;
	case 'l':
            if (*(++pFmt) == 'l')
	        {
	        returnSpec = ll;
	        pFmt++;
	        }
	    else
	        returnSpec = 'l';	

#ifdef _WRS_ALTIVEC_SUPPORT            
            if (*pFmt == 'v') { pFmt++; doVector = TRUE; }
#endif /* _WRS_ALTIVEC_SUPPORT */ 

            break;

#ifdef _WRS_ALTIVEC_SUPPORT            
        case 'v':
            pFmt++;
            doVector = TRUE;
                
	    switch (*pFmt)
		{
		case 'h':
		case 'L':
		    returnSpec = *pFmt++;
		    break;
		case 'l':
	            if (*(++pFmt) == 'l')
		        {
		        returnSpec = ll;
		        pFmt++;
		        }
	            else
	               returnSpec = 'l';	
		    break;
		}
            break;
#endif /* _WRS_ALTIVEC_SUPPORT */
        }
 
#ifdef _WRS_ALTIVEC_SUPPORT            
    if (C_Separator && !doVector) return (FALSE); /* unsuccessful conversion */
#endif /* _WRS_ALTIVEC_SUPPORT */

    /* set default fieldwidth if not specified */
 
    if (fieldwidth == 0)
        fieldwidth = (*pFmt == 'c') ? 1 : INT_MAX;
 
 
    switch (*pFmt)
        {
        case 'c':               /* character, or character array */
#ifdef _WRS_ALTIVEC_SUPPORT
	    if ( doVector )
	      {
		if (fieldwidth != 1) return (FALSE); /* unsuccessful conversion */
		
		i  = 16;
		ix = 0;
		ch = *pCh;
		vp = (char*) &vec.s8;
			
		while (i--) {
			
			if (ch != EOF)
			{
			     *vp++ = (char) ch;
			     GET_CHAR (ch, ix);
			}
			else return (FALSE); /* unsuccessful conversion */

			if (C_Separator && i)
			{
			    if ( (char)ch != C_Separator ) return (FALSE);
			    GET_CHAR (ch, ix);
			}
		}

		*pCh = ch;
		*pNchars += ix;

		if (pReturn != NULL)
			*((__vector unsigned long *) pReturn) = vec.vul;

	        break;
	      }
#endif /* _WRS_ALTIVEC_SUPPORT */
	      
            if (!scanChar ((char *) pReturn, fieldwidth, getRtn, getArg,
                           pCh, pNchars))
                return (FALSE);         /* unsuccessful conversion */
            break;
 
        case 'i':               /* integer */
        case 'o':               /* octal integer */
        case 'd':               /* decimal integer */
        case 'u':               /* unsigned decimal integer */
        case 'x':               /* hex integer */
        case 'X':               /* hex integer */
        case 'p':               /* pointer */
 
            switch (*pFmt)
                {
                case 'i':       base = 0;       break;
                case 'o':       base = 8;       break;
                case 'd':       base = 10;      break;
                case 'u':       base = 10;      break;
                case 'x':       base = 16;      break;
                case 'X':       base = 16;      break;
                case 'p':       base = 16;      break;
                default :       base = 0;       break;
                }
#ifdef _WRS_ALTIVEC_SUPPORT
            if ( doVector )
	      {
		i  = ((returnSpec == 'h') ? 8:4);
		ix = 0;
		ch = *pCh;
			
		while (i--) {
			
			if (returnSpec != 'h') /* int, long */
			{
            		   if (!scanNum (base,(void*)&vec.u32[3-i], returnSpec,
            		                 fieldwidth, getRtn, getArg, pCh, pNchars))
                	   return (FALSE);         /* unsuccessful conversion */
			}
			else /* short int, AltiVec pixel */
			{
            		   if (!scanNum (base,(void*)&vec.u16[7-i], returnSpec,
            		                 fieldwidth, getRtn, getArg, pCh, pNchars))
                	   return (FALSE);         /* unsuccessful conversion */
			}

			if (i)
			{
			    if ( *pCh != Separator ) return (FALSE);
			    GET_CHAR (ch, ix);
			    *pCh = ch;
			}
		}

		*pNchars += ix;

		if (pReturn != NULL)
			*((__vector unsigned long *) pReturn) = vec.vul;

	        break;
	      }
#endif /* _WRS_ALTIVEC_SUPPORT */

            if (!scanNum (base, pReturn, returnSpec, fieldwidth, getRtn,
	 	          getArg, pCh, pNchars))
                return (FALSE);         /* unsuccessful conversion */
            break;
 
        case 'e':               /* exponential float */
        case 'f':               /* float */
        case 'g':               /* float */
        case 'E':               /* exponential double */
        case 'F':               /* double */
        case 'G':               /* double */
#ifdef _WRS_ALTIVEC_SUPPORT
            if ( doVector )
	      {
	      
	        if (fioFltScanRtn == NULL) return (FALSE);

		i  = 4;
		ix = 0;
		ch = *pCh;
			
		while (i--) 
		  {
			
            	    if (!(* fioFltScanRtn) ((void*)&vec.f32[3-i], 
					    returnSpec, fieldwidth, 
					    getRtn, getArg, pCh, pNchars))
                	   return (FALSE);    /* unsuccessful conversion */

			if (i)
			{
			    if ( *pCh != Separator ) return (FALSE);
			    GET_CHAR (ch, ix);
			    *pCh = ch;
			}
		}

		*pNchars += ix;

		if (pReturn != NULL)
			*((__vector unsigned long *) pReturn) = vec.vul;

	        break;
	      }
#endif /* _WRS_ALTIVEC_SUPPORT */

            if ((fioFltScanRtn == NULL) ||
                (!(* fioFltScanRtn) (pReturn, returnSpec, fieldwidth, getRtn,
                                     getArg, pCh, pNchars)))
                return (FALSE);         /* unsuccessful conversion */
            break;
 
        case 's':               /* string */
            if (!scanString ((char *) pReturn, fieldwidth, getRtn, getArg,
                             pCh, pNchars))
                return (FALSE);         /* unsuccessful conversion */
            break;
 
        case '[':               /* non-space-delimited string */
            if (!scanCharSet ((char *) pReturn, fieldwidth, pFmt, getRtn,
                              getArg, pCh, pNchars))
                return (FALSE);

            /* get past format specifier string */
 
            while (*pFmt != ']')
                ++pFmt;
            break;
 
 
        case 'n':               /* current number of chars scanned */
            if (pReturn != NULL)
                {
                switch (returnSpec)
                    {
                    case 'h': *((short *) pReturn) = *pNchars; break;
                    case 'l': *((long *) pReturn)  = *pNchars; break;
		    case (ll) : *((long long *) pReturn) = *pNchars; break;
                    default:  *((int *) pReturn)   = *pNchars; break;
                    }
                }
            break;
 
        default:
            return (FALSE);
        }
 
    *ppFmt = pFmt + 1;          /* update callers format string pointer */
    
    return (TRUE);              /* successful conversion */
    }
/********************************************************************************
* scanString - perform conversion of space delineated string
*
* RETURNS: TRUE if successful, FALSE if unsuccessful.
*/

LOCAL BOOL scanString
    (
    FAST char * pResult,
    FAST int    fieldwidth,
    FAST FUNCPTR getRtn,
    FAST int    getArg,
    int *       pCh,
    int *       pNchars
    )
    {
    FAST int ix = 0;
    FAST int ch = *pCh;

    while (ch != EOF && ix < fieldwidth)
        {
        if (isspace (ch))
            break;

        if (pResult != NULL)
            *pResult++ = (char)ch;

        GET_CHAR (ch, ix);
        }

    if (pResult != NULL)
        *pResult = EOS;
 
    *pCh = ch;
    *pNchars += ix;
 
    return (ix != 0);
    }

/********************************************************************************
* scanChar - perform character conversion
*
* RETURNS: TRUE if successful, FALSE if unsuccessful.
*/

LOCAL BOOL scanChar
    (
    FAST char * pResult,
    FAST int    fieldwidth,
    FAST FUNCPTR getRtn,
    FAST int    getArg,
    FAST int *  pCh,
    FAST int *  pNchars
    )
    {
    FAST int ch = *pCh;
    FAST int ix = 0;

    while (ch != EOF && ix < fieldwidth)
        {
        if (pResult != NULL)
            *(pResult++) = (char) ch;

        GET_CHAR (ch, ix);
        }

    *pCh = ch;
    *pNchars += ix;

    return (ix != 0);
    }

/********************************************************************************
* scanCharSet - perform user-specified string conversion
*
* RETURNS: TRUE if successful, FALSE if unsuccessful.
*
* NOMANUAL
*/
 
BOOL scanCharSet
    (
    FAST char * pResult,
    int         fieldwidth,
    FAST const char *charsAllowed,   /* The chars (not) allowed in the string */
    FAST FUNCPTR getRtn,
    FAST int    getArg,
    int *       pCh,
    int *       pNchars
    )
    {
#define NUM_ASCII_CHARS 128
    char lastChar = 0;
    char charsAllowedTbl [NUM_ASCII_CHARS];
    FAST BOOL allowed;
    int nx;
    FAST int ix;
    FAST int ch;
 
    if (*charsAllowed++ != '[')         /* skip past the [ */
        return (FALSE);
 
    /* chars in table will (not) be allowed */
 
    if (*charsAllowed == '^')
        {
        allowed = FALSE;
        ++charsAllowed;                 /* skip past ^ */
        }
    else
        allowed = TRUE;
 
    /* initialize table to (not) allow any chars */
 
    for (ix = 0; ix < NUM_ASCII_CHARS; ix++)
        charsAllowedTbl [ix] = !allowed;
 
    /* Check for the first char of the set being '-' or ']', in which case
     * they are not special chars.
     */
 
    if (*charsAllowed == '-' || *charsAllowed == ']')
        charsAllowedTbl [(int)(*charsAllowed++)] = allowed;
 
    /* Examine the rest of the set, and put it into the table. */
 
    for (nx = 0; nx < NUM_ASCII_CHARS; nx++)
        {
        if (*charsAllowed == ']' || *charsAllowed == EOS)
            break;      /* all done */
 
        /* Check if its of the form x-y.
         * If x<y, then allow all chars in the range.
         * If x>=y, its not a legal range.  Just allow '-'.
         * Also, if the '-' is the last char before the final ']',
         * just allow '-'.
         */
        if (*charsAllowed == '-' &&
            (lastChar < *(charsAllowed+1) && *(charsAllowed+1) != ']'))
            {
            ++charsAllowed;     /* skip - */
            for (ix = lastChar; ix <= *charsAllowed; ix++)
                charsAllowedTbl [ix] = allowed;
            }
        else
            {
            charsAllowedTbl [(int)(*charsAllowed)] = allowed;
            lastChar = *charsAllowed;
            }
        ++charsAllowed;
        }
 
    /* make sure that ']' appeared */
 
    if (*charsAllowed != ']')
        return (FALSE);
 
    /* Copy the appropriate portion of the string */
 
    ix = 0;
    ch = *pCh;
 
    while (ch != EOF && ix < fieldwidth)
        {
        if (! charsAllowedTbl [(int)ch])
            break;
 
        if (pResult != NULL)
            *pResult++ = (char) ch;
 
        GET_CHAR (ch, ix);
        }
 
	/* If no characters were copied from the input, then this assignment
		has failed */
	
	if(ix == 0)
		return (FALSE);
	else
		{

    	if (pResult != NULL)
        	*pResult = EOS;
 
    	*pCh = ch;
    	*pNchars += ix;

    	return (TRUE);
		}
    }
/********************************************************************************
* scanNum - perform number conversion
*
* RETURNS: TRUE if successful, FALSE if unsuccessful.
*/
 
LOCAL BOOL scanNum
    (
    int         base,
    FAST void * pReturn,
    int         returnSpec,
    int         fieldwidth,
    FAST FUNCPTR getRtn,
    FAST int    getArg,
    int *       pCh,
    int *       pNchars
    )
    {
    int         dig;                    /* current digit */
    BOOL        neg     = FALSE;        /* negative or positive? */
    long long   num     = 0;            /* scanned number */
    FAST int    ix      = 0;            /* present used width */
    FAST int    ch      = *pCh;         /* current character */
 
    /* check for sign */
 
    if (ix < fieldwidth)
        {
        if ((char)ch == '+' || (neg = ((char)ch == '-')))
            GET_CHAR (ch, ix);
        }
 
 
    /* check for optional 0 or 0x */
 
    if (ix < fieldwidth && ch == '0')
        {
        GET_CHAR (ch, ix);
 
        if ((ix < fieldwidth) &&
            (ch == 'x' || ch == 'X') &&
            (base == 16 || base == 0))
            {
            base = 16;
            GET_CHAR (ch, ix);
            }
        else if (base == 0)
            base = 8;
        }
    else
        {
        /* default base is 10 */
        if (base == 0)
            base = 10;
        }
 
 
    /* scan digits */
 
    while (ch != EOF && ix < fieldwidth)
        {
        if (isdigit (ch))
            dig = ch - '0';
        else if (islower (ch))
            dig = ch - 'a' + 10;
        else if (isupper (ch))
            dig = ch - 'A' + 10;
        else
            break;
 
        if (dig >= base)
            break;
 
        num = (num * base) + dig;
 
        GET_CHAR (ch, ix);
        }
 
 
    /* check that we scanned at least one character */
 
    if (ix == 0)
        return (FALSE);
 
 
    /* return value to caller */
 
    if (neg)
        num = -num;
 
    if (pReturn != NULL)
        {
        switch (returnSpec)
            {
            case 'h': *((short *) pReturn) = num; break;
            case 'l': *((long *) pReturn)  = num; break;
            case (ll) : *((long long *) pReturn) = num; break;
            default:  *((int *) pReturn)   = num; break;
            }
        }
 
    *pCh = ch;
    *pNchars += ix;
 
    return (ix != 0);
    }


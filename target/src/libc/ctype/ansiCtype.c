/* ansiCtype.c - ANSI `ctype' documentation */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,03mar93,jdi  more documentation cleanup for 5.1.
01b,07feb93,jdi  documentation cleanup for 5.1.
01a,24oct92,smb  written
*/

/*
DESCRIPTION
The header ctype.h declares several functions useful for testing and
mapping characters.  In all cases, the argument is an `int', the value of
which is representable as an `unsigned char' or is equal to the
value of the macro EOF.  If the argument has any other value, the
behavior is undefined.

The behavior of the 'ctype' functions is affected by the current locale.
VxWorks supports only the "C" locale.

The term "printing character" refers to a member of an implementation-defined
set of characters, each of which occupies one printing position on a
display device; the term "control character" refers to a member of an
implementation-defined set of characters that are not printing characters.

INCLUDE FILES: ctype.h

SEE ALSO: American National Standard X3.159-1989
*/

/* 
INTERNAL DOCUMENTATION
 If locale additions are necessary the arrays of unsigned char will
   not be large enough. Type short should be used instead. Also two
   extra character classifications will be needed. _C_EXTRA_SPACE and
   _C_EXTRA_ALPHA.

   Two extra tables may be necessary for completeness but at the moment they
   are too large. Adding them is not difficult. The tables are defined in
   __tolower_tab.c  __toupper_tab.c

   extern  const unsigned char *_Tolower;
   extern  const unsigned char *_Toupper;

   __ctype_tab.c is a table of character classifications and is used to
   determine to what class a character belongs.

NOMANUAL

INTERNAL
This documentation module is built by appending the following files:

    isalnum.c
    isalpha.c
    iscntrl.c
    isdigit.c
    isgraph.c
    islower.c
    isprint.c
    ispunct.c
    isspace.c
    isupper.c
    isxdigit.c
    tolower.c
    toupper.c

*/


/* isalnum.c - character classification */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,03mar93,jdi  more documentation cleanup for 5.1.
01d,07feb93,jdi  documentation cleanup for 5.1.
01c,20sep92,smb  documentation additions.
01b,04jul92,smb  added vxWorks.h and extra documentation
01a,24mar92,smb  written
*/

/*
DESCRIPTION

INCLUDE FILE: ctype.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "ctype.h"


#undef isalnum
/*******************************************************************************
*
* isalnum - test whether a character is alphanumeric (ANSI)
*
* This routine tests whether <c> is a character for which isalpha() or
* isdigit() returns true.
*
* INCLUDE FILES: ctype.h
*
* RETURNS: Non-zero if and only if <c> is alphanumeric.
*/

int isalnum 
    (
    int c       /* character to test */
    )
    {
    return __isalnum(c);
    }
/* isalpha.c - character classification and conversion macros */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,03mar93,jdi  more documentation cleanup for 5.1.
01d,07feb93,jdi  documentation cleanup for 5.1.
01c,20sep92,smb  documentation additions.
01b,04jul92,smb  added vxWorks.h and extra documentation
01a,24mar92,smb  written
*/

/*
DESCRIPTION

INCLUDE FILE: ctype.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "ctype.h"


#undef isalpha
/*******************************************************************************
*
* isalpha - test whether a character is a letter (ANSI)
*
* This routine tests whether <c> is a character 
* for which isupper() or islower() returns true.
*
* INCLUDE FILES: ctype.h
*
* RETURNS: Non-zero if and only if <c> is a letter.
*/

int isalpha 
    (
    int c       /* character to test */
    )
    {
    return __isalpha(c);
    }

/* iscntrl.c - character classification */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,03mar93,jdi  more documentation cleanup for 5.1.
01d,07feb93,jdi  documentation cleanup for 5.1.
01c,20sep92,smb  documentation additions
01b,04jul92,smb  added vxWorks.h and extra documentation
01a,24mar92,smb  written
*/

/*
DESCRIPTION

INCLUDE FILE: ctype.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "ctype.h"


#undef iscntrl
/*******************************************************************************
*
* iscntrl - test whether a character is a control character (ANSI)
*
* This routine tests whether <c> is a control character.
*
* INCLUDE FILES: ctype.h
*
* RETURNS: Non-zero if and only if <c> is a control character.
*/

int iscntrl 
    (
    int c       /* character to test */
    )
    {
    return __iscntrl(c);
    }
/* isdigit.c - character classification */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,03mar93,jdi  more documentation cleanup for 5.1.
01d,07feb93,jdi  documentation cleanup for 5.1.
01c,20sep92,smb  documentation additions
01b,04jul92,smb  added vxWorks.h and extra documentation
01a,24mar92,smb  written
*/

/*
DESCRIPTION

INCLUDE FILE: ctype.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "ctype.h"


#undef isdigit
/*******************************************************************************
*
* isdigit - test whether a character is a decimal digit (ANSI)
*
* This routine tests whether <c> is a decimal-digit character.
*
* INCLUDE FILES: ctype.h
*
* RETURNS: Non-zero if and only if <c> is a decimal digit.
*/

int isdigit 
    (
    int c       /* character to test */
    )
    {
    return __isdigit(c);
    }
/* isgraph.c - character classification and conversion macros */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,03mar93,jdi  more documentation cleanup for 5.1.
01d,07feb93,jdi  documentation cleanup for 5.1.
01c,20sep92,smb  documentation additions
01b,04jul92,smb  added vxWorks.h and extra documentation
01a,24mar92,smb  
*/

/*
DESCRIPTION

INCLUDE FILE: ctype.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "ctype.h"


#undef isgraph
/*******************************************************************************
*
* isgraph - test whether a character is a printing, non-white-space character (ANSI)
*
* This routine returns true if <c> is a printing character, and not a
* character for which isspace() returns true.
*
* INCLUDE FILES: ctype.h
*
* RETURNS:
* Non-zero if and only if <c> is a printable, non-white-space character.
*
* SEE ALSO: isspace()
*/

int isgraph 
    (
    int c       /* character to test */
    )
    {
    return __isgraph(c); 
    }
/* islower.c - character conversion macros */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,03mar93,jdi  more documentation cleanup for 5.1.
01d,07feb93,jdi  documentation cleanup for 5.1.
01c,20sep92,smb  documentation additions
01b,04jul92,smb  added vxWorks.h and extra documentation
01a,24mar92,smb  written
*/

/*
DESCRIPTION

INCLUDE FILE: ctype.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "ctype.h"


#undef islower
/*******************************************************************************
*
* islower - test whether a character is a lower-case letter (ANSI)
*
* This routine tests whether <c> is a lower-case letter.
*
* INCLUDE FILES: ctype.h
*
* RETURNS: Non-zero if and only if <c> is a lower-case letter.
*/

int islower 
    (
    int c       /* character to test */
    )
    {
    return __islower(c);
    }

/* isprint.c - character classification and conversion macros */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,03mar93,jdi  more documentation cleanup for 5.1.
01d,07feb93,jdi  documentation cleanup for 5.1.
01c,20sep92,smb  documentation additions
01b,04jul92,smb  added vxWorks.h and extra documentation
01a,24mar92,smb  written
*/

/*
DESCRIPTION

INCLUDE FILE: ctype.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "ctype.h"

#undef isprint
/*******************************************************************************
*
* isprint - test whether a character is printable, including the space character (ANSI)
*
* This routine returns true if <c> is a printing character or the space
* character.
*
* INCLUDE FILES: ctype.h
*
* RETURNS: Non-zero if and only if <c> is printable, including the space
* character.
*/

int isprint 
    (
    int c       /* character to test */
    )
    {
    return __isprint(c);
    }
/* ispunct.c - character classification and conversion macros */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,03mar93,jdi  more documentation cleanup for 5.1.
01d,07feb93,jdi  documentation cleanup for 5.1.
01c,20sep92,smb  documentation additions
01b,04jul92,smb  added vxWorks.h and extra documentation
01a,24mar92,smb  written
*/

/*
DESCRIPTION

INCLUDE FILE: ctype.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "ctype.h"


#undef ispunct
/*******************************************************************************
*
* ispunct - test whether a character is punctuation (ANSI)
*
* This routine tests whether a character is punctuation, i.e., a printing
* character for which neither isspace() nor isalnum() is true.
*
* INCLUDE FILES: ctype.h
*
* RETURNS: Non-zero if and only if <c> is a punctuation character.
*/

int ispunct 
    (
    int c       /* character to test */
    )
    {
    return __ispunct(c);
    }
/* isspace.c - character classification and conversion macros */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,03mar93,jdi  more documentation cleanup for 5.1.
01d,07feb93,jdi  documentation cleanup for 5.1.
01c,20sep92,smb  documentation additions
01b,04jul92,smb  added vxWorks.h and extra documentation
01a,24mar92,smb  written
*/

/*
DESCRIPTION

INCLUDE FILE: ctype.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL

*/

#include "vxWorks.h"
#include "ctype.h"


#undef isspace
/*******************************************************************************
*
* isspace - test whether a character is a white-space character (ANSI)
*
* This routine tests whether a character is one of
* the standard white-space characters, as follows:
* 
* .TS
* tab(|);
* l l.
*         space           | "\0"
*         horizontal tab  | \et
*         vertical tab    | \ev
*         carriage return | \er
*         new-line        | \en
*         form-feed       | \ef
* .TE
*
* INCLUDE FILES: ctype.h
*
* RETURNS: Non-zero if and only if <c> is a space, tab, carriage return,
* new-line, or form-feed character.
*/

int isspace 
    (
    int c       /* character to test */
    )
    {
    return __isspace(c);
    }
/* isupper.c - character classification and conversion macros */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,03mar93,jdi  more documentation cleanup for 5.1.
01d,07feb93,jdi  documentation cleanup for 5.1.
01c,20sep92,smb  documentation additions
01b,04jul92,smb  added vxWorks.h and extra documentation
01a,24mar92,smb  written
*/

/*
DESCRIPTION

INCLUDE FILE: ctype.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "ctype.h"


#undef isupper
/*******************************************************************************
*
* isupper - test whether a character is an upper-case letter (ANSI)
*
* This routine tests whether <c> is an upper-case letter.
*
* INCLUDE FILES: ctype.h
*
* RETURNS: Non-zero if and only if <c> is an upper-case letter.
*
*/

int isupper 
    (
    int c       /* character to test */
    )
    {
    return __isupper(c);
    }

/* isxdigit.c - character classification and conversion macros */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,03mar93,jdi  more documentation cleanup for 5.1.
01d,07feb93,jdi  documentation cleanup for 5.1.
01c,20sep92,smb  documentation additions
01b,04jul92,smb  added vxWorks.h and extra documentation
01a,24mar92,smb  written
*/

/*
DESCRIPTION

INCLUDE FILE: ctype.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "ctype.h"


#undef isxdigit
/*******************************************************************************
*
* isxdigit - test whether a character is a hexadecimal digit (ANSI)
*
* This routine tests whether <c> is a hexadecimal-digit character.
*
* INCLUDE FILES: ctype.h
*
* RETURNS: Non-zero if and only if <c> is a hexadecimal digit.
*/

int isxdigit 
    (
    int c       /* character to test */
    )
    {
    return __isxdigit(c);
    }

/* tolower.c - character classification and conversion macros */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01f,03mar93,jdi  more documentation cleanup for 5.1.
01e,07feb93,jdi  documentation cleanup for 5.1.
01d,13oct92,jdi  mangen fixes.
01c,20sep92,smb  documentation additions
01b,04jul92,smb  added vxWorks.h and extra documentation
01a,24mar92,smb  written
*/

/*
DESCRIPTION

INCLUDE FILE: ctype.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "ctype.h"


#undef tolower
/*******************************************************************************
*
* tolower - convert an upper-case letter to its lower-case equivalent (ANSI)
*
* This routine converts an upper-case letter to the corresponding lower-case
* letter.
*
* INCLUDE FILES: ctype.h
*
* RETURNS:
* If <c> is an upper-case letter, it returns the lower-case equivalent;
* otherwise, it returns the argument unchanged.
*/

int tolower 
    (
    int c       /* character to convert */
    )
    {
    return __tolower(c);
    }

/* toupper.c - character classification and conversion macros */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,03mar93,jdi  more documentation cleanup for 5.1.
01d,07feb93,jdi  documentation cleanup for 5.1.
01c,20sep92,smb  documentation additions
01b,04jul92,smb  added vxWorks.h and extra documentation
01a,24mar92,smb  written
*/

/*
DESCRIPTION

INCLUDE FILE: ctype.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "ctype.h"


#undef toupper
/*******************************************************************************
*
* toupper - convert a lower-case letter to its upper-case equivalent (ANSI)
*
* This routine converts a lower-case letter to the corresponding upper-case
* letter.
*
* INCLUDE FILES: ctype.h
*
* RETURNS:
* If <c> is a lower-case letter, it returns the upper-case equivalent;
* otherwise, it returns the argument unchanged.
*/

int toupper 
    (
    int c       /* character to convert */
    )
    {
    return __toupper(c);
    }

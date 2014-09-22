/* ansiString.c - ANSI `string' documentation */

/* Copyright 1992-1995 Wind River Systems, Inc. */

/*
modification history
--------------------
01f,11jul97,dgp  doc: SPR 7651 need list of non-reentrant functions
01e,23oct95,jdi  doc: incorporated changes for strtok() & strtok_r() (SPR 4874).
01d,11feb95,jdi  fixed size parameter name in doc for memset().
01c,25feb93,jdi  documentation cleanup for 5.1.
01b,30nov92,jdi  fixed doc for strerror() - SPR 1825.
01a,24oct92,smb  written
*/

/*
DESCRIPTION
This library includes several standard ANSI routines.  Note that where
there is a pair of routines, such as div() and div_r(), only the routine
xxx_r() is reentrant.  The xxx() routine is not reentrant.
 
The header string.h declares one type and several functions, and defines one
macro useful for manipulating arrays of character type and other objects
treated as array of character type.  The type is `size_t' and the macro NULL.
Various methods are used for determining the lengths of the arrays, but in
all cases a `char *' or `void *' argument points to the initial (lowest
addressed) character of the array.  If an array is accessed beyond the end
of an object, the behavior is undefined.

SEE ALSO: American National Standard X3.159-1989

INTERNAL
This documentation module is built by appending the following files:

    memchr.c
    memcmp.c
    memcpy.c
    memmove.c
    memset.c
    strcat.c
    strchr.c
    strcmp.c
    strcoll.c
    strcpy.c
    strcspn.c
    strerror.c
    strlen.c
    strncat.c
    strncmp.c
    strncpy.c
    strpbrk.c
    strrchr.c
    strspn.c
    strstr.c
    strtok.c
    strtok_r.c
    strxfrm.c
*/


/* memchr.c - search memory for a character, string */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,25feb93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions
01a,08jul92,smb  written and documented.
*/

/*
DESCRIPTION

INCLUDE FILES: string.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "string.h"

/*******************************************************************************
*
* memchr - search a block of memory for a character (ANSI)
*
* This routine searches for the first element of an array of `unsigned char',
* beginning at the address <m> with size <n>, that equals <c> converted to
* an `unsigned char'.
*
* INCLUDE FILES: string.h
*
* RETURNS: If successful, it returns the address of the matching element;
* otherwise, it returns a null pointer.
*/

void * memchr
    (
    const void * m,		/* block of memory */
    int 	 c,		/* character to search for */
    size_t 	 n		/* size of memory to search */
    )
    {
    uchar_t *p = (uchar_t *) CHAR_FROM_CONST(m);

    if (n != 0)
	do 
	    {
	    if (*p++ == (unsigned char) c)
		return (VOID_FROM_CONST(p - 1));

	    } while (--n != 0);

    return (NULL);
    }
/* memcmp.c - memory compare file for string */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,25feb93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions
01a,08jul92,smb  written and documented.
*/

/*
DESCRIPTION

INCLUDE FILE: string.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "string.h"

/*******************************************************************************
*
* memcmp - compare two blocks of memory (ANSI)
*
* This routine compares successive elements from two arrays of `unsigned char',
* beginning at the addresses <s1> and <s2> (both of size <n>), until it finds
* elements that are not equal.
*
* INCLUDE FILES: string.h
*
* RETURNS:
* If all elements are equal, zero.  If elements differ and the differing
* element from <s1> is greater than the element from <s2>, the routine
* returns a positive number; otherwise, it returns a negative number.
*/

int memcmp
    (
    const void * s1,		/* array 1 */
    const void * s2,		/* array 2 */
    size_t       n		/* size of memory to compare */
    )
    {
    const unsigned char *p1;
    const unsigned char *p2;

    /* size of memory is zero */

    if (n == 0)
	return (0);

    /* compare array 2 into array 1 */

    p1 = s1;
    p2 = s2;

    while (*p1++ == *p2++)
	{
	if (--n == 0)
	    return (0);
        }

    return ((*--p1) - (*--p2));
    }
/* memcpy.c - memory copy file for string */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01g,25feb93,jdi  documentation cleanup for 5.1.
01f,20sep92,smb  documentation additions
01e,14sep92,smb  memcpy again uses bcopy
01d,07sep92,smb  changed so that memcpy is seperate from bcopy.
01c,30jul92,smb  changed to use bcopy.
01b,12jul92,smb  changed post decrements to pre decrements.
01a,08jul92,smb  written and documented.
           +rrr
*/

/*
DESCRIPTION

INCLUDE FILES: string.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "string.h"


/*******************************************************************************
*
* memcpy - copy memory from one location to another (ANSI)
*
* This routine copies <size> characters from the object pointed
* to by <source> into the object pointed to by <destination>. If copying
* takes place between objects that overlap, the behavior is undefined.
*
* INCLUDE FILES: string.h
*
* RETURNS: A pointer to <destination>.
*/

void * memcpy
    (
    void *       destination,   /* destination of copy */
    const void * source,        /* source of copy */
    size_t       size           /* size of memory to copy */
    )
    {
    bcopy ((char *) source, (char *) destination, (size_t) size);
    return (destination);
    }

/* memmove.c - memory move file for string */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,25feb93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions
01a,08jul92,smb  written and documented.
*/

/*
DESCRIPTION

INCLUDE FILES: string.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "string.h"

/*******************************************************************************
*
* memmove - copy memory from one location to another (ANSI)
*
* This routine copies <size> characters from the memory location <source> to
* the location <destination>.  It ensures that the memory is not corrupted
* even if <source> and <destination> overlap.
*
* INCLUDE FILES: string.h
*
* RETURNS: A pointer to <destination>.
*/

void * memmove
    (
    void *	 destination,	/* destination of copy */
    const void * source,	/* source of copy */
    size_t 	 size		/* size of memory to copy */
    )
    {
    char *	dest;
    const char *src;

    dest = destination;
    src = source;

    if ((src < dest) && (dest < (src + size)))
	{
	for (dest += size, src += size; size > 0; --size)
	    *--dest = *--src;
        }
    else 
	{
	while (size > 0)
	    {
	    size--;
	    *dest++ = *src++;
	    }
        }
    return (destination);
    }
/* memset.c - set a block of memory, string */

/* Copyright 1992-1995 Wind River Systems, Inc. */

/*
modification history
--------------------
01g,11feb95,jdi  fixed size parameter name in doc.
01f,25feb93,jdi  documentation cleanup for 5.1.
01e,20sep92,smb  documentation additions
01d,14sep92,smb  changes back to use bfill.
01c,07sep92,smb  changed so that memset is seperate from bfill
01b,30jul92,smb  changes to use bfill.
01a,08jul92,smb  written and documented.
           +rrr
*/

/*
DESCRIPTION

INCLUDE FILES: string.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "string.h"

/*******************************************************************************
*
* memset - set a block of memory (ANSI)
*
* This routine stores <c> converted to an `unsigned char' in each of the
* elements of the array of `unsigned char' beginning at <m>, with size <size>.
*
* INCLUDE FILES: string.h
*
* RETURNS: A pointer to <m>.
*/

void * memset
    (
    void * m,                   /* block of memory */
    int    c,                   /* character to store */
    size_t size                 /* size of memory */
    )
    {
    bfill ((char *) m, (int) size, c);
    return (m);
    }
/* strcat.c - concatenate one string to another, string */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,25feb93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions
01a,08jul92,smb  written and documented.
*/

/*
DESCRIPTION

INCLUDE FILES: string.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "string.h"


/*******************************************************************************
*
* strcat - concatenate one string to another (ANSI)
*
* This routine appends a copy of string <append> to the end of string 
* <destination>.  The resulting string is null-terminated.
*
* INCLUDE FILES: string.h
*
* RETURNS: A pointer to <destination>.
*/

char * strcat
    (
    char *       destination, /* string to be appended to */
    const char * append       /* string to append to <destination> */
    )
    {
    char *save = destination;

    while (*destination++ != '\0')		/* find end of string */
        ;

    destination--;

    while ((*destination++ = *append++) != '\0')
	;

    return (save);
    }
/* strchr.c - search string for character, string */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,25feb93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions
01a,08jul92,smb  written and documented.
*/

/*
DESCRIPTION

INCLUDE FILES: string.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "string.h"


/******************************************************************************
*
* strchr - find the first occurrence of a character in a string (ANSI)
*
* This routine finds the first occurrence of character <c>
* in string <s>.  The terminating null is considered to be part of the string.
*
* INCLUDE FILES: string.h
*
* RETURNS:
* The address of the located character, or NULL if the character is not found.
*/

char * strchr
    (
    const char * s,         /* string in which to search */
    int 	 c          /* character to find in string */
    )
    {
    char *r = CHAR_FROM_CONST(s); 

    while (*r != (char) c)		/* search loop */
	{
	if (*r++ == EOS)		/* end of string */
	    return (NULL);
        }

    return (r);
    }
/* strcmp.c - compare two strings, string */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,25feb93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions
01a,08jul92,smb  written and documented.
*/

/*
DESCRIPTION

INCLUDE FILES: string.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "string.h"


/*******************************************************************************
*
* strcmp - compare two strings lexicographically (ANSI)
*
* This routine compares string <s1> to string <s2> lexicographically.
*
* INCLUDE FILES: string.h
*
* RETURNS: An integer greater than, equal to, or less than 0,
* according to whether <s1> is lexicographically
* greater than, equal to, or less than <s2>, respectively.
*/

int strcmp
    (
    const char * s1,   /* string to compare */
    const char * s2    /* string to compare <s1> to */
    )
    {
    while (*s1++ == *s2++)
	if (s1 [-1] == EOS)
	    return (0);

    return ((s1 [-1]) - (s2 [-1]));
    }
/* strcoll.c - string collate, string */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,25feb93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions
01a,08jul92,smb  written and documented.
*/

/*
DESCRIPTION

INCLUDE FILES: string.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "private/strxfrmP.h"

/* The __sctl type describes a data object that holds the information 
 * needed to process each source string. The internal function getxfrm
 * calls __strxfrm to update an sctl data object.
 */
typedef struct 
    {
    char 	   buf[32];
    const uchar_t *s1;
    const uchar_t *s2;
    const uchar_t *sout;
    __cosave       state;
    } __sct1;


/***************************************************************************
*
* getxfrm - get transformed characters
*
* A conparison loop within strcoll calls getxfrm for each source string that
* has no mapped characters in its sctl buffer. This ensures that each source
* string is represented by at least one mapped character, if any such
* character remains to be generated.
*
* RETURNS: the size of the transformed string
* NOMANUAL
*/

LOCAL size_t getxfrm
    (
    __sct1 *p	/* information needed to process each source string */
    )
    {
    size_t sz;

    /* loop until chars delivered */
    do  
	{
        p->sout = (const uchar_t *) p->buf;

        sz = __strxfrm (p->buf, &p->s1, sizeof (p->buf), &p->state);

        if ((sz > 0) && (p->buf [sz - 1] == EOS))
    	    return (sz - 1);

        if (*p->s1 == EOS)
	     p->s1 = p->s2;		/* rescan */

        } while (sz == 0);

    return (sz);
    }
	
/***************************************************************************
*
* strcoll - compare two strings as appropriate to LC_COLLATE  (ANSI)
*
* This routine compares two strings, both interpreted as appropriate to the
* LC_COLLATE category of the current locale.
*
* INCLUDE FILES: string.h
*
* RETURNS:
* An integer greater than, equal to, or less than zero, according to whether
* string <s1> is greater than, equal to, or less than string <s2> when both
* are interpreted as appropriate to the current locale.
*/

int strcoll
    (
    const char * s1,	/* string 1 */
    const char * s2	/* string 2 */
    )
    {
    size_t n1 = 0;		/* size of string 1 */
    size_t n2 = 0;		/* size of string 2 */
    __sct1 st1;			/* transform structure for string 1 */
    __sct1 st2;		 	/* transform structure for string 2 */
    static const __cosave initial = 
	{
	0
	};

    /* compare s1[], s2[] using locale-dependant rules */

    st1.s1	= (const uchar_t *)s1;	/* string transformation 1 */
    st1.s2	= (const uchar_t *)s1;
    st1.state	= initial;

    st2.s1	= (const uchar_t *)s2;	/* string transformation 2 */
    st2.s2	= (const uchar_t *)s2;
    st2.state	= initial;

    FOREVER				/* compare transformed characters */
    	{
    	int ans;
    	size_t sz;

    	if (n1 == 0)
    	    n1 = getxfrm (&st1);	/* string 1 */

    	if (n2 == 0)
    	    n2 = getxfrm (&st2);	/* string 2 */

    	sz = (n1 < n2) ? n1 : n2;

    	if (sz == 0)
	    {
    	    if (n1 == n2) 
		return (0); 

	    if (n2 > 0) 
		return (-1);

	    return (1);
            }

    	if ((ans = memcmp (st1.sout, st2.sout, sz)) != 0)
    	    return (ans);

    	st1.sout += sz;
    	st2.sout += sz;
    	n1 	 -= sz;
    	n2 	 -= sz;
    	}
    }
/* strcpy.c - string copy, string */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,25feb93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions
01a,08jul92,smb  written and documented.
*/

/*
DESCRIPTION

INCLUDE FILES: string.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "string.h"


/*******************************************************************************
*
* strcpy - copy one string to another (ANSI)
*
* This routine copies string <s2> (including EOS) to string <s1>.
*
* INCLUDE FILES: string.h
*
* RETURNS: A pointer to <s1>.
*/

char * strcpy
    (
    char *       s1,	/* string to copy to */
    const char * s2	/* string to copy from */
    )
    {
    char *save = s1;

    while ((*s1++ = *s2++) != EOS)
	;

    return (save);
    }
/* strcspn.c - search string for character, string */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,25feb93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions
01a,08jul92,smb  written and documented.
*/

/*
DESCRIPTION

INCLUDE FILES: string.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "string.h"


/*******************************************************************************
*
* strcspn - return the string length up to the first character from a given set (ANSI)
*
* This routine computes the length of the maximum initial segment of string
* <s1> that consists entirely of characters not included in string <s2>.
*
* INCLUDE FILES: string.h
*
* RETURNS:
* The length of the string segment.
*
* SEE ALSO: strpbrk(), strspn()
*/
 
size_t strcspn   
    (
    const char * s1,	/* string to search */
    const char * s2	/* set of characters to look for in <s1> */
    )
    {
    const char *save;
    const char *p;
    char 	c1;
    char 	c2;

    for (save = s1 + 1; (c1 = *s1++) != EOS; )	/* search for EOS */
	for (p = s2; (c2 = *p++) != EOS; )	/* search for first occurance */
	    {
	    if (c1 == c2)
		return (s1 - save);	      	/* return index of substring */
            }

    return (s1 - save);
    }
/* strerror.c - string error, string */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01d,25feb93,jdi  documentation cleanup for 5.1.
01c,30nov92,jdi  fixed doc for strerror() - SPR 1825.
01b,20sep92,smb  documentation additions
01a,08jul92,smb  written and documented.
*/

/*
DESCRIPTION

INCLUDE FILES: string.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "string.h"
#include "errno.h"
#include "symLib.h"
#include "limits.h"
#include "stdio.h"
#include "sysSymTbl.h"
#include "private/funcBindP.h"


/* forward declarations */

LOCAL STATUS strerrorIf (int errcode, char *buf);


/*******************************************************************************
*
* strerror_r - map an error number to an error string (POSIX)
*
* This routine maps the error number in <errcode> to an error message string.
* It stores the error string in <buffer>.
*
* This routine is the POSIX reentrant version of strerror().
*
* INCLUDE FILES: string.h
*
* RETURNS: OK or ERROR.
*
* SEE ALSO: strerror()
*/

STATUS strerror_r 
    (
    int    errcode,	/* error code */
    char * buffer	/* string buffer */
    )
    {
    return (strerrorIf (errcode, buffer));
    }

/*******************************************************************************
*
* strerror - map an error number to an error string (ANSI)
*
* This routine maps the error number in <errcode> to an error message string.
* It returns a pointer to a static buffer that holds the error string.
*
* This routine is not reentrant.  For a reentrant version, see strerror_r().
*
* INCLUDE: string.h
*
* RETURNS: A pointer to the buffer that holds the error string.
*
* SEE ALSO: strerror_r()
*/

char * strerror
    (
    int errcode		/* error code */
    )
    {
    static char buffer [NAME_MAX];

    (void) strerror_r (errcode, buffer);

    return (buffer);
    }

/*******************************************************************************
*
* strerrorIf - interface from libc to VxWorks for strerror_r
*
* RETURNS: OK, or ERROR if <buf> is null.
* NOMANUAL
*/

LOCAL STATUS strerrorIf 
    (
    int   errcode,		/* error code */
    char *buf			/* string buffer */
    )
    {
    int		value;
    SYM_TYPE	type;
    char	statName [NAME_MAX];

    if (buf == NULL)
	return (ERROR);

    if (errcode == 0)
	{
        strcpy (buf, "OK");
	return (OK);
	}

    if ((_func_symFindByValueAndType != (FUNCPTR) NULL) && (statSymTbl != NULL))
	{
	(* _func_symFindByValueAndType) (statSymTbl, errcode, statName, &value,
					 &type, SYM_MASK_NONE, SYM_MASK_NONE);
	if (value == errcode)
	    {
	    strcpy (buf, statName);
	    return (OK);
	    }
	}

    sprintf (buf, "errno = %#x", errcode);

    return (OK);
    }
/* strlen.c - file for string */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,25feb93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions
01a,08jul92,smb  written and documented.
*/

/*
DESCRIPTION

INCLUDE FILES: string.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "string.h"


/*******************************************************************************
*
* strlen - determine the length of a string (ANSI)
*
* This routine returns the number of characters in <s>, not including EOS.
*
* INCLUDE FILES: string.h
*
* RETURNS: The number of non-null characters in the string.
*/

size_t strlen
    (
    const char * s        /* string */
    )
    {
    const char *save = s + 1;

    while (*s++ != EOS)
	;

    return (s - save);
    }
/* strncat.c - file for string */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,25feb93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions
01a,08jul92,smb  written and documented.
*/

/*
DESCRIPTION

INCLUDE FILES: string.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "string.h"

/*******************************************************************************
*
* strncat - concatenate characters from one string to another (ANSI)
*
* This routine appends up to <n> characters from string <src> to the
* end of string <dst>.
*
* INCLUDE FILES: string.h
*
* RETURNS: A pointer to the null-terminated string <s1>.
*/

char * strncat
    (
    char *	 dst,  	/* string to append to */
    const char * src,   /* string to append */
    size_t	 n     	/* max no. of characters to append */
    )
    {
    if (n != 0)
	{
	char *d = dst;

	while (*d++ != EOS)			/* find end of string */
	    ;

	d--;					/* rewind back of EOS */

	while (((*d++ = *src++) != EOS) && (--n > 0))
	    ;

	if (n == 0)
	    *d = EOS;				/* NULL terminate string */
	}

    return (dst);
    }
/* strncmp.c - string compare, string */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,25feb93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions
01a,08jul92,smb  written and documented.
*/

/*
DESCRIPTION

INCLUDE FILES: string.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "string.h"


/*******************************************************************************
*
* strncmp - compare the first <n> characters of two strings (ANSI)
*
* This routine compares up to <n> characters of string <s1> to string <s2>
* lexicographically.
*
* INCLUDE FILES: string.h
*
* RETURNS:
* An integer greater than, equal to, or less than 0, according to whether
* <s1> is lexicographically greater than, equal to, or less than <s2>,
* respectively.
*/

int strncmp
    (
    const char * s1,           	/* string to compare */
    const char * s2,           	/* string to compare <s1> to */
    size_t       n             	/* max no. of characters to compare */
    )
    {
    if (n == 0)
	return (0);

    while (*s1++ == *s2++)
	{
	if ((s1 [-1] == EOS) || (--n == 0))
	    return (0);
        }

    return ((s1 [-1]) - (s2 [-1]));
    }
/* strncpy.c - string copy, string */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,25feb93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions
01a,08jul92,smb  written and documented.
*/

/*
DESCRIPTION

INCLUDE FILES: string.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "string.h"


/*******************************************************************************
*
* strncpy - copy characters from one string to another (ANSI)
*
* This routine copies <n> characters from string <s2> to string <s1>.
* If <n> is greater than the length of <s2>, nulls are added to <s1>.
* If <n> is less than or equal to the length of <s2>, the target
* string will not be null-terminated.
*
* INCLUDE FILES: string.h
*
* RETURNS: A pointer to <s1>.
*/

char *strncpy
    (
    char *      s1,   	/* string to copy to */
    const char *s2,   	/* string to copy from */
    size_t      n      	/* max no. of characters to copy */
    )
    {
    FAST char *d = s1;

    if (n != 0)
	{
	while ((*d++ = *s2++) != 0)	/* copy <s2>, checking size <n> */
	    {
	    if (--n == 0)
		return (s1);
            }

	while (--n > 0)
	    *d++ = EOS;			/* NULL terminate string */
	}

    return (s1);
    }
/* strpbrk.c - string search, string */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,25feb93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions
01a,08jul92,smb  written and documented.
*/

/*
DESCRIPTION

INCLUDE FILES: string.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "string.h"


/*******************************************************************************
*
* strpbrk - find the first occurrence in a string of a character from a given set (ANSI)
*
* This routine locates the first occurrence in string <s1> of any character
* from string <s2>.
*
* INCLUDE FILES: string.h
*
* RETURNS:
* A pointer to the character found in <s1>, or
* NULL if no character from <s2> occurs in <s1>.
*
* SEE ALSO: strcspn()
*/

char * strpbrk
    (
    const char * s1,       /* string to search */
    const char * s2        /* set of characters to look for in <s1> */
    )
    {
    char *scanp;
    int   c;
    int   sc;

    while ((c = *s1++) != 0)			/* wait until end of string */
	{
	/* loop, searching for character */

	for (scanp = CHAR_FROM_CONST(s2); (sc = *scanp++) != 0;)
	    {
	    if (sc == c)
		return (CHAR_FROM_CONST(s1 - 1));
	    }
	}

    return (NULL);
    }
/* strrchr.c - string search, string */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,25feb93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions
01a,08jul92,smb  written and documented.
*/

/*
DESCRIPTION

INCLUDE FILES: string.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "string.h"


/******************************************************************************
*
* strrchr - find the last occurrence of a character in a string (ANSI)
*
* This routine locates the last occurrence of <c> in the string pointed
* to by <s>.  The terminating null is considered to be part of the string.
*
* INCLUDE FILES: string.h
*
* RETURNS:
* A pointer to the last occurrence of the character, or
* NULL if the character is not found.
*/

char * strrchr
    (
    const char * s,         /* string to search */
    int          c          /* character to look for */
    )
    {
    char *save = NULL;

    do					/* loop, searching for character */
	{
	if (*s == (char) c)
	    save = CHAR_FROM_CONST (s);
        } while (*s++ != EOS);

    return (CHAR_FROM_CONST(save));
    }
/* strspn.c - string search, string */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,25feb93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions
01a,08jul92,smb  written and documented.
*/

/*
DESCRIPTION

INCLUDE FILES: string.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "string.h"


/******************************************************************************
*
* strspn - return the string length up to the first character not in a given set (ANSI)
*
* This routine computes the length of the maximum initial segment of
* string <s> that consists entirely of characters from the string <sep>.
*
* INCLUDE FILES: string.h
*
* RETURNS:
* The length of the string segment.
*
* SEE ALSO: strcspn()
*/
 
size_t strspn
    (
    const char * s,             /* string to search */
    const char * sep            /* set of characters to look for in <s> */
    )
    {
    const char *save;
    const char *p;
    char c1;
    char c2;

    for (save = s + 1; (c1 = *s++) != EOS; )
	for (p = sep; (c2 = *p++) != c1; )
	    {
	    if (c2 == EOS)
		return (s - save);
            }

    return (s - save);
    }
/* strstr.c - file for string */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,25feb93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions
01a,08jul92,smb  written and documented.
*/

/*
DESCRIPTION

INCLUDE FILES: string.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "string.h"


/******************************************************************************
*
* strstr - find the first occurrence of a substring in a string (ANSI)
*
* This routine locates the first occurrence in string <s>
* of the sequence of characters (excluding the terminating null character)
* in the string <find>.
*
* INCLUDE FILES: string.h
*
* RETURNS:
* A pointer to the located substring, or <s> if <find> points to a
* zero-length string, or NULL if the string is not found.
*/

char * strstr
    (
    const char * s,        /* string to search */
    const char * find      /* substring to look for */
    )
    {
    char *t1;
    char *t2;
    char c;
    char c2;

    if ((c = *find++) == EOS)		/* <find> an empty string */
	return (CHAR_FROM_CONST(s));

    FOREVER
	{
	while (((c2 = *s++) != EOS) && (c2 != c))
	    ;

	if (c2 == EOS)
	    return (NULL);

	t1 = CHAR_FROM_CONST(s);
	t2 = CHAR_FROM_CONST(find); 

	while (((c2 = *t2++) != 0) && (*t1++ == c2))
	    ;

	if (c2 == EOS)
	    return (CHAR_FROM_CONST(s - 1));
	}
    }
/* strtok.c - file for string */

/* Copyright 1992-1995 Wind River Systems, Inc. */

/*
modification history
--------------------
01d,23oct95,jdi  doc: added comment that input string will be
		    changed (SPR 4874).
01c,25feb93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions
01a,08jul92,smb  written and documented.
*/

/*
DESCRIPTION

INCLUDE FILES: string.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "string.h"


/******************************************************************************
*
* strtok - break down a string into tokens (ANSI)
*
* A sequence of calls to this routine breaks the string <string> into a
* sequence of tokens, each of which is delimited by a character from the
* string <separator>.  The first call in the sequence has <string> as its
* first argument, and is followed by calls with a null pointer as their
* first argument.  The separator string may be different from call to call.
* 
* The first call in the sequence searches <string> for the first character
* that is not contained in the current separator string.  If the character
* is not found, there are no tokens in <string> and strtok() returns a
* null pointer.  If the character is found, it is the start of the first
* token.
* 
* strtok() then searches from there for a character that is contained in the
* current separator string.  If the character is not found, the current
* token expands to the end of the string pointed to by <string>, and
* subsequent searches for a token will return a null pointer.  If the
* character is found, it is overwritten by a null character, which
* terminates the current token.  strtok() saves a pointer to the following
* character, from which the next search for a token will start.
* (Note that because the separator character is overwritten by a null
* character, the input string is modified as a result of this call.)
* 
* Each subsequent call, with a null pointer as the value of the first
* argument, starts searching from the saved pointer and behaves as
* described above.
* 
* The implementation behaves as if strtok() is called by no library functions.
*
* REENTRANCY
* This routine is not reentrant; the reentrant form is strtok_r().
*
* INCLUDE FILES: string.h
* 
* RETURNS
* A pointer to the first character of a token, or a NULL pointer if there is
* no token.
*
* SEE ALSO: strtok_r()
*/ 

char * strtok
    (
    char *       string,	/* string */
    const char * separator	/* separator indicator */
    )
    {
    static char *last = NULL;

    return (strtok_r (string, separator, &last));
    }
/* strtok_r.c - file for string */

/* Copyright 1992-1995 Wind River Systems, Inc. */

/*
modification history
--------------------
01d,23oct95,jdi  doc: added comment that input string will be
		    changed (SPR 4874).
01c,25feb93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions
01a,08jul92,smb  written and documented.
*/

/*
DESCRIPTION

INCLUDE FILES: string.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "string.h"


/*****************************************************************************
*
* strtok_r - break down a string into tokens (reentrant) (POSIX)
*
* This routine considers the null-terminated string <string> as a sequence
* of zero or more text tokens separated by spans of one or more characters
* from the separator string <separators>.  The argument <ppLast> points to a
* user-provided pointer which in turn points to the position within <string>
* at which scanning should begin.
*
* In the first call to this routine, <string> points to a null-terminated
* string; <separators> points to a null-terminated string of separator
* characters; and <ppLast> points to a NULL pointer.  The function returns a
* pointer to the first character of the first token, writes a null character
* into <string> immediately following the returned token, and updates the
* pointer to which <ppLast> points so that it points to the first character
* following the null written into <string>.  (Note that because the
* separator character is overwritten by a null character, the input string
* is modified as a result of this call.)
*
* In subsequent calls <string> must be a NULL pointer and <ppLast> must
* be unchanged so that subsequent calls will move through the string <string>,
* returning successive tokens until no tokens remain.  The separator
* string <separators> may be different from call to call.  When no token
* remains in <string>, a NULL pointer is returned.
*
* INCLUDE FILES: string.h
* 
* RETURNS
* A pointer to the first character of a token,
* or a NULL pointer if there is no token.
*
* SEE ALSO: strtok()
*/

char * strtok_r
    (
    char *       string,	/* string to break into tokens */
    const char * separators,	/* the separators */
    char **      ppLast		/* pointer to serve as string index */
    )
    {
    if ((string == NULL) && ((string = *ppLast) == NULL))
	return (NULL);

    if (*(string += strspn (string, separators)) == EOS)
	return (*ppLast = NULL);

    if ((*ppLast = strpbrk (string, separators)) != NULL)
	*(*ppLast)++ = EOS;

    return (string);
    }
/* strxfrm.c - file for string */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01d,25feb93,jdi  documentation cleanup for 5.1.
01c,20sep92,smb  documentation additions
01b,13jul92,smb  changed __cosave initialisation for MIPS.
01a,08jul92,smb  written and documented.
*/

/*
DESCRIPTION

INCLUDE FILES: string.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "limits.h"
#include "private/strxfrmP.h"


/*******************************************************************************
*
* strxfrm - transform up to <n> characters of <s2> into <s1> (ANSI)
*
* This routine transforms string <s2> and places the resulting string in <s1>.
* The transformation is such that if strcmp() is applied to two transformed
* strings, it returns a value greater than, equal to, or less than zero,
* corresponding to the result of the strcoll() function applied to the same
* two original strings.  No more than <n> characters are placed into the
* resulting <s1>, including the terminating null character.  If <n> is zero,
* <s1> is permitted to be a NULL pointer.  If copying takes place between
* objects that overlap, the behavior is undefined.
*
* INCLUDE FILES: string.h
*
* RETURNS:
* The length of the transformed string, not including the terminating null
* character.  If the value is <n> or more, the contents of <s1> are
* indeterminate.
*
* SEE ALSO: strcmp(), strcoll()
*/

size_t strxfrm
    (
    char *	 s1,		/* string out */
    const char * s2,		/* string in */
    size_t  	 n		/* size of buffer */
    )
    {
    size_t	   i;
    size_t	   nx = 0;
    const uchar_t *s = (const uchar_t *)s2;
    char	   buf[32];
    __cosave 	   state;

    /* stores state information */
    state.__state = EOS;
    state.__wchar = 0;

    while (nx < n)				/* translate and deliver */
    	{
    	i = __strxfrm (s1, &s, nx - n, &state);
    	s1 += i; 
	nx += i;

    	if ((i > 0) && (s1[-1] == EOS))
	    return (nx - 1);

    	if (*s == EOS)
	    s = (const uchar_t *) s2;
    	}

    FOREVER					/* translate the rest */ 
    	{
    	i = __strxfrm (buf, &s, sizeof (buf), &state);

    	nx += i;

    	if ((i > 0) && (buf [i - 1] == EOS))
	    return (nx - 1);

    	if (*s == EOS)
	    s = (const uchar_t *) s2;
    	}
    }

/*******************************************************************************
*
*  __strxfrm - translates string into an easier form for strxfrm() and strcoll()
*
* This routine performs the mapping as a finite state machine executing
* the table __wcstate defined in xstate.h.
*
* NOMANUAL
*/

size_t __strxfrm
    (
    char *	  	sout,		/* out string */
    const uchar_t **	ppsin,		/* pointer to character within string */
    size_t 		size,		/* size of string */
    __cosave *		ps		/* state information */
    )
    {
    const ushort_t *	stab;
    ushort_t	 	code;
    char 		state = ps->__state;	/* initial state */
    BOOL 		leave = FALSE;
    int 		limit = 0;
    int 		nout = 0;
    const uchar_t *	sin = *ppsin;		/* in string */
    ushort_t		wc = ps->__wchar;

    FOREVER					/* do state transformation */
    	{	
    	if ((_NSTATE <= state) ||
    	    ((stab = __costate.__table [state]) == NULL) ||
    	    ((code = stab [*sin]) == 0))
    	    break;		/* error */

    	state = (code & ST_STATE) >> ST_STOFF;

    	if ( code & ST_FOLD)
    		wc = wc & ~UCHAR_MAX | code & ST_CH;

    	if ( code & ST_ROTATE)
    		wc = wc >> CHAR_BIT & UCHAR_MAX | wc << CHAR_BIT;

    	if ((code & ST_OUTPUT) &&
    	    (((sout[nout++] = code & ST_CH ? code : wc) == EOS) ||
    	    (size <= nout)))
    		leave = TRUE;

    	if (code & ST_INPUT)
    	    if (*sin != EOS)
	        {
    	        ++sin; 
	        limit = 0;
	        }
    	    else
    	    	leave = TRUE;

    	if (leave)
    	    {		/* save state and return */
    	    *ppsin = sin;
    	    ps->__state = state;
    	    ps->__wchar = wc;
    	    return (nout);
    	    }
    	}

    sout[nout++] = EOS;		/* error */
    *ppsin = sin;
    ps->__state = _NSTATE;
    return (nout);
    }

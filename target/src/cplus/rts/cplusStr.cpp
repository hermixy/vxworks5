/* cplusStr.cpp - simple string class for C++ demangler */

/* Copyright 1993 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01e,17oct00,sn   backed out __cplusStr_o removal
01d,22jul99,mrs  Rewrite in C++.
01d,12may99,jku  removed __cplusStr_o
01c,03jun93,srh  doc cleanup
01b,23apr93,srh  implemented new force-link/initialization scheme
01a,31jan93,srh  written.
*/

/*
DESCRIPTION
This module provides a very simple, ring-buffer based string class.
It was written, in the absence of a more general (and more useful)
string class to support the demangler. It values speed over most
everything else. It contains no user-callable routines.

NOMANUAL
*/

/* globals */

char __cplusStr_o;

/* includes */

#include "vxWorks.h"
#include "cplusLib.h"
#include "string.h"

/*******************************************************************************
*
* RBStringIterator_T :: RBStringIterator_T - initialize an RBStringIterator
*
* RETURNS:
* nothing
*
* NOMANUAL
*/

RBStringIterator_T :: RBStringIterator_T (const RBString_T &anRBString)
    {
    theRBString = &anRBString;
    nc = anRBString.head;
    }

/*******************************************************************************
*
* RBStringIterator_T :: nextChar - advance iterator by one character
*
* RETURNS:
* next character, or EOS if end-of-string
*
* NOMANUAL
*/

char RBStringIterator_T :: nextChar ()
    {
    char returnValue = *nc;

    // If nc already points to the terminating EOS, return it (possible
    // for a second time).
    // Otherwise increment nc and see if it is still within bounds.
    // If not, wrap nc around to the beginning of the string and return
    // the first character (of the array).
    // Otherwise, dereference the pointer and return the character.
    if (*nc)
	{
	nc += 1;
	if (nc == &theRBString->data [ sizeof (theRBString->data) ])
	    {
	    nc = theRBString->data;
	    }
	}
    
    return returnValue;
    }

/*******************************************************************************
*
* RBString_T :: RBString_T - create a zero-length RBString
*
* RETURNS:
* nothing
*
* NOMANUAL
*/

RBString_T :: RBString_T ()
    {
    // String is initially empty. Start head and tail in the middle of
    // the array to minimize wraparound occasions. I do not know if this
    // affects performance one way or the other, but whenever the string
    // is contiguous in memory, i.e. not wrapped, the contents can be
    // more easily viewed when debugging client code.
    // head = tail = data + sizeof (data) / 2;
    
    head = tail = data;
    data [0] = 0;
    nChars = 0;
    }
/******************************************************************************
 *
 * RBString :: clear - reinitialize
 *
 * RETURNS: N/A
 *
 * NOMANUAL
 */
void RBString_T :: clear ()
    {
    head = tail = data;
    data [0] = 0;
    nChars = 0;
    }
/*******************************************************************************
*
* RBString_T :: RBString_T - clone an RBString
*
* RETURNS:
* nothing
*
* NOMANUAL
*/

RBString_T :: RBString_T (RBString_T & anRBString)
    {
    nChars = anRBString.nChars;
    memcpy (data, anRBString.data, sizeof (data));
    head = data + (anRBString.head - anRBString.data);
    tail = data + (anRBString.tail - anRBString.data);
    }

/*******************************************************************************
*
* RBString_T :: RBString_T - make an RBString out of a C string
*
* RETURNS:
* nothing
*
* NOMANUAL
*/

RBString_T :: RBString_T (const char * aCString)
    {
    // For most strings, which are relatively short, strlen followed
    // by memcpy should be faster than strncpy.
    nChars = min ((sizeof (data) - 1), strlen (aCString));
    memcpy (data, aCString, nChars);
    data [ nChars ] = 0;
    head = data;
    tail = & data [ nChars ];
    }

/*******************************************************************************
*
* RBString_T & RBString_T :: operator =
*
* RETURNS:
* Reference to updated RBString
*
* NOMANUAL
*/

RBString_T & RBString_T :: operator = (RBString_T & anRBString)
    {
    if (data != anRBString.data)
	{
	nChars = anRBString.nChars;
	memcpy (data, anRBString.data, sizeof (data));
	head = data + (anRBString.head - anRBString.data);
	tail = data + (anRBString.tail - anRBString.data);
	}
    return *this;
    }

/*******************************************************************************
*
* RBString_T :: operator ==
*
* RETURNS:
* TRUE if operands contain the same characters, otherwise FALSE
*
* NOMANUAL
*/

BOOL RBString_T :: operator == (RBString_T & anRBString) const
    {
    if (nChars != anRBString.nChars)
	{
	return FALSE;
	}
    else
	{
	int i;
	RBStringIterator_T s1 (*this);
	RBStringIterator_T s2 (anRBString);
	
	for (i = 0; i < nChars; i++)
	    {
	    if (s1.nextChar () != s2.nextChar ())
		{
		break;
		}
	    }
	
	return (i == nChars) ? TRUE : FALSE;
	}
    }

/*******************************************************************************
*
* RBString_T :: extractCString - extract demangled internal representation,
*                                copy to C string
*
* RETURNS:
* 0, if demangling failed, otherwise pChar
*
* NOMANUAL
*/

char * RBString_T :: extractCString
    (
     char	* pChar,			 // buffer in which to save
     						 // extracted string
     int	  length			 // size of buffer
    )
    {
    RBStringIterator_T si (*this);
    int i;
    int limit;
    char *s;
    
    for (i = 0, s = pChar, limit = min (length - 1, nChars);
	 i < limit;
	 i++, s++)
	{
	*s = si.nextChar ();
	}
    *s = 0;
    return pChar;
    }
/*******************************************************************************
*
* RBString_T :: operator != 
*
* RETURNS:
* TRUE if operands do not contain the same characters, otherwise FALSE
*
* NOMANUAL
*/

inline BOOL RBString_T :: operator != (RBString_T & anRBString ) const
    {
    return ! (*this == anRBString);
    }

/*******************************************************************************
*
* RBString_T :: appendChar - append a character
*
* RETURNS:
* EOS if character cannot be added, otherwise the appended character
*
* NOMANUAL
*/

inline char RBString_T :: appendChar (char c)
    {
    if ((nChars + 2) >= sizeof (data))
	{
	c = 0;
	}
    if (c != 0)
	{
	*tail = c;
	tail = (++tail == &data [ sizeof (data) ]) ? data : tail;
	*tail = 0;
	nChars++;
	}
    return c;
    }

/*******************************************************************************
*
* RBString_T :: prependChar - prepend a character
*
* RETURNS:
* EOS if character cannot be added, otherwise the prepended character
*
* NOMANUAL
*/

inline char RBString_T :: prependChar (char c)
    {
    if ((nChars + 2) >= sizeof (data))
	{
	c = 0;
	}
    else
	{
	head = (--head < data)  ?  &data [ sizeof (data) - 1 ] : head;
	*head = c;
	nChars++;
	}
    return c;
    }

/*******************************************************************************
*
* RBString_T :: append  - append contents of an RBString to this RBString
*
* RETURNS:
* Reference to updated RBString
*
* NOMANUAL
*/

RBString_T & RBString_T :: append (RBString_T & anRBString)
    {
    RBStringIterator_T sourceString (anRBString);
    char c;
    
    if (anRBString.nChars > 0)
	{
	while ((c = sourceString.nextChar ()))
	    {
	    appendChar (c);
	    }
	}
    return *this;
    }

/*******************************************************************************
*
* RBString_T :: append  - append contents of a C string to this RBString
*
* This function will append up to INT_MAX characters, or through the end of
* the C string. The operation terminates at the earlier of these conditions.
*
* RETURNS:
* Reference to updated RBString
*
* NOMANUAL
*/

RBString_T & RBString_T :: append (const char *aCString, int length)
    {
    for ( ; (length > 0) && (*aCString != 0); aCString++, length--)
	{
	appendChar (*aCString);
	}
    return *this;
    }

/*******************************************************************************
*
* RBString_T :: append - append a character to this RBString
*
* RETURNS:
* Reference to updated RBString
*
* NOMANUAL
*/

RBString_T & RBString_T :: append (char c)
    {
    appendChar (c);
    return *this;
    }

/*******************************************************************************
*
* RBString_T :: prepend - prepend a character to this RBString
*
* RETURNS:
* Reference to updated RBString
*
* NOMANUAL
*/

RBString_T & RBString_T :: prepend (char c)
    {
    prependChar (c);
    return *this;
    }

/*******************************************************************************
*
* RBString_T :: prepend - prepend contents of an RBString to this RBString
*
* RETURNS:
* Reference to updated RBString
*
* NOMANUAL
*/

RBString_T & RBString_T :: prepend (RBString_T & anRBString)
    {
    RBString_T tmp (anRBString);
    tmp.append (*this);
    *this = tmp;
    return *this;
    }

/*******************************************************************************
*
* RBString_T :: prepend - prepend contents of a C string to this RBString
*
* RETURNS:
* Reference to updated RBString
*
* NOMANUAL
*/

RBString_T & RBString_T :: prepend (const char *aCString, int length)
    {
    RBString_T tmp;
    tmp.append (aCString, length);
    tmp.append (*this);
    *this = tmp;
    return *this;
    }

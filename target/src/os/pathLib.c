/* pathLib.c - file/directory path library */

/* Copyright 1984-1992 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02b,01oct93,jmm  removed the last change (02a)
02a,20jul93,jmm  changed pathArrayReduce() to cope with "../../../..."
01z,18jul92,smb  Changed errno.h to errnoLib.h.
01y,04jul92,jcf  scalable/ANSI/cleanup effort.
01x,26may92,rrr  the tree shuffle
01w,20jan92,jmm  changed pathBuild to check for null dereference
01v,13dec91,gae  ANSI fixes.
01u,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -changed VOID to void
		  -changed copyright notice
01t,01oct90,dnw  fixed pathCat().
		 made entire library and all routines NOMANUAL.
01s,30jul90,dnw  changed pathLastName() back to 4.0.2 calling sequence and
		   added pathLastNamePtr() with new calling sequence.
		 added forward declaration of void routines.
01r,19jul90,kdl	 mangen fixes for backslashes.
01q,19jul90,dnw  deleted unused pathSlashIndex(); made pathSlashRindex() LOCAL
01p,18may90,llk  small tweaks to pathCat().
01o,01may90,llk  changed pathBuild() and pathCat() to check that the path names
		   they construct are within MAX_FILENAME_LENGTH chars.
		   They both return STATUS now.
		 small documentation improvements.
		 lint reduction.
01n,01apr90,llk  added buffer parameter to pathParse to improve mem usage.
		 removed pathArrayFree().
01m,14mar90,kdl  allow "\" as well as "/" for path separator, for MS-DOS.
01l,22feb90,jdi  documentation cleanup.
01k,12oct89,llk  fixed pathArrayReduce.  "../.." only went up 1 directory.
01j,12jul89,llk  lint.
01i,06jul89,llk  cut down number of malloc's to 1 in pathParse.
		 rewrote many routines.
		 changed parameters to pathBuild().
		 added pathArrayFree(), pathArrayReduce().
		 deleted pathAppend(), pathRemoveTail().
		 got NOMANUAL and LOCAL straightened out.
		 made pathLastName() return char *.
		 delinted.
01h,19oct88,llk  changed pathCat to insert a slash between a device name
		   and file name if no '/' or ':' is already there.
01g,23sep88,gae  documentation touchup.
01f,07jul88,jcf  fixed malloc to match new declaration.
01e,30jun88,llk  added pathBuild().
01d,16jun88,llk  moved pathSplit() here.
01c,04jun88,llk  cleaned a little.
01b,30may88,dnw  changed to v4 names.
01a,25may88,llk  written.
*/

/*
This library provides functions for manipulating and parsing directory
and file path names for heirarchical file systems.
The path names are UNIX style, with directory names separated by a "/".
(Optionally, a "\\" may be used; this is primarily useful for MS-DOS
files.)
The directory "." refers to the current directory.
The directory ".." refers to the parent directory.

Path names are handled in two forms:
as a null-terminated string (such as "/abc/dir/file"), or
as an array of directory names
(such as a \f2char**\fP array with the entries "abc", "dir", "file", and NULL).

SEE ALSO
ioLib(1), iosDevFind(2)

NOMANUAL
*/

/* LINTLIBRARY */

#include "vxWorks.h"
#include "iosLib.h"
#include "ioLib.h"
#include "errnoLib.h"
#include "string.h"
#include "memLib.h"
#include "pathLib.h"


/* forward static functions */

static void pathArrayReduce (char ** nameArray);
static char *strcatlim (char *s1, char *s2, int limit);
static char *pathSlashRindex (char *pString);


/*******************************************************************************
*
* pathParse - parse a full pathname into an array of directory/file names
*
* Parses a UNIX style directory or file name which has directory names
* separated by '/'s.  It copies the directory and file names, separated by
* EOSs, into the user supplied buffer. The buffer is filled with "." for a
* null path name.
* The last entry in the returned array will be set to NULL.
* It is assumed that the caller has created a buffer large enough to
* contain the parsed pathname.
*
* For instance, "/usr/vw/dir/file" gets parsed into
*
*                          nameArray
*                         |---------|
*   ---------------------------o    |
*   |                     |---------|
*   |   -----------------------o    |
*   |   |                 |---------|
*   |   |  --------------------o    |
*   |   |  |              |---------|
*   |   |  |   ----------------o    |
*   |   |  |   |          |---------|
*   v   v  v   v          |   NULL  |
*  |----------------|     |---------|
*  |usr vw dir file |
*  |----------------|
*    nameBuf
*
* RETURNS: parsed directory name returned in `nameBuf' pointed to by `nameArray'
*
* NOMANUAL
*/

void pathParse
    (
    char *longName,     /* full path name */
    char **nameArray,   /* resulting array of pointers to directory names */
    char *nameBuf       /* buffer containing resulting dir names pointed
                              to by nameArray pointers */
    )
    {
    FAST char *pName = nameBuf; /* ptr to name in nameArray */
    FAST char *p0;		/* ptr to character in longName */
    FAST char *p1;		/* ptr to character in longName */
    int nameCount = 0;		/* # of names encountered so far */
    int nChars;			/* # of characters in a name */

    p0 = p1 = longName;

    /*
     * Copy names delimitted by '/'s and '\'s to array of strings.
     * nameArray entries point to strings
     */

    while (*p0 != EOS)
        {
        while ((*p1 != '/') && (*p1 != '\\') && (*p1 != EOS))
            p1++;

        if (p0 == p1)   /* p1 hasn't moved forward, probably hit another '/' */
            {
            p0++; p1++;
            continue;
            }
        nChars = p1 - p0;		/* how many characters in dir name? */

        bcopy (p0, pName, nChars);	/* copy dir name to buffer */
        pName [nChars] = EOS;		/* terminate with EOS */
        nameArray [nameCount] = pName;	/* index into buffer with nameArray */
	pName += nChars + 1;		/* move ptr to next open spot in buf */

        if ((*p1 == '/') || (*p1 == '\\'))
            p1++;
        p0 = p1;
        nameCount++;
        } /* while */

    /* return "." (current directory) if no directories were specified */

    if (nameCount == 0)
        {
        (void) strcpy (nameBuf, ".");
        *nameArray = nameBuf;
        nameCount++;
        }

    nameArray [nameCount] = NULL;
    }

/*******************************************************************************
*
* pathCondense - condense a file or directory path
*
* This routine condenses a path name by removing any occurrences of:
*
* .RS
*  /../
*  /./
*  //
* .RE
*
* NOMANUAL
*/

void pathCondense
    (
    FAST char *pathName         /* directory or file path */
    )
    {
    char *nameArray  [MAX_DIRNAMES];
    char nameBuf     [MAX_FILENAME_LENGTH];
    char newPathName [MAX_FILENAME_LENGTH];
    char *pTail;

    /* parse the stuff after the device name */

    (void) iosDevFind (pathName, &pTail);

    /* parse path name into individual dir names -- has the added
     * benefit of eliminating "//"s */

    pathParse (pTail, nameArray, nameBuf);

    /* reduce occurances of ".." and "." */

    pathArrayReduce (nameArray);

    /*
     * Rebuild path name.  Start with an empty path name.
     * Preserve initial '/' if necessary
     */

    if ((*pTail == '/') || (*pTail == '\\'))
	{
	newPathName [0] = *pTail;
	newPathName [1] = EOS;
	}
    else
	newPathName [0] = EOS;

    /*
     * pathBuild() will return ERROR if newPathName gets too big.
     * We're assuming it won't here, since it has either stayed the
     * same size or been reduced.
     */

    (void) pathBuild (nameArray, (char **) NULL, newPathName);

    /* newPathName should be the same size or smaller than pathName */

    (void) strcpy (pTail, newPathName);
    }

/*******************************************************************************
*
* pathArrayReduce - reduce the number of entries in a nameArray by
*                   eliminating occurances of "." and "..".
*
* pathArrayReduce eliminates occurances of "." and ".." in a nameArray.
* Each instance of "." is set to EOS.  Each instance of ".." and the previous
* directory name is set to EOS.
*/

LOCAL void pathArrayReduce
    (
    char **nameArray    /* array of directory names, last entry is NULL */
    )
    {
    FAST char **pArray = nameArray;
    FAST char **ppPrevString;
    FAST char *pString;

    while ((pString = *pArray) != NULL)
        {
        if (strcmp (pString, ".") == 0)
            {
            /* "." means same directory, ignore */

            *pString = EOS;
            }
        else if (strcmp (pString, "..") == 0)
            {
            /* up one directory and down one,
             * ignore this directory and previous non-nullified directory
             */

            *pString = EOS;	/* clear out this directory name */

	    ppPrevString = pArray - 1;
	    while (ppPrevString >= nameArray)
		{
		/* find last non-nullified string */

		if (**ppPrevString == EOS)
		    ppPrevString--;
		else
		    {
		    **ppPrevString = EOS;
		    break;
		    }
		}
            }
        pArray++;
        } /* while */
    }

/*******************************************************************************
*
* pathBuild - generate path name by concatenating a list of directory names
*
* This routine uses an array of names to build a path name.  It appends the
* names in nameArray to the given destination name destString.  It
* successively uses the names in nameArray up to but not including
* nameArrayEnd, or up to a * NULL entry in the nameArray, whichever
* comes first.
*
* To use the entire path name array, pass NULL for nameArrayEnd.
* destString must be NULL terminated before calling pathBuild.
*
* It assumes that destString should end up no longer than MAX_FILENAME_LENGTH.
*
* RETURNS:
* OK    and copies resulting path name to <destString>
* ERROR if resulting path name would have more than MAX_FILENAME_LENGTH chars.
*
* NOMANUAL
*/

STATUS pathBuild
    (
    FAST char **nameArray,      /* array of directory names */
    char **nameArrayEnd,        /* ptr to first name in array which should
                                 * NOT be added to path     */
    FAST char *destString       /* destination string       */
    )
    {
    FAST char **pp;
    int len = strlen (destString);
    int newlen = 0;
    BOOL slashEnd = FALSE;		/* Does destString end with a slash? */

    if (len >= MAX_FILENAME_LENGTH)
	{
	errnoSet (S_ioLib_NAME_TOO_LONG);
	return (ERROR);
	}

    if ((len > 0) && (destString [len - 1] != '/')
	&& (destString [len - 1] != '\\'))
	{
	/* if string contains part of a path name and it doesn't end with a '/'
	 * or '\', then slash terminate it */

	destString [len] = '/';
	destString [len + 1] = EOS;
	slashEnd = TRUE;
	}

    /*
     * Added a test to make sure pp is non-NULL before checking
     * that *pp is not EOS.
     */

    for (pp = nameArray; (pp != NULL) && (*pp != NULL)
	 && (pp != nameArrayEnd); pp++)
	{
	if (**pp != EOS)
	    {
	    if (newlen = (len + strlen (*pp) + 1) > MAX_FILENAME_LENGTH)
		{
		errnoSet (S_ioLib_NAME_TOO_LONG);
		return (ERROR);
		}

	    (void) strcat (destString, *pp);
	    (void) strcat (destString, "/");
	    slashEnd = TRUE;
	    len = newlen;
	    }
	}

    /* final string should not end with '/' */

    if (slashEnd)
	destString [strlen (destString) - 1] = EOS;
    return (OK);
    }

/*******************************************************************************
*
* pathCat - concatenate directory path to filename
*
* This routine constructs a valid filename from a directory path and a filename.
* The resultant path name is put into <result>.
*
* The following arcane rules are used to derive the concatenated result.
*
* 1. if <fileName> begins with a device name, then <result> will just be
*    <fileName>.
* 2. if <dirName> begins with a device name that does NOT start with a "/",
*    then that device name is first part of <result>.
* 3. if <fileName> is a relative path name, i.e. does not start with
*    '/', '\\', '~', or '$', then <dirName> (less the device name of rule 2,
*    if any) is the appended to <result>.  A '/' is appended to the result
*    unless <dirName> already ends with one.
* 4. <fileName> is appended to <result>.
*
* Thus, if the following are the only devices in the system:
*
*     dev:
*     /usr
*     /rt11/
*
* Then the following results would be obtained:
*
* dirName	fileName	result		rule
* -------	---------	-------------	---------------------------
* (any)		yuba:blah	yuba:blah	fileName starts with device
* (any)		/usr/blah	/usr/blah	fileName starts with device
* dev:		blah		dev:blah	<dev><file>
* dev:		/blah		dev:/blah	<dev><file>
* dev:foo	blah		dev:foo/blah	<dev><dir>/<file>
* dev:foo	/blah		dev:/blah	<dev><file>
* /usr		blah		/usr/blah	</dir>/<file>
* /usr		/blah		/blah		<file>
* /rt11/	blah		/rt11/blah	</dir/><file>
* /rt11/	/blah		/blah		<file>
*
* <dirName> and <fileName> should be less than MAX_FILENAME_LENGTH chars long.
*
* RETURNS: OK
*
* RATIONALE
*
* These rules are intended to make pathnames work as in UNIX for devices
* that start with "/", but also to allow DOS-like device prefixes and
* rcp-like host prefixes, and also to retain compatibility with old
* rt-11 conventions.
* That these rules are so complex suggests that adopting a simpler,
* more UNIX-like model would be better in the future.
* For example, if device names were required to start with '/' and NOT
* end in '/', then the following simple UNIX-like rules would suffice:
*
* 1. if <fileName> starts with "/" (i.e. is an absolute pathname), then
*    the result is <filename>
* 2. otherwise the result is <dirName>/<fileName>
*
* NOMANUAL
*/

STATUS pathCat
    (
    FAST char *dirName,         /* directory path */
    FAST char *fileName,        /* filename to be added to end of path */
    FAST char *result           /* where to form concatenated name */
    )
    {
    char *pTail;

    if ((fileName == NULL) || (fileName[0] == EOS))
	{
	strcpy (result, dirName);
	return (OK);
	}

    if ((dirName == NULL) || (dirName[0] == EOS))
	{
	strcpy (result, fileName);
	return (OK);
	}

    /* if filename starts with a device name, result is just filename */

    (void) iosDevFind (fileName, &pTail);
    if (pTail != fileName)
	{
	strcpy (result, fileName);
	return (OK);
	}

    /* if dirName starts with a device name that does NOT start with "/",
     * then prepend that device name
     */

    *result = EOS;

    if (dirName[0] != '/')
	{
	(void) iosDevFind (dirName, &pTail);
	if (pTail != dirName)
	    {
	    strncat (result, dirName, pTail - dirName);
	    dirName = pTail;
	    }
	}

    /* if filename is relative path, prepend directory if any */

    if ((index ("/\\~$", fileName [0]) == NULL) && (dirName[0] != EOS))
	{
	strcatlim (result, dirName, PATH_MAX);

	if (dirName [strlen (dirName) - 1] != '/')
	    strcatlim (result, "/", PATH_MAX);
	}

    /* concatenate filename */

    strcatlim (result, fileName, PATH_MAX);

    return (OK);
    }

/*******************************************************************************
*
* strcatlim - append characters of one string onto another up to limit
*
* This routine appends characters from string <s2> to the end of string <s1>,
* limiting the resulting length of <s1> to <limit> characters, not including
* the EOS.  The resulting <s1> will always be NULL terminated.
* Thus <s1> must be big enough to hold <limit> + 1 characters.
*
* INTERNAL
* Perhaps should be in strLib.
*
* RETURNS: A pointer to null-terminated <s1>.
*
*/

LOCAL char *strcatlim
    (
    FAST char *s1,      /* string to be appended to */
    FAST char *s2,      /* string to append to s1 */
    int limit           /* max number of chars in resulting s1, excluding EOS */
    )
    {
    int n1;
    int n2;

    n1 = strlen (s1);

    if (n1 < limit)
	{
	n2 = strlen (s2);
	n2 = min (n2, limit - n1);

	bcopy (s2, &s1 [n1], n2);
	s1 [n1 + n2] = EOS;
	}

    return (s1);
    }

/*******************************************************************************
*
* pathLastNamePtr - return last name in a path name
*
* This routine returns a pointer to the character after the last slash
* ("/" or "\\") in the path name.  Note that if the directory name ends
* with a slash, the returned pointer will point to a null string.
*
* RETURNS: A pointer to the last name in path.
*
* NOMANUAL
*/

char *pathLastNamePtr
    (
    char *pathName
    )
    {
    char *p;

    if ((p = pathSlashRindex (pathName)) == NULL)
	return (pathName);
    else
	return ((char *) (p + 1));
    }

/*******************************************************************************
*
* pathLastName - return last name in a path name
*
* This routine returns a pointer to the character after the last slash
* ("/" or "\\") in the path name.  Note that if the directory name ends
* with a slash, the returned pointer will point to a null string.
*
* This routine is provided in addition to pathLastNamePtr() just for
* backward compatibility with 4.0.2.  pathLastNamePtr() is the preferred
* routine now.
*
* RETURNS: pointer to last name in location pointed to by <pLastName>
*
* NOMANUAL
*/

void pathLastName
    (
    char *pathName,
    char **pLastName
    )
    {
    *pLastName = pathLastNamePtr (pathName);
    }

/*******************************************************************************
*
* pathSlashRindex - find last occurance of '/' or '\' in string
*
* This routine searches a string for the last occurance of '/' or '\'.
* It returns a pointer to the last slash character.
*
* RETURNS:
*    pointer to last '/' or '\', or
*    NULL if no slashes in string
*/

LOCAL char *pathSlashRindex
    (
    char *pString               /* string to search */
    )
    {
    FAST char *pForward;	/* ptr to forward slash */
    FAST char *pBack;		/* ptr to back slash */


    pForward = rindex (pString, '/');
    pBack    = rindex (pString, '\\');

    return (max (pForward, pBack));
    }

/*******************************************************************************
*
* pathSplit - split a path name into its directory and file parts
*
* This routine splits a valid UNIX-style path name into its directory
* and file parts by finding the last slash ("/" or "\\").  The results are
* copied into <dirName> and <fileName>.
*
* NOMANUAL
*/

void pathSplit
    (
    FAST char *fullFileName,    /* full file name being parsed */
    FAST char *dirName,         /* result, directory name */
    char *fileName              /* result, simple file name */
    )
    {
    FAST int nChars;

    if (fullFileName != NULL)
	{
	char *p = pathSlashRindex (fullFileName);

	if (p == NULL)
	    {
	    (void) strcpy (fileName, fullFileName);
	    (void) strcpy (dirName, "");
	    }
	else
	    {
	    nChars = p - fullFileName;
	    (void) strncpy (dirName, fullFileName, nChars);
	    dirName [nChars] = EOS;
	    (void) strcpy (fileName, ++p);
	    }
	}
    else
	{
	(void) strcpy (fileName, "");
	(void) strcpy (dirName, "");
	}
    }

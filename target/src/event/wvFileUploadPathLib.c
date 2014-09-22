/* wvFileUploadPathLib.c -  file destination for event data */

/* Copyright 1997 Wind River Systems, Inc. */
/*
modification history
--------------------
01f,28aug98,dgp  FCS man page edit
01e,08may98,dgp  clean up man pages for WV 2.0 beta release
01d,15apr98,cth  removed debug print statement, removed errno set
01c,27jan97,cth  added openFlags arg to create, added fileUpPathDefaultPerm
01b,18dec97,cth  changed this file's name to from wvFileUploadPath.c to
                 wvFileUploadPathLib.c, added fileUploadPathLibInit,
		 updated include files
01a,21nov97,cth  written, taken from evtSockLib.c
*/

/*
DESCRIPTION
This file contains routines that write events to a file rather than
uploading them to the host using a type of socket connection.  If the file
indicated is a TSFS file, this routine has the same result as uploading to a
host file using other methods, allowing it to replace evtRecv.  The file can be
created anywhere, however, and event data can be kept on the target if
desired.

INCLUDE FILES:

SEE ALSO: wvSockUploadPathLib, wvTsfsUploadPathLib
*/

#include "vxWorks.h"
#include "errno.h"
#include "ioLib.h"
#include "fcntl.h"
#include "stdlib.h"
#include "logLib.h"

#include "private/wvUploadPathP.h"
#include "private/wvFileUploadPathLibP.h"



typedef struct fileUploadPath  	/* FILE_UPLOAD_DESC */
    {
    UPLOAD_DESC path;		/* struct must begin with this descriptor */
    int		fileFd;		/* private fd for each upload path */
    } FILE_UPLOAD_DESC;


/* globals */

int fileUpPathDefaultPerm = 0644;




/*******************************************************************************
*
* fileUploadPathLibInit - initialize the wvFileUploadPathLib library (Windview)
*
* This routine initializes the library by pulling in the routines in this 
* file for use with WindView.  It is called during system configuration 
* from usrWindview.c.
*
* RETURNS: OK.
*
*/

STATUS fileUploadPathLibInit (void)
    {
    return OK;
    }

/*******************************************************************************
*
* fileUploadPathCreate - create a file for depositing event data (Windview)
*
* This routine opens and initializes a file to receive uploaded events.  
* The <openFlags> argument is passed on as the flags argument to the actual 
* open call so that the caller can specify things like O_TRUNC and O_CREAT.
* The file is always opened as O_WRONLY, regardless of the value of <openFlags>.
* 
* RETURNS: The UPLOAD_ID, or NULL if the file can not be opened or 
* memory for the ID is not available.
*
* SEE ALSO: fileUploadPathClose()
*/

UPLOAD_ID fileUploadPathCreate 
    (
    char *fname,				/* name of file to create */
    int   openFlags				/* O_CREAT, O_TRUNC */
    )
    {
    FILE_UPLOAD_DESC   *pFileUploadDesc;	/* this socket's descriptor */

    /* Allocate the upload path's descriptor. */

    if ((pFileUploadDesc = (FILE_UPLOAD_DESC *)
                           malloc (sizeof (FILE_UPLOAD_DESC))) == NULL)
        {
        logMsg ("fileUploadPathCreate: failed to allocate upload descriptor.\n",
                0, 0, 0, 0, 0, 0);
        return (NULL);
        }

    /* Open the file for writing only, maintaining O_CREAT, O_TRUNC. */

    openFlags &= ~O_RDONLY;
    openFlags &= ~O_RDWR;
    openFlags |=  O_WRONLY;

    if ((pFileUploadDesc->fileFd = open (fname, openFlags, 
					 fileUpPathDefaultPerm)) == ERROR)
        {
        logMsg ("fileUploadPathCreate: failed to open file (%s).\n", 
		(int) fname, 0, 0, 0, 0, 0);
        return (NULL);
        }

    /* Fill in the file's upload routines so the uploader can access them. */

    pFileUploadDesc->path.writeRtn = (FUNCPTR) fileUploadPathWrite;
    pFileUploadDesc->path.errorRtn = (FUNCPTR) fileUploadPathClose;

    /* Cast the FILE_UPLOAD_DESC to a generic UPLOAD_DESC before returning. */

    return ((UPLOAD_ID) pFileUploadDesc);
    }

/*******************************************************************************
*
* fileUploadPathClose - close the event-destination file (WindView)
*
* This routine closes the file associated with <pathId> that is serving
* as a destination for event data.
*
* RETURNS: N/A
* 
* SEE ALSO: fileUploadPathCreate()
*/

void fileUploadPathClose 
    (
    UPLOAD_ID pathId			/* generic upload-path descriptor */
    )
    {
    FILE_UPLOAD_DESC *pFileUploadDesc;	/* pathId cast to see private data */

    if (pathId == NULL)
        return;

    pFileUploadDesc = (FILE_UPLOAD_DESC *) pathId;
    close (pFileUploadDesc->fileFd);
    free (pFileUploadDesc);
    }

/*******************************************************************************
*
* fileUploadPathWrite - write to the event-destination file (WindView)
*
* This routine writes <size> bytes of data beginning at <pStart> to the file
* indicated by <pathId>. 
*
* RETURNS: The number of bytes written, or ERROR.
*
*/

int fileUploadPathWrite
    (
    UPLOAD_ID   pathId,                 /* generic upload-path descriptor */
    char *      pStart,                 /* address of data to write */
    size_t      size                    /* number of bytes of data at pStart */
    )
    {
    if (pathId == NULL)
        return (ERROR);

    return (write (((FILE_UPLOAD_DESC *) pathId)->fileFd, pStart, size));
    }


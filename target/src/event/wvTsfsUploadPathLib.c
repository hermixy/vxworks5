/* wvTsfsUploadPathLib.c - target host connection library using TSFS */

/* Copyright 1998 Wind River Systems, Inc. */
/*
modification history
--------------------
02i,28aug98,dgp  FCS man page edit
02h,08may98,dgp  clean up man pages for WV 2.0 beta release
02g,15apr98,cth  removed errno set
02f,02apr98,cjtc extended size of fname in tsfsUploadPathCreate.
		 Removed erroneous call to htons in tsfsUploadPathCreate
02e,20mar98,cth  removed debug print statements
02d,27jan98,cth  removed oob error indicators, removed tsfsUploadPathError
02c,18dec97,cth  renamed again to wvTsfsUploadPathLib.c from wvTsfsUploadPath.c,
                 added tsfsUploadPathLibInit, updated included files
02b,16nov97,cth  renamed again to wvTsfsUploadPath.c from tsfsUploadPath.c
                 changed include tsfsUploadPathP.h to wvTsfsUploadPathP.h
02a,16nov97,cth  rewritten for WV2.0, modhist restarted to 'a'
		 renamed from evtTsfsSockLib.c to tsfsUploadPath.c
01f,21aug97,cth  created, modified evtSockLib.c
*/

/*
DESCRIPTION
This library contains routines that are used by wvLib to transfer event
data from the target to the host.  This transfer mechanism uses the socket
functionality of the Target Server File System (TSFS), and can therefore be
used without including any socket or network facilities within the target.

INTERNAL
Each open connection is referenced by a pointer to a TSFS_UPLOAD_DESC.
This pointer is returned by tsfsUploadPathCreate when the connection is
created successfully. The TSFS_UPLOAD_DESC structure must begin  with the
UPLOAD_DESC structure defined in wvUploadPath.h.  This is analagous to the
DEV_HDR mechanism used in iosLib.  Information private to this library is
maintained in the remainder of the TSFS_UPLOAD_DESC structure (e.g. the
socket fd).  When an operation such as tsfsUploadPathRead is performed, it
receives an UPLOAD_ID (a pointer to an UPLOAD_PATH) that must be cast into
a TSFS_UPLOAD_DESC to see the private information.

INCLUDE FILES:

SEE ALSO: wvSockUploadPathLib, wvFileUploadPathLib
*/

#include "vxWorks.h"
#include "ioLib.h"
#include "logLib.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "wdb/wdbVioLib.h"

#include "private/wvUploadPathP.h"
#include "private/wvTsfsUploadPathLibP.h"


typedef struct tsfsUploadPath	/* TSFS_UPLOAD_DESC */
    {
    UPLOAD_DESC	path;		/* struct must begin with this descriptor */
    int		sockFd;		/* private data for each open socket */
    } TSFS_UPLOAD_DESC;


/*******************************************************************************
*
* tsfsUploadPathLibInit - initialize wvTsfsUploadPathLib library (Windview)
*
* This routine initializes wvTsfsUploadPathLib by pulling in the
* routines in this file for use with WindView.  It is called during system
* configuration from usrWindview.c.
*
* RETURNS: OK.
*
*/

STATUS tsfsUploadPathLibInit (void)
    {
    return OK;
    }

/*******************************************************************************
*
* tsfsUploadPathCreate - open an upload path to the host using a TSFS socket (Windview)
*
* This routine opens a TSFS socket to the host to be used for uploading
* event data.  After successfully establishing this connection, an UPLOAD_ID 
* is returned which points to the TSFS_UPLOAD_DESC that is passed to 
* open(), close(), read(), etc. for future operations.
*
* RETURNS: The UPLOAD_ID, or NULL if the connection cannot be completed or 
* not enough memory is available.
*
* SEE ALSO: tsfsUploadPathClose()
*/

UPLOAD_ID tsfsUploadPathCreate 
    (
    char *ipAddress, 		/* server's IP address in .-notation */
    short port			/* port number to bind to */
    )
    {
    char 		fName[64]; 	 /* holds tsfs path & file name */
    TSFS_UPLOAD_DESC   *pTsfsUploadDesc; /* this socket's descriptor */

    /* Allocate the upload path's descriptor. */

    if ((pTsfsUploadDesc = (TSFS_UPLOAD_DESC *) 
			   malloc (sizeof (TSFS_UPLOAD_DESC))) == NULL)
        {
	logMsg ("tsfsUploadPathCreate: failed to allocate upload descriptor.\n",
		0, 0, 0, 0, 0, 0);
	return (NULL);
	}

    /*
     * Open a socket through the target-server file system, with a file 
     * name like "TCP:host:port".  Mode and permissions are ignored.
     */

    sprintf (fName, "/tgtsvr/TCP:%s:%d", ipAddress, port);

    if ((pTsfsUploadDesc->sockFd = open (fName, 0, 0)) == ERROR)
	{
	logMsg ("tsfsUploadPathCreate: failed to open socket.\n",
		0, 0, 0, 0, 0, 0);
        return (NULL);
	}

    /* Fill in the tsfs upload routines so the uploader can access them. */

    pTsfsUploadDesc->path.writeRtn = (FUNCPTR) tsfsUploadPathWrite;
    pTsfsUploadDesc->path.errorRtn = (FUNCPTR) tsfsUploadPathClose;

    /* Cast the TSFS_UPLOAD_DESC to a generic UPLOAD_DESC before returning */

    return ((UPLOAD_ID) pTsfsUploadDesc);
    }

/*******************************************************************************
*
* tsfsUploadPathClose - close the TSFS-socket upload path (Windview)
*
* This routine closes the TSFS-socket connection to the event receiver on 
* the host.
*
* RETURNS: N/A
*
* SEE ALSO: tsfsUploadPathCreate()
*/

void tsfsUploadPathClose 
    (
    UPLOAD_ID upId			/* generic upload-path descriptor */
    )
    {
    TSFS_UPLOAD_DESC *pTsfsUploadDesc; 	/* upId cast to see private data */

    if (upId == NULL)
	return;

    pTsfsUploadDesc = (TSFS_UPLOAD_DESC *) upId;
    close (pTsfsUploadDesc->sockFd);
    free (pTsfsUploadDesc);
    }

/*******************************************************************************
*
* tsfsUploadPathWrite - write to the TSFS upload path (Windview)
*
* This routine writes <size> bytes of data beginning at <pStart> to the upload
* path connecting the target with the host receiver.
* 
* RETURNS: The number of bytes written, or ERROR.
*
* SEE ALSO: tsfsUploadPathCreate()
*/

int tsfsUploadPathWrite
    (
    UPLOAD_ID	upId,			/* generic upload-path descriptor */
    char *	pStart,			/* address of data to write */
    size_t	size			/* number of bytes of data at pStart */
    )
    {
    TSFS_UPLOAD_DESC  *pTsfsUploadDesc; /* upId cast to see private data */

    if (upId == NULL)
	return (ERROR);

    pTsfsUploadDesc = (TSFS_UPLOAD_DESC *)upId;
    return (write (pTsfsUploadDesc->sockFd, pStart, size));
    }

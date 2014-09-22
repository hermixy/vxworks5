/* wvSockUploadPathLib.c -  socket upload path library */

/* Copyright 1998 Wind River Systems, Inc. */
/*
modification history
--------------------
02i,28aug98,dgp  FCS man page edit
02h,06aug98,cth  added ability to create path with host name
02g,08may98,dgp  clean up man pages for WV 2.0 beta release
02f,15apr98,cth  removed errno set
02e,20mar98,cth  removed debug print statements
02d,27jan98,cth  removed oob error indications, removed sockUploadPathError,
		 changed SOSENDBUFSIZE to sockUpPathSendBufSize
02c,19dec97,cth  renamed again from wvSockUploadPath.c to wvSockUploadPathLib.c,
                 added sockUploadPathLibInit, updated include files
02b,16nov97,cth  renamed again from sockUploadPath.c to wvSockUploadPath.c
                 changed include sockUploadPathP.h to wvSockUploadPathP.h
02a,16nov97,cth  rewritten for WV2.0
		 renamed file to sockUploadPath.c from evtSockLib.c
01g,21aug97,cth  reverted functionality to 01e, TSFS support now in
		 evtTsfsSockLib.c
01f,18aug97,cth  added support for upload through TSFS
01e,31jul97,nps  WindView 2.0 - evtSockInit now passes fd to event rBuff.
01d,22feb94,smb  corrected Copyright date (SPR #2910)
01c,21jan94,maf  shut off event logging when write() to event socket fails
		   (SPR #2805).
		 handle case of write() writing fewer bytes than requested
		   in evtSockDataTransfer().
                 other minor tweaks.
01b,18jan94,maf  evtSockError() now closes event stream socket (part of fix
		   for SPR #2800).
01a,10dec93,smb  created.
*/

/*
DESCRIPTION
This file contains routines that are used by wvLib to pass event data from
the target buffers to the host.  This particular event-upload path opens
a normal network socket connected with the WindView host process to 
transfer the data.

INCLUDE FILES:

SEE ALSO:  wvTsfsUploadPathLib, wvFileUploadPathLib

*/

#include "vxWorks.h"
#include "fcntl.h"
#include "stdlib.h"
#include "in.h"
#include "inetLib.h"
#include "ioLib.h"
#include "logLib.h"
#include "nfsLib.h"
#include "socket.h"
#include "sockLib.h"
#include "hostLib.h"
#include "string.h"
#include "sys/socket.h"

#include "private/wvUploadPathP.h"
#include "private/wvSockUploadPathLibP.h"


typedef struct sockUploadPath  	/* SOCK_UPLOAD_DESC */
    {
    UPLOAD_DESC path;		/* struct must begin with this descriptor */
    int		sockFd;		/* private fd for each upload path */
    } SOCK_UPLOAD_DESC;


/* globals */

int sockUpPathSendBufSize = (64 * 1024 >> 1);


/*******************************************************************************
*
* sockUploadPathLibInit - initialize wvSockUploadPathLib library (Windview)
*
* This routine initializes wvSockUploadPathLib by pulling in the
* routines in this file for use with WindView.  It is called during system
* configuration from usrWindview.c.
*
* RETURN: OK.
*
*/

STATUS sockUploadPathLibInit (void)
    {
    return OK;
    }

/*******************************************************************************
*
* sockUploadPathCreate - establish an upload path to the host using a socket (Windview)
*
* This routine initializes the TCP/IP connection to the host process that 
* receives uploaded events.  It can be retried if the connection attempt fails.
*
* RETURNS: The UPLOAD_ID, or NULL if the connection cannot be completed or 
* memory for the ID is not available.
*
* SEE ALSO: sockUploadPathClose()
*/

UPLOAD_ID sockUploadPathCreate 
    (
    char *ipAddress,	    /* server's hostname or IP address in .-notation */
    short port		    /* port number to bind to */
    )
    {
    SOCK_UPLOAD_DESC   *pSockUploadDesc;	/* this socket's descriptor */
    struct sockaddr_in	sin;			/* address of server */

    /* Allocate the upload path's descriptor. */

    if ((pSockUploadDesc = (SOCK_UPLOAD_DESC *)
                           malloc (sizeof (SOCK_UPLOAD_DESC))) == NULL)
        {
        logMsg ("sockUploadPathCreate: failed to allocate upload descriptor.\n",
                0, 0, 0, 0, 0, 0);
        return (NULL);
        }

    /* Open the upload socket. */

    if ((pSockUploadDesc->sockFd = socket (AF_INET, SOCK_STREAM, 
 					  IPPROTO_TCP)) == ERROR)
        {
        logMsg ("sockUploadPathCreate: failed to open socket.\n",
                0, 0, 0, 0, 0, 0);
        return (NULL);
        }

    /* Increase the socket's send-buffer size. */

    if (setsockopt (pSockUploadDesc->sockFd, SOL_SOCKET, 
                    SO_SNDBUF, (char *) & sockUpPathSendBufSize, 
                    sizeof (sockUpPathSendBufSize)) == ERROR)
        {
        logMsg ("sockUploadPathCreate: setsockopt failed\n",
                0, 0, 0, 0, 0, 0);
	close (pSockUploadDesc->sockFd);
        return (NULL);
        }

    /* Fill in the server's address before connection. */

    bzero ((char *) &sin, sizeof (sin));
    sin.sin_family      = AF_INET;
    sin.sin_port        = htons (port);

    if ((sin.sin_addr.s_addr = hostGetByName (ipAddress)) == ERROR &&
        (sin.sin_addr.s_addr = inet_addr (ipAddress)) == ERROR)
        {
        logMsg ("sockUploadPathCreate: failed to get inet addr for (%s)\n",
                (int) ipAddress, 0, 0, 0, 0, 0);
	close (pSockUploadDesc->sockFd);
        return (NULL);
	}

    /* Connect to host (server). */

    if (connect (pSockUploadDesc->sockFd, (struct sockaddr *)&sin, 
                 sizeof (sin)) == ERROR)
        {
        logMsg ("sockUploadPathCreate: connect failed\n",
                0, 0, 0, 0, 0, 0);
	close (pSockUploadDesc->sockFd);
        return (NULL);
        }

    /* Fill in the socket upload routines so the uploader can access them. */

    pSockUploadDesc->path.writeRtn = (FUNCPTR) sockUploadPathWrite;
    pSockUploadDesc->path.errorRtn = (FUNCPTR) sockUploadPathClose;

    /* Cast the SOCK_UPLOAD_DESC to a generic UPLOAD_DESC before returning. */

    return ((UPLOAD_ID) pSockUploadDesc);
    }

/*******************************************************************************
*
* sockUploadPathClose - close the socket upload path (Windview)
*
* This routine closes the socket connection to the event
* receiver on the host.
*
* RETURNS: N/A
*
* SEE ALSO: sockUploadPathCreate()
*/

void sockUploadPathClose 
    (
    UPLOAD_ID upId			/* generic upload-path descriptor */
    )
    {
    SOCK_UPLOAD_DESC *pSockUploadDesc;	/* upId cast to see private data */

    if (upId == NULL)
        return;

    pSockUploadDesc = (SOCK_UPLOAD_DESC *) upId;
    close (pSockUploadDesc->sockFd);
    free (pSockUploadDesc);
    }

/*******************************************************************************
*
* sockUploadPathWrite - write to the socket upload path (Windview)
*
* This routine writes <size> bytes of data beginning at <pStart> to the upload
* path between the target and the event receiver on the host.
*
* RETURNS: The number of bytes written, or ERROR.
*
* SEE ALSO: sockUploadPathCreate()
*/

int sockUploadPathWrite
    (
    UPLOAD_ID   upId,                   /* generic upload-path descriptor */
    char *      pStart,                 /* address of data to write */
    size_t      size                    /* number of bytes of data at pStart */
    )
    {
    SOCK_UPLOAD_DESC  *pSockUploadDesc; /* upId cast to see private data */

    if (upId == NULL)
        return (ERROR);

    /* Cast upId to a SOCK_UPLOAD_DESC for access to private data. */

    pSockUploadDesc = (SOCK_UPLOAD_DESC *)upId;

    return (write (pSockUploadDesc->sockFd, pStart, size));
    }


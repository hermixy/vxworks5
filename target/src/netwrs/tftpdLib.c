/* tftpdLib.c - Trivial File Transfer Protocol server library */

/* Copyright 1992 - 2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01l,07may02,kbw  man page edits
01k,15oct01,rae  merge from truestack ver 01n, base 01i (SPR #69222 etc.)
01j,09jan01,dgr  Adding to comment-header-block of tftpdInit as per SPR #63337
01i,09oct97,nbs  modified tftpd to use filename from TFTP_DESC, spr # 9413
01h,01aug96,sgv  added trunc flag for open call SPR #6839
01g,21jul95,vin	 applied ntohs for the opcode field. SPR4124.
01f,11aug93,jmm  Changed ioctl.h and socket.h to sys/ioctl.h and sys/socket.h
01e,21sep92,jdi  documentation cleanup. 
01d,18jul92,smb  Changed errno.h to errnoLib.h.
01c,04jun92,ajm  shut up warnings on mips compiler
01b,26may92,rrr  the tree shuffle
		  -changed includes to have absolute path from h/
01a,29Jan92,jmm		written.
*/

/*
DESCRIPTION
This library implements the VxWorks Trivial File Transfer Protocol
(TFTP) server module.  The server can respond to both read and write
requests.  It is started by a call to tftpdInit().

The server has access to a list of directories that can either be
provided in the initial call to tftpdInit() or changed dynamically
using the tftpdDirectoryAdd() and tftpDirectoryRemove() calls.
Requests for files not in the directory trees specified in the access
list will be rejected, unless the list is empty, in which case all
requests will be allowed.  By default, the access list contains the
directory given in the global variable `tftpdDirectory'.  It is possible
to remove the default by calling tftpdDirectoryRemove().

For specific information about the TFTP protocol, see RFC 783, "TFTP
Protocol."

VXWORKS AE PROTECTION DOMAINS
Under VxWorks AE, you can run the tftp server in the kernel protection 
domain only.  This restriction does not apply under non-AE versions of 
VxWorks.  

INTERNAL

The server library uses the TFTP client routines tftpPut() and
tftpGet() to do the actual file transfer.  When the server receives a
request, it does one of three things:

    Read request (RRQ): spawns tftpFileRead task, which will call
                        tftpPut().

    Write request (WRQ): spawns tftpFileWrite task, which will call
                         tftpGet().

    All others: sends back error packet

To use this feature, include the following component:
INCLUDE_TFTP_SERVER

INCLUDE FILES: tftpdLib.h, tftpLib.h

SEE ALSO:
tftpLib, RFC 783 "TFTP Protocol"
*/

#include "vxWorks.h"
#include "tftpdLib.h"
#include "netinet/in.h"
#include "sockLib.h"
#include "sys/socket.h"
#include "tftpLib.h"
#include "errnoLib.h"
#include "fcntl.h"
#include "ioLib.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "sys/types.h"
#include "unistd.h"
#include "usrLib.h"
#include "iosLib.h"
#include "msgQLib.h"
#include "semLib.h"
#include "inetLib.h"
#include "memPartLib.h"

/* EXTERNALS */

extern int sysClkRateGet (void);

/* GLOBALS */

BOOL tftpdDebug			= FALSE;	/* TRUE: debugging messages */
int tftpdTaskPriority		= 55;
int tftpdTaskStackSize		= 12000;
int tftpdTaskId			= NONE;
int tftpdErrorSendTries		= 3;
int tftpdMaxConnections		= 10;
char *tftpdDirectoryDefault	= "/tftpboot";
int tftpdResponsePriority	= 100;

/* XXX Hack for Genus */

FUNCPTR tftpdNameMunge = NULL;

/* LOCALS */

LOCAL SEM_ID	tftpdDirectorySem;	/* protection for the semaphore list */
LOCAL LIST    	tftpdDirectoryList;	/* access list, elements  TFTPD_DIR  */
LOCAL MSG_Q_ID	tftpdDescriptorQueue;	/* msg queue of available connection */
LOCAL TFTP_DESC *tftpdDescriptorPool;
LOCAL char	tftpdErrStr []	= "TFTP server";

/* PROTOTYPES */

static STATUS tftpdRequestVerify (TFTP_DESC *pReplyDesc, int opCode,
				  char *fileName);
static STATUS tftpdRequestDecode (TFTP_MSG *pTftpMsg, int *opCode,
				  char *fileName, char *mode);
static STATUS tftpdFileRead (char *fileName, TFTP_DESC *pReplyDesc);
static STATUS tftpdFileWrite (char *fileName, TFTP_DESC *pReplyDesc);
static STATUS tftpdDescriptorQueueInit (int nEntries);
static STATUS tftpdDescriptorQueueDelete (void);
static TFTP_DESC *tftpdDescriptorCreate (char *mode, BOOL connected,
					 int sock, u_short clientPort,
					 struct sockaddr_in *pClientAddr);
static STATUS tftpdDescriptorDelete (TFTP_DESC *descriptor);
static STATUS tftpdDirectoryValidate (char *fileName);
static STATUS tftpdErrorSend (TFTP_DESC *pReplyDesc, int errorNum);

/******************************************************************************
*
* tftpdInit - initialize the TFTP server task
*
* This routine will spawn a new TFTP server task, if one does not already
* exist.  If a TFTP server task is running already, tftpdInit() will simply
* return an ERROR value without creating a new task.
*
* To change the default stack size for the TFTP server task, use the
* <stackSize> parameter.  The task stack size should be set to a large enough
* value for the needs of your application - use checkStack() to evaluate your
* stack usage.  The default size is set in the global variable
* `tftpdTaskStackSize'.  Setting <stackSize> to zero will result in the stack
* size being set to this default.
*
* To set the maximum number of simultaneous TFTP connections (each with its
* own transfer identifier or TID), set the <maxConnections> parameter.  More
* information on this is found in RFC 1350 ("The TFTP Protocol (Revision 2)").
* Setting <maxConnections> to zero will result in the maximum number of
* connections being set to the default, which is 10.
*
* If <noControl> is TRUE, the server will be set up to transfer any
* file in any location.  Otherwise, it will only transfer files in the
* directories in '/tftpboot' or the <nDirectories> directories in the
* <directoryNames> list, and will send an
* access violation error to clients that attempt to access files outside of
* these directories.
*
* By default, <noControl> is FALSE, <directoryNames> is empty, <nDirectories>
* is zero, and access is restricted to the '/tftpboot' directory.
*
* Directories can be added to the access list after initialization by using
* the tftpdDirectoryAdd() routine.
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  In addition, all arguments to this function can  
* reference only that data which is valid in the kernel protection domain. 
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS:
* OK, or ERROR if a new TFTP task cannot be created.
*/

STATUS tftpdInit
    (
    int	 stackSize,		/* stack size for the tftpdTask		*/
    int  nDirectories,		/* number of directories allowed read	*/
    char **directoryNames,	/* array of dir names			*/
    BOOL noControl,		/* TRUE if no access control required	*/
    int	 maxConnections
    )
    {

    /*
     * Make sure there isn't a TFTP server task running already
     */

    if (tftpdTaskId != NONE)
	{
	return (ERROR);
	}

    /*
     * Initialize the access list, add the default directory,
     * and give the semaphore that protects the list
     */

    lstInit (&tftpdDirectoryList);

    tftpdDirectorySem = semMCreate(SEM_Q_FIFO);

    /*
     * If access control isn't turned off, add the default directory
     * to the list
     */

    if (noControl != TRUE)
        tftpdDirectoryAdd (tftpdDirectoryDefault);

    /*
     * Add the first set of directories to the list
     */

    while (--nDirectories >= 0)
	{
	tftpdDirectoryAdd (directoryNames [nDirectories]);
	}

    /* create a TFTP server task */

    tftpdTaskId = taskSpawn ("tTftpdTask", tftpdTaskPriority, 0,
			     stackSize == 0 ? tftpdTaskStackSize : stackSize,
			     tftpdTask, nDirectories, (int) directoryNames,
			     maxConnections, 0, 0, 0, 0, 0, 0, 0);

    if (tftpdTaskId == ERROR)
	{
        printErr ("%s: tftpdTask cannot be created\n", tftpdErrStr);
	return (ERROR);
	}

    return (OK);
    }

/******************************************************************************
*
* tftpdTask - TFTP server daemon task
*
* This routine processes incoming TFTP client requests by spawning a new
* task for each connection that is set up.  This routine is called by 
* tftpdInit().
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  In addition, all arguments to this function can  
* reference only that data which is valid in the kernel protection domain. 
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS:
* OK, or ERROR if the task returns unexpectedly.
*/

STATUS tftpdTask
    (
    int		nDirectories,		/* number of dirs allowed access    */
    char	**directoryNames,	/* array of directory names         */
    int		maxConnections		/* max number of simultan. connects */
    )
    {
    int			serverSocket;	/* socket to use to communicate with
					 * the remote process */
    struct sockaddr_in	clientAddr;	/* process requesting TFTP
					 * connection */
    struct sockaddr_in	serverAddr;
    int			clientAddrLength = sizeof (struct sockaddr_in);
    TFTP_MSG 		requestBuffer;
    int			value;
    int			opCode;
    char		*fileName;
    char		mode [TFTP_SEGSIZE];
    TFTP_DESC		*pReplyDesc;
    int			replySocket;

    serverSocket = socket (AF_INET, SOCK_DGRAM, 0);

    bzero ((char *) &serverAddr, sizeof (struct sockaddr_in));
    bzero ((char *) &clientAddr, sizeof (struct sockaddr_in));

    serverAddr.sin_family	= AF_INET;
    serverAddr.sin_port		= htons((u_short) TFTP_PORT);


    serverAddr.sin_addr.s_addr	= INADDR_ANY;

    if (bind (serverSocket, (SOCKADDR *) &serverAddr,
	      sizeof (struct sockaddr_in)) == ERROR)
	{
	printErr ("%s: could not bind to TFTP port\n", tftpdErrStr);
	return (ERROR);
	}

    if (tftpdDescriptorQueueInit (maxConnections) == ERROR)
	{
	printErr ("%s: could not create descriptor queue\n", tftpdErrStr);
	return (ERROR);
	}

    /*
     * Clean out any outstanding data on the TFTP port.
     */

    FOREVER
	{
	if (ioctl (serverSocket, FIONREAD, (int) &value) == ERROR)
	    return (ERROR);

	if (value == 0)                /* done - socket cleaned out */
	    break;

	recvfrom (serverSocket, (caddr_t) &requestBuffer,
		  sizeof (TFTP_MSG), 0, (SOCKADDR *) NULL,
		  (int *) NULL);
	}

    /*
     * The main loop.  Receive requests on the TFTP port, parse the request,
     * and spawn tasks to handle it.
     */

    FOREVER
	{

	/*
	 * Read a message from the TFTP port
	 */

	value = recvfrom (serverSocket, (char *) &requestBuffer, TFTP_SEGSIZE,
			  0, (struct sockaddr *) &clientAddr,
			  &clientAddrLength);

	/*
	 * If there's an error reading on the port, abort the server.
	 */

	if (value == ERROR)
	    {
	    printErr ("%s:  could not read on TFTP port\n", tftpdErrStr);
	    close (serverSocket);
	    tftpdDescriptorQueueDelete ();
	    break;
	    }

	/*
	 * Set up a socket to use for a reply, and get a port number for it.
	 */

	replySocket = socket (AF_INET, SOCK_DGRAM, 0);
	if (replySocket == ERROR)
	    {

	    /*
	     * XXX How should we deal with an error here?
	     */

	    continue;
	    }

	serverAddr.sin_port = htons((u_short) 0);
	if (bind (replySocket, (SOCKADDR *) &serverAddr,
		  sizeof (struct sockaddr_in)) == ERROR)
	    {

	    /*
	     * XXX How should we deal with an error here?
	     */

	    continue;
	    }

	if (tftpdRequestDecode (&requestBuffer, &opCode, NULL,
				(char *) mode) == ERROR)
	    {

	    /*
	     * We received something that doesn't look like a TFTP request.
	     * Ignore it.
	     */

	    close (replySocket);
	    continue;
	    }

	/*
	 * Get a reply descriptor.  This will pend until one is available.
	 */

	pReplyDesc = tftpdDescriptorCreate (mode, TRUE, replySocket,
					    clientAddr.sin_port, &clientAddr);
	if (pReplyDesc == NULL)
	    {

	    /*
	     * Couldn't create a reply descriptor.
	     */

	    close (replySocket);
	    continue;
	    }

	/*
	 * Copy the name of the requested file into the TFTP_DESC
	 */

	fileName = pReplyDesc->fileName;
        if (tftpdRequestDecode (&requestBuffer, NULL, (char *) fileName,
                                NULL) == ERROR)
            {

            /*
             * We received something that doesn't look like a TFTP request.
             * Ignore it.
             */

            close (replySocket);
            continue;
            }

	if (tftpdRequestVerify (pReplyDesc, opCode, fileName) != OK)
	    {

	    /*
	     * Invalid request, error packet already sent by tftpdRequestVerify
	     */

	    tftpdDescriptorDelete (pReplyDesc);
	    close (replySocket);
	    continue;
	    }

	if (tftpdDebug)
	    {
	    printf ("%s: Request: Opcode = %d, file = %s, client = %s\n",
		    tftpdErrStr, opCode, fileName, pReplyDesc->serverName);
	    }

	switch (opCode)
	    {
	    case TFTP_RRQ:

		/*
		 * We've received a read request.  Spawn a tftpdFileRead
		 * task to process it.
		 */

	        taskSpawn ("tTftpRRQ", tftpdResponsePriority, 0, 10000,
			   tftpdFileRead, (int) fileName, (int) pReplyDesc,
			   0, 0, 0, 0, 0, 0, 0, 0);

		break;

	    case TFTP_WRQ:

		/*
		 * We've received a write request.  Spawn a tftpdFileWrite
		 * task to process it.
		 */

	        taskSpawn ("tTftpWRQ", tftpdResponsePriority, 0, 10000,
			   tftpdFileWrite, (int) fileName, (int) pReplyDesc,
			   0, 0, 0, 0, 0, 0, 0, 0);
		break;
	    }
	} /* end FOREVER */

    printErr ("%s:  aborting TFTP server\n", tftpdErrStr);
    tftpdDescriptorQueueDelete ();
    close (serverSocket);
    return (ERROR);
    }

/******************************************************************************
*
* tftpdRequestVerify - ensure that an incoming TFTP request is valid
*
* Checks a TFTP request to make sure that the opcode is either
* a read or write request, and then checks to see if the file requested
* is in the access list.
*
* If there is an error, it sends an error packet to the offending client.
*
* RETURNS: OK, or ERROR if any of the conditions aren't met.
*/

LOCAL STATUS tftpdRequestVerify
    (
    TFTP_DESC	*pReplyDesc,
    int		opCode,
    char	*fileName
    )
    {
    int		dirIsValid;

    /*
     * Need to check two things:
     *
     * 1.  The request itself needs to be valid, either a write request (WRQ)
     *     or a read request (RRQ).
     *
     * 2.  It needs to be to a valid directory.
     */

    if ((opCode != TFTP_RRQ) && (opCode != TFTP_WRQ))
	{

	/*
	 * Bad opCode sent to the server.
	 */

	tftpdErrorSend (pReplyDesc, EBADOP);
	return (ERROR);
	}

    dirIsValid = tftpdDirectoryValidate (fileName);
    if (dirIsValid != OK)
	{

	/*
	 * Access was denied to the file that the client
	 * requested.
	 */

	tftpdErrorSend (pReplyDesc, errnoGet());
	return (ERROR);
	}

    return (OK);
    }

/******************************************************************************
*
* tftpdRequestDecode - break down a TFTP request
*
* Given a pointer to a TFTP message, this routine decodes the message
* and returns the message's opcode, file name, and mode.
*
* RETURNS:
* OK or ERROR.
*/


LOCAL STATUS tftpdRequestDecode
    (
    TFTP_MSG	*pTftpMsg,
    int	*	opCode,		/* pointer to the opCode to return */
    char	*fileName,	/* where to return filename */
    char	*mode		/* where to return mode */
    )
    {
    char	*strIndex;	/* index into pTftpMsg to get mode string */

    if (pTftpMsg == NULL)
	return (ERROR);

    if (opCode != NULL)
	*opCode = ntohs(pTftpMsg->th_opcode);

    if (fileName != NULL)
	{
	strncpy (fileName, pTftpMsg->th.request, 128);
	fileName [127] = EOS;
	}

    if (mode != NULL)
	{

	/*
	 * Need to get the next string in the struct. Use the for loop to
	 * find the end of the first string.
	 */

	for (strIndex = pTftpMsg->th.request;
	     *strIndex != EOS;
	     strIndex++)
	    ;

	strncpy(mode, ++strIndex, 32);
	mode [31] = EOS;
	}

    return (OK);
    }

/******************************************************************************
*
* tftpdFileRead - handle a read request
*
* This routine constructs and executes the tftpPut() command to put the file
* to the remote system.  Normally this routine is the entry point for a task
* created by tftpdTask() in response to a read request.
*
* RETURNS: OK, or ERROR if the file requested could not be opened.
*/

LOCAL STATUS tftpdFileRead
    (
    char	*fileName,		/* file to be sent */
    TFTP_DESC	*pReplyDesc 	/* where to send the file */
    )
    {
    int		requestFd;
    int		returnValue = OK;

    /*
     * XXX - X windows needs files that don't work with DOS - they have
     * more than three chars after a dot (as in fonts.alias).  If the
     * funcpointer is non-null, call the routine with a pointer to
     * filename, and the called routine should change the name to something
     * DOS understands.
     */

    if (tftpdNameMunge != NULL)
        (*tftpdNameMunge) (fileName);

    requestFd = open (fileName, O_RDONLY, 0);

    if (requestFd == ERROR)
	{
	if (tftpdDebug)
	    {
	    printErr ("%s: Could not open file %s\n", tftpdErrStr, fileName);
	    }
	tftpdErrorSend (pReplyDesc, errnoGet());
	}
    else
	{

	/*
	 * We call tftpPut from the server on a read request because the
	 * server is putting the file to the client
	 */

	returnValue = tftpPut (pReplyDesc, fileName, requestFd,
			       TFTP_SERVER);
	close (requestFd);
	}

    /*
     * Close the socket, and delete the
     * tftpd descriptor.
     */

    if (returnValue == ERROR)
	{
	printErr ("%s:  could not send client file \"%s\"\n", tftpdErrStr,
		  fileName);
	}


    tftpdDescriptorDelete (pReplyDesc);

    return (returnValue);
    }

/******************************************************************************
*
* tftpdFileWrite - handle a write request
*
* This routine constructs and executes the tftpGet() command to get the file
* from the remote system.  Normally this routine is the entry point for a
* task created by tftpdTask() in response to a write request.
*
* RETURNS: OK, or ERROR if the file requested could not be opened.
*/

LOCAL STATUS tftpdFileWrite
    (
    char	*fileName,		/* file to be sent */
    TFTP_DESC	*pReplyDesc 	/* where to send the file */
    )
    {
    int		requestFd;
    int		returnValue = OK;

    requestFd = open (fileName, O_WRONLY | O_CREAT | O_TRUNC, 0);

    if (requestFd == ERROR)
	{
	if (tftpdDebug)
	    {
	    printErr ("%s: Could not open file %s\n", tftpdErrStr, fileName);
	    }
	tftpdErrorSend (pReplyDesc, errnoGet());
	}
    else
	{

	/*
	 * We call tftpGet from the server on a read request because the
	 * server is putting the file to the client
	 */

	returnValue = tftpGet (pReplyDesc, fileName, requestFd,
			       TFTP_SERVER);
	close (requestFd);
	}

    /*
     * Close the socket, and delete the
     * tftpd descriptor.
     */

    if (returnValue == ERROR)
	{
	printErr ("%s:  could not send \"%s\" to client\n", tftpdErrStr);
	}

    tftpdDescriptorDelete (pReplyDesc);

    return (returnValue);
    }

/******************************************************************************
*
* tftpdDescriptorQueueInit - set up pool of available descriptors
*
* This routine sets up the system for managing a pool of available reply
* descriptors.  A piece of contiguous memory, large enough to hold an
* array of <nEntries> descriptors, is allocated (with malloc()).
* Pointers to each element in the array are written into a message
* queue.  The routine tftpdDescriptorCreate() reads pointers out of this
* queue, and tftpdDescriptorDelete() writes them back in when the
* descriptor space is no longer needed.
*
* RETURNS: OK, or ERROR if either out of memory or the queue could not
* be allocated.
*/

LOCAL STATUS tftpdDescriptorQueueInit
    (
    int	nEntries	/* maximum number of descriptors in
			 * use at any time */
    )
    {
    TFTP_DESC *theAddress;

    /*
     * If nEntries is 0, set it to the default number of connections
     */

    if (nEntries == 0)
        nEntries = tftpdMaxConnections;

    /*
     * Create the message queue
     */

    if ((tftpdDescriptorQueue = (MSG_Q_ID) msgQCreate (nEntries,
						       sizeof (TFTP_DESC *),
						       MSG_Q_FIFO)) == NULL)
        {
	return (ERROR);
	}

    /*
     * Allocate space for the descriptors.
     */

    tftpdDescriptorPool = (TFTP_DESC *) KHEAP_ALLOC(sizeof (TFTP_DESC) * nEntries);
    if (tftpdDescriptorPool == NULL)
	{
	msgQDelete (tftpdDescriptorQueue);
        return (ERROR);
	}

    while (nEntries--)
	{
	theAddress = &(tftpdDescriptorPool [nEntries]);
	msgQSend (tftpdDescriptorQueue, (char *) &theAddress,
		  sizeof (TFTP_DESC *), WAIT_FOREVER, MSG_PRI_NORMAL);
	}

    return (OK);
    }

/******************************************************************************
*
* tftpdDescriptorQueueDelete - delete pool of available descriptors
*
* This routine deletes the memory and message queues allocated by
* tftpdDescriptorQueueInit.
*
* RETURNS: OK, or ERROR if either memory could not be freed or if the
* message queue could not be deleted.
*/

LOCAL STATUS tftpdDescriptorQueueDelete
    (
    void
    )
    {
    STATUS returnValue = OK;

    if (msgQDelete (tftpdDescriptorQueue) == ERROR)
	{
	printErr ("%s:  could not delete message queue\n", tftpdErrStr);
	returnValue = ERROR;
	}

    /*
     * free() is now void according to ANSI, so can't check the return value
     */

    KHEAP_FREE((char *)tftpdDescriptorPool);

    return (returnValue);
    }

/******************************************************************************
*
* tftpdDescriptorCreate - build a tftp descriptor to use with tftpLib
*
* The routines in tftpLib, tftpPut() and tftpGet() in particular, expect to
* be passed a pointer to a struct of type TFTP_DESC that contains the
* information about the connection to the host.  This is a convenience
* routine to allocate space for a TFTP_DESC struct, fill in the elements,
* and return a pointer to it.
*
* This routine pends until a descriptor is available.
*
* RETURNS:
* A pointer to a newly allocated TFTP_DESC struct, or NULL on failure.
*/

LOCAL TFTP_DESC *tftpdDescriptorCreate
    (
    char	*mode,			/* mode 		*/
    BOOL	connected,		/* state		*/
    int		sock,			/* socket number	*/
    u_short	clientPort,		/* client port number	*/
    struct sockaddr_in *pClientAddr 	/* client address	*/
    )
    {
    TFTP_DESC	*pTftpDesc;		/* pointer to the struct to return */
    char	clientName [INET_ADDR_LEN];

    if (msgQReceive (tftpdDescriptorQueue, (char *) &pTftpDesc,
		     sizeof (pTftpDesc), WAIT_FOREVER) == ERROR)
	{

	/*
	 * Couldn't get a descriptor from the queue, return an error
	 */

	return (NULL);
	}

    /*
     * Copy the arguments into the appropriate elements of the struct
     */

    strncpy (pTftpDesc->mode, mode, 31);
    pTftpDesc->mode [31] = EOS;
    pTftpDesc->connected = connected;

    /*
     * clientName and serverName are reversed, because the routines
     * that use this descriptor are expecting to look at the universe
     * from the client side.
     */


    inet_ntoa_b (pClientAddr->sin_addr, clientName);

    strncpy (pTftpDesc->serverName, clientName, 128);
    pTftpDesc->serverName [127] = EOS;

    bcopy ((char *) pClientAddr, (char *) &pTftpDesc->serverAddr,
	   sizeof (struct sockaddr_in));
    pTftpDesc->sock = sock;
    pTftpDesc->serverPort = clientPort;

    /*
     * We've filled the struct, now return a pointer to it
     */

    return (pTftpDesc);
    }

/******************************************************************************
*
* tftpdDescriptorDelete - delete a reply descriptor
*
* This routine returns the space for a reply descriptor back to the
* descriptor pool.
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS tftpdDescriptorDelete
    (
    TFTP_DESC *descriptor
    )
    {

    close (descriptor->sock);

    return (msgQSend (tftpdDescriptorQueue, (char *) &descriptor,
		      sizeof (TFTP_DESC *), WAIT_FOREVER, MSG_PRI_NORMAL));
    }

/******************************************************************************
*
* tftpdDirectoryAdd - add a directory to the access list
*
* This routine adds the specified directory name to the access list 
* for the TFTP server.
*
* RETURNS: N/A
*/

STATUS tftpdDirectoryAdd
    (
    char	*fileName	/* name of directory to add to access list */
    )
    {
    TFTPD_DIR	*newNode;

    /*
     * Allocate memory for the node
     */

    newNode = (TFTPD_DIR *) KHEAP_ALLOC(sizeof (TFTPD_DIR));
    if (newNode == NULL)
	{
	return (ERROR);
	}

    /*
     * Copy the filename into the node
     */

    strncpy (newNode->dirName, fileName, MAX_FILENAME_LENGTH);
    newNode->dirName [MAX_FILENAME_LENGTH - 1] = EOS;

    /*
     * Add the node to the list
     */

    semTake (tftpdDirectorySem, WAIT_FOREVER);
    lstAdd (&tftpdDirectoryList, (NODE *) newNode);
    semGive (tftpdDirectorySem);

    return (OK);
    }

/******************************************************************************
*
* tftpdDirectoryRemove - delete a directory from the access list
*
* This routine deletes the specified directory name from the access list 
* for the TFTP server.
*
* RETURNS: N/A
*/

STATUS tftpdDirectoryRemove
    (
    char	*fileName	/* name of directory to add to access list */
    )
    {
    TFTPD_DIR	*dirNode;

    for (dirNode = (TFTPD_DIR *) lstFirst (&tftpdDirectoryList);
	 dirNode != NULL;
	 dirNode = (TFTPD_DIR *) lstNext ((NODE *) dirNode))
	{

	/*
	 * if the name of the file matches the name in the current
	 * element, delete the element
	 */

	if (strcmp (dirNode->dirName, fileName) == 0)
	    {
	    semTake (tftpdDirectorySem, WAIT_FOREVER);
	    lstDelete (&tftpdDirectoryList, (NODE *) dirNode);
	    semGive (tftpdDirectorySem);

	    /*
	     * Also need to free the memory
	     */

	    KHEAP_FREE((char *)dirNode);

	    return (OK);
	    }
	}

    /*
     * If we got to this point, then there was no match.  Return error.
     */

    return (ERROR);
    }

/******************************************************************************
*
* tftpdDirectoryValidate - confirm that file requested is in valid directory
*
* This routine checks that the file requested is in a directory that
* matches an entry in the directory access list.  The validation
* procedure is:
*
*     1.  If the requested file matches the directory exactly,
*         deny access.  This prevents opening devices rather
* 	  than files.
*
*     2.  If the directory and the first part of the file
*         are equal, permission is granted.
*
*     Examples:
*
* 		File		Directory	Result
* 		/mem		/mem		rejected by #1
* 		/mem/foo	/mem		granted
* 		/stuff/foo	/mem		rejected by #2
* 						(first four chars of file
* 						don't match "/mem")
*
* RETURNS: OK, or ERROR if access is not allowed.
*/

LOCAL STATUS tftpdDirectoryValidate
    (
    char *fileName
    )
    {
    DEV_HDR	*deviceHdr;
    char 	fullPathName [MAX_FILENAME_LENGTH];
    char 	tmpString [MAX_FILENAME_LENGTH];
    TFTPD_DIR	*dirNode;
    STATUS 	returnValue = ERROR;

    /*
     * Use ioFullFileNameGet to get the complete name, including
     * the device, of the requested file.
     */

    ioFullFileNameGet (fileName, &deviceHdr, fullPathName);
    strcpy (tmpString, deviceHdr->name);
    strcat (tmpString, fullPathName);
    strcpy (fullPathName, tmpString);

    /*
     * If the access list is empty, there are no restrictions.  Return OK.
     */

    if (lstCount (&tftpdDirectoryList) == 0)
	return (OK);

    semTake (tftpdDirectorySem, WAIT_FOREVER);

    for (dirNode = (TFTPD_DIR *) lstFirst (&tftpdDirectoryList);
	 dirNode != NULL;
	 dirNode = (TFTPD_DIR *) lstNext ((NODE *) dirNode))
	{

	/*
	 * If the filename is exactly the same as the directory, break the loop
	 * and return an error (rejected by #1)
	 */

	if (strcmp (fullPathName, dirNode->dirName) == 0)
	    {
	    returnValue = ERROR;
	    break;
	    }

	/*
	 * If the first part of the filename is exactly equal
	 * to the directory name, return OK (#2).
	 */

	if (strncmp (dirNode->dirName, fullPathName,
		     strlen (dirNode->dirName)) == 0)
	    {
	    returnValue = OK;
	    break;
	    }
	}
    semGive (tftpdDirectorySem);

    if (returnValue == ERROR)
	errnoSet (EACCESS);
    return (returnValue);
    }

/******************************************************************************
*
* tftpdErrorSend - send an error to a client
*
* Given a client connection and an error number, this routine builds an error
* packet and sends it to the client.
*
* RETURNS:
* OK, or ERROR if tftpSend() fails.  Note that no further action is required if
* tftpSend() fails.
*/

LOCAL STATUS tftpdErrorSend
    (
    TFTP_DESC	*pReplyDesc,	/* client to send to */
    int		errorNum		/* error to send */
    )
    {
    TFTP_MSG	errMsg;
    int		errSize;
    int		repeatCount = tftpdErrorSendTries;
    STATUS	returnValue = OK;

    errSize = tftpErrorCreate (&errMsg, errorNum);

    /*
     * Try to send the error message a few times.
     */

    while (repeatCount--)
	{
	returnValue = tftpSend (pReplyDesc, &errMsg, errSize,
				(TFTP_MSG *) 0, 0, 0, (int *) 0);
	/*
	 * If we didn't get an error, break out of the loop.  Otherwise,
	 * wait one second and retry
	 */

	if (returnValue != ERROR)
	    {
	    break;
	    }
	else
	    {
	    taskDelay (sysClkRateGet ());
	    }
	}
    return (returnValue);
    }

    /* XXX
     * everything below here doesn't go into final product
     */

char *dirs[] = { "/mem", "/tftpboot", "/folk/james/zap", "/xt" };

void tftpTst (void)
    {
    tftpdTask (sizeof (dirs) / sizeof (char *), dirs, 3);
    }



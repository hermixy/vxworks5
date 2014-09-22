/* ftpdLib.c - File Transfer Protocol (FTP) server */

/* Copyright 1990 - 2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02u,22may02,elr  Corrected closing of sockets so they do not linger (SPR #77377)
02t,05nov01,vvv  fixed compilation warnings
02s,15oct01,rae  merge from truestack ver 02w, base 02q
02r,13oct00,cn   fixed memory leak (SPR# 25954).
02q,16mar99,spm  recovered orphaned code from tor2_0_x branch (SPR #25770)
02p,01dec98,spm  changed reply code for successful DELE command (SPR #20554)
02o,27mar98,spm  corrected byte-ordering problem in PASV command (SPR #20828)
02n,27mar98,spm  merged from recovered version 02m of tor1_0_x branch
02m,10dec97,spm  upgraded server shutdown routine to terminate active 
                 sessions (SPR #9906); corrected response for PASV command
                 to include valid IP address (SPR #1318); modified syntax
                 of PASV command (SPR #5627); corrected handling of PORT 
                 command to support multiple interfaces (SPR #3500); added 
                 support for maximum number of connections (SPR #2032);
                 applied changes for configurable password authentication 
                 from SENS branch (SPR #8602); removed incorrect note from
                 man page concerning user/password verification, which was 
                 actually performed (SPR #7672); general cleanup (reorganized
                 code, added FTP responses for error conditions, replaced 
                 "static" with LOCAL keyword in function declarations)
02l,09jul97,dgp  doc: add note on UID and password per SPR 7672
02k,06feb97,jdi  made drawing internal.
02j,30sep96,spm  partial fix for spr #7227. Added support for deleting files
                 and using relative pathnames when listing directories.
02i,05aug96,sgv  fix for spr #3583 and spr #5920. Provide login security
		 for VxWorks login
02h,21may96,sgv  Added global variable ftpdWindowSize which can be set by
		 the user. the server would set the window size after the
		 connection is established.
02g,29mar95,kdl  changed ftpdDirListGet() to use ANSI time format in stat.
02f,11feb95,jdi  doc format tweak.
02e,20aug93,jag  Fixed memory leak by calling fclose (SPR #2194)
                 Changed ftpdWorkTask Command Read Logic, Added error checking
		 on write calls to the network and file operations.
                 Added case-conversion changes (SPR #2035)
02d,20aug93,jmm  Changed ioctl.h and socket.h to sys/ioctl.h and sys/socket.h
02c,27feb93,kdl  Removed 01z case-conversion changes (SPR #2035).
02b,05feb93,jag  Changed call to inet_ntoa to inet_ntoa_b. SPR# 1814
02a,20jan93,jdi  documentation cleanup for 5.1.
01z,09sep92,jmm  fixed spr 1568, ftpd now recognizes lower case commands
                 changed errnoGet() to errno to get rid of warning message
01y,19aug92,smb  Changed systime.h to sys/times.h.
01x,16jun92,kdl	 increased slot buffer to hold null terminator; use calloc()
		 to allocate slot struct (SPR #1509).
01w,26may92,rrr  the tree shuffle
		  -changed includes to have absolute path from h/
01v,08apr92,jmm  cleaned up some ansi warnings
01u,18dec91,rrr  removed a recursive macro (killed the mips compiler)
01t,10dec91,gae  ANSI cleanup. Changed ftpdSlotSem to an Id so that internal
		    routine semTerminate() not used.
01s,19nov91,rrr  shut up some ansi warnings.
01r,14nov91,rrr  shut up some warnings
01q,12nov91,wmd  fixed bug in ftpdDataStreamSend() and ftpdDataStreamReceive(),
                 EOF is cast to type char to prevent endless looping.
01p,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -changed VOID to void
		  -changed copyright notice
01o,10jul91,gae  i960 fixes: non-varargs usage, added ntohs().  added HELP
		 command, changed listing to support NFS/DOS not old style.
01n,30apr91,jdi	 documentation tweaks.
01m,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by dnw.
01l,12feb91,jaa	 documentation.
01k,08oct90,hjb  included "inetLib.h".
01j,05oct90,dnw  made ftpdWorkTask() be LOCAL.
		 documentation tweaks.
01i,02oct90,hjb  deleted "inet.h".  added more doc to ftpdInit().  added a call
		   to htons() where needed.
01h,18sep90,kdl  removed erroneous forward declaration of "ftpDataStreamRecv()".
01g,10aug90,dnw  added forward declaration of ftpdDataStreamReceive().
01f,10aug90,kdl  added forward declarations for functions returning void.
01e,26jun90,jcf  changed ftpd semaphore to static mutex.
01d,07may90,hjb  various bug fixes -- too numerous to mention.
01c,17apr90,jcf  changed ftpd work task name to tFtpd...
01b,11apr90,hjb  de-linted
01a,01mar90,hjb  written
*/

/*
DESCRIPTION
This library implements the server side of the File Transfer Protocol (FTP),
which provides remote access to the file systems available on a target.
The protocol is defined in RFC 959. This implementation supports all commands
required by that specification, as well as several additional commands.

USER INTERFACE
During system startup, the ftpdInit() routine creates a control connection
at the predefined FTP server port which is monitored by the primary FTP
task. Each FTP session established is handled by a secondary server task
created as necessary. The server accepts the following commands:

.TS
tab(|);
l1 l.
    HELP | - List supported commands.
    USER | - Verify user name.
    PASS | - Verify password for the user.
    QUIT | - Quit the session.
    LIST | - List out contents of a directory.
    NLST | - List directory contents using a concise format.
    RETR | - Retrieve a file.
    STOR | - Store a file.
    CWD | - Change working directory.
    TYPE | - Change the data representation type.
    PORT | - Change the port number.
    PWD | - Get the name of current working directory.
    STRU | - Change file structure settings.
    MODE | - Change file transfer mode.
    ALLO | - Reserver sufficient storage.
    ACCT | - Identify the user's account.
    PASV | - Make the server listen on a port for data connection.
    NOOP | - Do nothing.
    DELE | - Delete a file

.TE

The ftpdDelete() routine will disable the FTP server until restarted. 
It reclaims all system resources used by the server tasks and cleanly 
terminates all active sessions.

To use this feature, include the following component:
INCLUDE_FTP_SERVER

INTERNAL
The ftpdInit() routine spawns the primary server task ('ftpdTask') to handle
multiple FTP sessions. That task creates a separate task ('ftpdWorkTask') for
each active control connection.

The diagram below defines the structure chart of ftpdLib.
.CS

  ftpdDelete		   			ftpdInit
	|  \					  |
	|   \					  |
	|    \					ftpdTask
	|     \					/    |  \____________
	|      \			       /     |		     \
	|	|   	          ftpdSessionAdd ftpdWorkTask ftpdSessionDelete
	|	|	      ______________________/     |  \
	|	|	     /    /	|	          |   \
 ftpdSlotDelete | ftpdDirListGet /  ftpdDataStreamReceive |   ftpdDataStreamSend
	|	|	        /	|	\	  |   /	     /
	 \	|    __________/	|	 \	  |  /	    /
	  \	|   /			|         ftpdDataConnGet  /
	   \	|   |			|	   |   ___________/
	    \	|   |			|	   |  /
         ftpdSockFree			ftpdDataCmdSend
.CE

INCLUDE FILES: ftpdLib.h

SEE ALSO:
ftpLib, netDrv, 
.I "RFC-959 File Transfer Protocol"
*/


#include "vxWorks.h"
#include "ioLib.h"
#include "taskLib.h"
#include "lstLib.h"
#include "stdio.h"
#include "sys/socket.h"
#include "netinet/in.h"
#include "errno.h"
#include "version.h"
#include "string.h"
#include "stdlib.h"
#include "iosLib.h"
#include "inetLib.h"
#include "dirent.h"
#include "sys/stat.h"
#include "sys/times.h"
#include "sockLib.h"
#include "logLib.h"
#include "unistd.h"
#include "pathLib.h"
#include "sysLib.h"
#include "ftpdLib.h"
#include "ctype.h"
#include "time.h"
#include "loginLib.h"
#include "memPartLib.h"
#ifndef _WRS_VXWORKS_5_X
#include "private/pdLibP.h"
#endif /* _WRS_VXWORKS_5_X */

/* Representation Type */

#define FTPD_BINARY_TYPE	0x1
#define FTPD_ASCII_TYPE		0x2
#define FTPD_EBCDIC_TYPE	0x4
#define FTPD_LOCAL_BYTE_TYPE	0x8

/* Transfer mode */

#define FTPD_STREAM_MODE	0x10
#define FTPD_BLOCK_MODE		0x20
#define FTPD_COMPRESSED_MODE	0x40

/* File structure */

#define FTPD_NO_RECORD_STRU	0x100
#define FTPD_RECORD_STRU	0x200
#define FTPD_PAGE_STRU		0x400

/* Session Status */

#define FTPD_USER_OK		0x1000
#define FTPD_PASSIVE		0x2000

/* Macros to obtain correct parts of the status code */

#define FTPD_REPRESENTATION(slot)	( (slot)->status	& 0xff)
#define FTPD_TRANS_MODE(slot)		(((slot)->status >> 8)	& 0xff)
#define FTPD_FILE_STRUCTURE(slot)	(((slot)->status >> 16)	& 0xff)
#define FTPD_STATUS(slot)		(((slot)->status >> 24) & 0xff)

/* Well known port definitions -- someday we'll have getservbyname */

#define FTP_DATA_PORT		20
#define FTP_DAEMON_PORT		21

/* Free socket indicative */

#define FTPD_SOCK_FREE		-1

/* Arbitrary limits for the size of the FTPD work task name */

#define FTPD_WORK_TASK_NAME_LEN	40

/* Arbitrary limits hinted by Unix FTPD in waing for a new data connection */

#define FTPD_WAIT_MAX		90
#define FTPD_WAIT_INTERVAL	5

/* Macro to get the byte out of an int */

#define FTPD_UC(ch)		(((int) (ch)) & 0xff)

/* Bit set in FTP reply code to indicate multi-line reply.
 * Used internally by ftpdCmdSend() where codes are less than
 * 1024 but are 32-bit integers.  [Admittedly a hack, see
 * ftpdCmdSend().]
 */

#define	FTPD_MULTI_LINE		0x10000000

#define FTPD_WINDOW_SIZE	10240

/* globals */

int ftpdDebug			= FALSE;	/* TRUE: debugging messages */

int ftpdTaskPriority		= 56;
int ftpdTaskOptions 		= VX_SUPERVISOR_MODE | VX_UNBREAKABLE;
int ftpdWorkTaskPriority	= 252;
int ftpdWorkTaskOptions		= VX_SUPERVISOR_MODE | VX_UNBREAKABLE; 
int ftpdWorkTaskStackSize	= 12000;
int ftpdWindowSize              = FTPD_WINDOW_SIZE;
int ftpsMaxClients = 4; 	/* Default max. for simultaneous connections */
int ftpsCurrentClients;
FUNCPTR loginVerifyRtn;

/* locals */

LOCAL BOOL ftpsActive = FALSE; 	/* Server started? */
LOCAL BOOL ftpsShutdownFlag; 	/* Server halt requested? */

/*
 * The FTP server keeps track of active client sessions in a linked list
 * of the following FTPD_SESSION_DATA data structures. That structure
 * contains all the variables which must be maintained separately
 * for each client so that the code shared by every secondary
 * task will function correctly.
 */

typedef struct
    {
    NODE		node;		/* for link-listing */
    int			status;		/* see various status bits above */
    int			byteCount;	/* bytes transferred */
    int			cmdSock;	/* command socket */
    STATUS		cmdSockError;   /* Set to ERROR on write error */
    int			dataSock;	/* data socket */
    struct sockaddr_in	peerAddr;	/* address of control connection */
    struct sockaddr_in 	dataAddr; 	/* address of data connection */
    char		buf [BUFSIZE]; /* multi-purpose buffer per session */
    char 		curDirName [MAX_FILENAME_LENGTH]; /* active directory */
    char               user [MAX_LOGIN_NAME_LEN+1]; /* current user */
    } FTPD_SESSION_DATA;

LOCAL int ftpdTaskId 		= -1;
LOCAL int ftpdServerSock 	= FTPD_SOCK_FREE;

LOCAL LIST		ftpsSessionList;
LOCAL SEM_ID		ftpsMutexSem;
LOCAL SEM_ID 		ftpsSignalSem;

LOCAL char		ftpdWorkTaskName [FTPD_WORK_TASK_NAME_LEN];
LOCAL int		ftpdNumTasks;

/* Various messages to be told to the clients */

LOCAL char *messages [] =
    {
    "Can't open passive connection",
    "Parameter not accepted",
    "Data connection error",
    "Directory non existent or syntax error",
    "Local resource failure: %s",
    "VxWorks (%s) FTP server ready",
    "Password required",
    "User logged in",
    "Bye...see you later",
    "USER and PASS required",
    "No files found or invalid directory or permission problem",
    "Transfer complete",
    "File \"%s\" not found or permission problem",
    "Cannot create file \"%s\" or permission problem",
    "Changed directory to \"%s\"",
    "Type set to I, binary mode",
    "Type set to A, ASCII mode",
    "Port set okay",
    "Current directory is \"%s\"",
    "File structure set to NO RECORD",
    "Stream mode okay",
    "Allocate and Account not required",
    "Entering Passive Mode (%d,%d,%d,%d,%d,%d)",
    "NOOP -- did nothing as requested...hah!",
    "Command not recognized",
    "Error in input file",
    "Unimplemented TYPE %d",
    "You could at least say goodbye.",
    "The following commands are recognized:",
    "End of command list.",
    "File deleted successfully.",
    "Login failed.",
    };

/* Indexes to the messages [] array */

#define MSG_PASSIVE_ERROR	0
#define MSG_PARAM_BAD		1
#define MSG_DATA_CONN_ERROR	2
#define MSG_DIR_NOT_PRESENT	3
#define MSG_LOCAL_RESOURCE_FAIL	4
#define MSG_SERVER_READY	5
#define MSG_PASSWORD_REQUIRED	6
#define MSG_USER_LOGGED_IN	7
#define MSG_SEE_YOU_LATER	8
#define MSG_USER_PASS_REQ	9
#define MSG_DIR_ERROR		10
#define MSG_TRANS_COMPLETE	11
#define MSG_FILE_ERROR		12
#define MSG_CREATE_ERROR	13
#define MSG_CHANGED_DIR		14
#define MSG_TYPE_BINARY		15
#define MSG_TYPE_ASCII		16
#define MSG_PORT_SET		17
#define MSG_CUR_DIR		18
#define MSG_FILE_STRU		19
#define MSG_STREAM_MODE		20
#define MSG_ALLOC_ACCOUNT	21
#define MSG_PASSIVE_MODE	22
#define MSG_NOOP_OKAY		23
#define MSG_BAD_COMMAND		24
#define MSG_INPUT_FILE_ERROR	25
#define MSG_TYPE_ERROR		26
#define MSG_NO_GOOD_BYE		27
#define	MSG_COMMAND_LIST_BEGIN	28
#define	MSG_COMMAND_LIST_END	29
#define MSG_DELE_OKAY           30
#define        MSG_USER_LOGIN_FAILED   31

LOCAL char *ftpdCommandList =
"HELP	USER	PASS	QUIT	LIST	NLST\n\
RETR	STOR	CWD	TYPE	PORT	PWD\n\
STRU	MODE	ALLO	ACCT	PASV	NOOP\n\
DELE\n";


/* forward declarations */

LOCAL FTPD_SESSION_DATA *ftpdSessionAdd (void);
LOCAL void ftpdSessionDelete (FTPD_SESSION_DATA *);
LOCAL STATUS ftpdWorkTask (FTPD_SESSION_DATA *);
LOCAL STATUS ftpdCmdSend (FTPD_SESSION_DATA *, int, int, char *,
                          int, int, int, int, int, int);
LOCAL STATUS ftpdDataConnGet (FTPD_SESSION_DATA *);
LOCAL void ftpdDataStreamSend (FTPD_SESSION_DATA *, FILE *);
LOCAL void ftpdDataStreamReceive (FTPD_SESSION_DATA *, FILE *outStream);
LOCAL void ftpdSockFree (int *);
LOCAL STATUS ftpdDirListGet (int, char *, BOOL);
LOCAL void ftpdDebugMsg (char *, int, int, int, int);
LOCAL void unImplementedType (FTPD_SESSION_DATA *pSlot);
LOCAL void dataError (FTPD_SESSION_DATA *pSlot);
LOCAL void fileError (FTPD_SESSION_DATA *pSlot);
LOCAL void transferOkay (FTPD_SESSION_DATA *pSlot);

/*******************************************************************************
*
* ftpdTask - FTP server daemon task
*
* This routine monitors the FTP control port for incoming requests from clients
* and processes each request by spawning a secondary server task after 
* establishing the control connection. If the maximum number of connections is
* reached, it returns the appropriate error to the requesting client. The 
* routine is the entry point for the primary FTP server task and should only
* be called internally.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* INTERNAL:
* The server task is deleted by the server shutdown routine. Adding a newly
* created client session to the list of active clients is performed atomically
* with respect to the shutdown routine. However, accepting control connections
* is not a critical section, since closing the initial socket used in the
* listen() call also closes any later connections which are still open.
*
* NOMANUAL
*/

LOCAL void ftpdTask (void)
    {
    int		newSock;
    FAST FTPD_SESSION_DATA *pSlot;
    int		on = 1;
    int		addrLen;
    struct sockaddr_in	addr;
    char	a_ip_addr [INET_ADDR_LEN];  /* ascii ip address of client */

    ftpdNumTasks = 0;

    /* The following loop halts if this task is deleted. */

    FOREVER
        {
        /* Wait for a new incoming connection. */

        addrLen = sizeof (struct sockaddr);

        ftpdDebugMsg ("waiting for a new client connection...\n",0,0,0,0);

        newSock = accept (ftpdServerSock, (struct sockaddr *) &addr, &addrLen);
	if (newSock < 0)
            {
            ftpdDebugMsg ("cannot accept a new connection\n",0,0,0,0);
            break;
            }

        /*
         * Register a new FTP client session. This process is a critical
         * section with the server shutdown routine. If an error occurs,
         * the reply must be sent over the control connection to the peer
         * before the semaphore is released. Otherwise, this task could
         * be deleted and no response would be sent, possibly causing
         * the new client to hang indefinitely.
         */

        semTake (ftpsMutexSem, WAIT_FOREVER);

        setsockopt (newSock, SOL_SOCKET, SO_KEEPALIVE, (char *) &on,
                    sizeof (on));

        inet_ntoa_b (addr.sin_addr, a_ip_addr);
        ftpdDebugMsg ("accepted a new client connection from %s\n",
                      (int) a_ip_addr, 0, 0, 0);

        /* Create a new session entry for this connection, if possible. */

        pSlot = ftpdSessionAdd ();
        if (pSlot == NULL) 	/* Maximum number of connections reached. */
            {
            /* Send transient failure report to client. */

            ftpdCmdSend (pSlot, newSock, 421, 
                         "Session limit reached, closing control connection", 
                         0, 0, 0, 0, 0, 0);
            ftpdDebugMsg ("cannot get a new connection slot\n",0,0,0,0);
	    close (newSock);
            semGive (ftpsMutexSem);
            continue;
            }

	pSlot->cmdSock	= newSock;

        /* Save the control address and assign the default data address. */

        bcopy ( (char *)&addr, (char *)&pSlot->peerAddr, 
               sizeof (struct sockaddr_in));
        bcopy ( (char *)&addr, (char *)&pSlot->dataAddr, 
               sizeof (struct sockaddr_in));
        pSlot->dataAddr.sin_port = htons (FTP_DATA_PORT);

	/* Create a task name. */

        sprintf (ftpdWorkTaskName, "tFtpdServ%d", ftpdNumTasks);

        /* Spawn a secondary task to process FTP requests for this session. */

	ftpdDebugMsg ("creating a new server task %s...\n", 
		      (int) ftpdWorkTaskName, 0, 0, 0);

	if (taskSpawn (ftpdWorkTaskName, ftpdWorkTaskPriority,
		       ftpdWorkTaskOptions, ftpdWorkTaskStackSize,
		       ftpdWorkTask, (int) pSlot,
		       0, 0, 0, 0, 0, 0, 0, 0, 0) == ERROR)
	    {
            /* Send transient failure report to client. */

            ftpdCmdSend (pSlot, newSock, 421, 
                         "Service not available, closing control connection",
                         0, 0, 0, 0, 0, 0);
	    ftpdSessionDelete (pSlot);
	    ftpdDebugMsg ("cannot create a new work task\n",0,0,0,0);
            semGive (ftpsMutexSem);
            continue;
	    }

	ftpdDebugMsg ("done.\n",0,0,0,0);

        /* Session added - end of critical section with shutdown routine. */

        semGive (ftpsMutexSem);
	}

    /* Fatal error - update state of server. */

    ftpsActive = FALSE;

    return;
    }

/*******************************************************************************
*
* ftpdInit - initialize the FTP server task
*
* This routine installs the password verification routine indicated by
* <pLoginRtn> and establishes a control connection for the primary FTP
* server task, which it then creates. It is called automatically during
* system startup if INCLUDE_FTP_SERVER is defined. The primary server task 
* supports simultaneous client sessions, up to the limit specified by the 
* global variable 'ftpsMaxClients'. The default value allows a maximum of 
* four simultaneous connections. The <stackSize> argument specifies the stack 
* size for the primary server task. It is set to the value specified in the 
* 'ftpdWorkTaskStackSize' global variable by default.
*
* RETURNS:
* OK if server started, or ERROR otherwise.
*
* ERRNO: N/A
*/

STATUS ftpdInit
    (
    FUNCPTR 	pLoginRtn, 	/* user verification routine, or NULL */
    int 	stackSize 	/* task stack size, or 0 for default */
    )
    {
    int 		on = 1;
    struct sockaddr_in 	ctrlAddr;

    if (ftpsActive)
	return (OK);

    loginVerifyRtn = pLoginRtn;

    ftpsShutdownFlag = FALSE;
    ftpsCurrentClients = 0;

    /* Create the FTP server control socket. */

    ftpdServerSock = socket (AF_INET, SOCK_STREAM, 0);
    if (ftpdServerSock < 0)
        return (ERROR);

    /* Create data structures for managing client connections. */

    lstInit (&ftpsSessionList);
    ftpsMutexSem = semMCreate (SEM_Q_FIFO | SEM_DELETE_SAFE);
    if (ftpsMutexSem == NULL)
        {
        close (ftpdServerSock);
        return (ERROR);
        }

    ftpsSignalSem = semBCreate (SEM_Q_FIFO, SEM_EMPTY);
    if (ftpsSignalSem == NULL)
        {
        close (ftpdServerSock);
        semDelete (ftpsMutexSem);
        return (ERROR);
        }

    /* Setup control connection for client requests. */

    ctrlAddr.sin_family = AF_INET;
    ctrlAddr.sin_addr.s_addr = INADDR_ANY;
    ctrlAddr.sin_port = htons (FTP_DAEMON_PORT);

    if (setsockopt (ftpdServerSock, SOL_SOCKET, SO_REUSEADDR,
                    (char *) &on, sizeof (on)) < 0)
        {
        close (ftpdServerSock);
        semDelete (ftpsMutexSem);
        semDelete (ftpsSignalSem);
        return (ERROR);
        }

    if (bind (ftpdServerSock, (struct sockaddr *) &ctrlAddr,
              sizeof (ctrlAddr)) < 0)
        {
        close (ftpdServerSock);
        semDelete (ftpsMutexSem);
        semDelete (ftpsSignalSem);
        return (ERROR);
        }

    if (listen (ftpdServerSock, 5) < 0)
        {
        close (ftpdServerSock);
        semDelete (ftpsMutexSem);
        semDelete (ftpsSignalSem);
        return (ERROR);
        }

    /* Create a FTP server task to receive client requests. */

    ftpdTaskId = taskSpawn ("tFtpdTask", ftpdTaskPriority - 1, ftpdTaskOptions,
                            stackSize == 0 ? ftpdWorkTaskStackSize : stackSize,
                            (FUNCPTR) ftpdTask, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    if (ftpdTaskId == ERROR)
        {
        ftpdDebugMsg ("ERROR: ftpdTask cannot be created\n",0,0,0,0);
        close (ftpdServerSock);
        semDelete (ftpsMutexSem);
        semDelete (ftpsSignalSem);
        return (ERROR);
        }

    ftpsActive = TRUE;
    ftpdDebugMsg ("ftpdTask created\n",0,0,0,0);
    return (OK);
    }

/*******************************************************************************
*
* ftpdDelete - terminate the FTP server task
*
* This routine halts the FTP server and closes the control connection. All
* client sessions are removed after completing any commands in progress.
* When this routine executes, no further client connections will be accepted
* until the server is restarted. This routine is not reentrant and must not
* be called from interrupt level.
*
* NOTE: If any file transfer operations are in progress when this routine is
* executed, the transfers will be aborted, possibly leaving incomplete files
* on the destination host.
*
* RETURNS: OK if shutdown completed, or ERROR otherwise.
*
* ERRNO: N/A
*
* INTERNAL
* This routine is synchronized with the deletion routine for a client session
* so that the exit of the client tasks can be detected. It also shares a
* critical section with the creation of client sessions to prevent orphaned
* tasks, which would occur if a session were added after this routine had
* shut down all known clients.
*/

STATUS ftpdDelete (void)
    {
    BOOL serverActive = FALSE;
    FTPD_SESSION_DATA * 	pData;

    if (! ftpsActive)    /* Automatic success if server is not running. */
        return (OK);

    /*
     * Remove the FTP server task to prevent additional sessions from starting.
     * The exclusion semaphore guarantees a stable list of active clients.
     */
    semTake (ftpsMutexSem, WAIT_FOREVER);

    taskDelete (ftpdTaskId);
    ftpdTaskId = -1;

    if (ftpsCurrentClients != 0)
        serverActive = TRUE;

    /*
     * Set the shutdown flag so that any secondary server tasks will exit
     * as soon as possible. Once the FTP server has started, this routine is
     * the only writer of the flag and the secondary tasks are the only
     * readers. To avoid unnecessary blocking, the secondary tasks do not
     * guard access to this flag when checking for a pending shutdown during
     * normal processing. Those tasks do protect access to this flag during
     * their cleanup routine to prevent a race condition which would result
     * in incorrect use of the signalling semaphore.
     */

    ftpsShutdownFlag = TRUE;

    /* 
     * Close the command sockets of any active sessions to prevent further 
     * activity. If the session is waiting for a command from a socket,
     * the close will trigger the session exit.
     */

    pData = (FTPD_SESSION_DATA *)lstFirst (&ftpsSessionList);
    while (pData != NULL)
        {
        ftpdSockFree (&pData->cmdSock);
        pData = (FTPD_SESSION_DATA *)lstNext (&pData->node);
        }

    semGive (ftpsMutexSem);   

    /* Wait for all secondary tasks to exit. */

    if (serverActive)
        {
        /*
         * When a shutdown is in progress, the cleanup routine of the last
         * client task to exit gives the signalling semaphore.
         */

        semTake (ftpsSignalSem, WAIT_FOREVER);
        }

    /*
     * Remove the original socket - this occurs after all secondary tasks
     * have exited to avoid prematurely closing their control sockets.
     */

    ftpdSockFree (&ftpdServerSock);

    /* Remove the protection and signalling semaphores and list of clients. */

    lstFree (&ftpsSessionList);    /* Sanity check - should already be empty. */

    semDelete (ftpsMutexSem);

    semDelete (ftpsSignalSem);

    ftpsActive = FALSE;

    return (OK);
    }

/*******************************************************************************
*
* ftpdSessionAdd - add a new entry to the ftpd session slot list
*
* Each of the incoming FTP sessions is associated with an entry in the
* FTP server's session list which records session-specific context for each
* control connection established by the FTP clients. This routine creates and
* initializes a new entry in the session list, unless the needed memory is not
* available or the upper limit for simultaneous connections is reached.
*
* RETURNS: A pointer to the session list entry, or NULL of none available.
*
* ERRNO: N/A
*
* NOMANUAL
*
* INTERNAL
* This routine executes within a critical section of the primary FTP server
* task, so mutual exclusion is already present when adding entries to the
* client list and updating the shared variables indicating the current number
* of connected clients.
*/

LOCAL FTPD_SESSION_DATA *ftpdSessionAdd (void)
    {
    FAST FTPD_SESSION_DATA 	*pSlot;

    if (ftpsCurrentClients == ftpsMaxClients)
        return (NULL);

    /* get memory for the new session entry */

    pSlot = (FTPD_SESSION_DATA *) KHEAP_ALLOC(sizeof (FTPD_SESSION_DATA));
    if (pSlot == NULL)
	{
	return (NULL);
	}
    bzero ((char *)pSlot, sizeof(FTPD_SESSION_DATA));

    /* initialize key fields in the newly acquired slot */

    pSlot->dataSock = FTPD_SOCK_FREE;
    pSlot->cmdSock = FTPD_SOCK_FREE;
    pSlot->cmdSockError = OK;
    pSlot->status = FTPD_STREAM_MODE | FTPD_ASCII_TYPE | FTPD_NO_RECORD_STRU;
    pSlot->byteCount = 0;

    /* assign the default directory for this guy */

    ioDefPathGet (pSlot->curDirName);

    /* Add new entry to the list of active sessions. */

    lstAdd (&ftpsSessionList, &pSlot->node);
    ftpdNumTasks++;
    ftpsCurrentClients++;

    return (pSlot);
    }

/*******************************************************************************
*
* ftpdSessionDelete - remove an entry from the FTP session list
*
* This routine removes the session-specific context from the session list
* after the client exits or a fatal error occurs.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*
* INTERNAL
* Unless an error occurs, this routine is only called in response to a
* pending server shutdown, indicated by the shutdown flag. Even if the
* shutdown flag is not detected and this routine is called because of an
* error, the appropriate signal will still be sent to any pending shutdown
* routine. The shutdown routine will only return after any active client
* sessions have exited.
*/

LOCAL void ftpdSessionDelete
    (
    FAST FTPD_SESSION_DATA *pSlot       /* pointer to the slot to be deleted */
    )
    {
    if (pSlot == NULL)			/* null slot? don't do anything */
        return;

    /*
     * The deletion of a session entry must be an atomic operation to support
     * an upper limit on the number of simultaneous connections allowed.
     * This mutual exclusion also prevents a race condition with the server
     * shutdown routine. The last client session will always send an exit
     * signal to the shutdown routine, whether or not the shutdown flag was
     * detected during normal processing.
     */

    semTake (ftpsMutexSem, WAIT_FOREVER);
    --ftpdNumTasks;
    --ftpsCurrentClients;
    lstDelete (&ftpsSessionList, &pSlot->node);

    ftpdSockFree (&pSlot->cmdSock);	/* release data and command sockets */
    ftpdSockFree (&pSlot->dataSock);

    KHEAP_FREE((char *)pSlot);

    /* Send required signal if all sessions are closed. */

    if (ftpsShutdownFlag)
        {
        if (ftpsCurrentClients == 0)
            semGive (ftpsSignalSem);
        }
    semGive (ftpsMutexSem);

    return;
    }

/*******************************************************************************
*
* ftpdWorkTask - main protocol processor for the FTP service
*
* This function handles all the FTP protocol requests by parsing
* the request string and performing appropriate actions and returning
* the result strings.  The main body of this function is a large
* FOREVER loop which reads in FTP request commands from the client
* located on the other side of the connection.  If the result of
* parsing the request indicates a valid command, ftpdWorkTask() will
* call appropriate functions to handle the request and return the
* result of the request.  The parsing of the requests are done via
* a list of strncmp routines for simplicity.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*
* INTERNAL
* To handle multiple simultaneous connections, this routine and all secondary
* routines which process client commands must be re-entrant. If the server's
* halt routine is started, the shutdown flag is set, causing this routine to
* exit after completing any operation already in progress.
*/

LOCAL STATUS ftpdWorkTask
    (
    FTPD_SESSION_DATA   *pSlot  /* pointer to the active slot to be handled */
    )
    {
    FAST int		sock;		/* command socket descriptor */
    FAST char		*pBuf;		/* pointer to session specific buffer */
    struct sockaddr_in	passiveAddr;	/* socket address in passive mode */
    FAST char		*dirName;	/* directory name place holder */
    FAST int		numRead;
    int			addrLen = sizeof (passiveAddr);	/* for getpeername */
    int			portNum [6];	/* used for "%d,%d,%d,%d,%d,%d" */
    u_long 		value;
    char 		*pTail;
    char 		newPath [MAX_FILENAME_LENGTH];
    char 		curDirName [MAX_FILENAME_LENGTH];
    char		*pFileName;
    FILE		*inStream;
    FILE 		*outStream;
    char                *upperCommand;    /* convert command to uppercase */


    pBuf = &pSlot->buf [0];	/* use session specific buffer area */
    sock = pSlot->cmdSock;

    if (ftpsShutdownFlag)
        {
        /* Server halt in progress - send abort message to client. */

        ftpdCmdSend (pSlot, sock, 421, 
                     "Service not available, closing control connection",
                     0, 0, 0, 0, 0, 0);
        ftpdSessionDelete (pSlot);
        return (OK);
        }

    /* tell the client we're ready to rock'n'roll */
#ifdef _WRS_VXWORKS_5_X
    if (ftpdCmdSend (pSlot, sock, 220, messages [MSG_SERVER_READY],
		(int)vxWorksVersion, 0, 0, 0, 0, 0) == ERROR)
#else
        if (ftpdCmdSend (pSlot, sock, 220, messages [MSG_SERVER_READY],
                         (int)runtimeVersion, 0, 0, 0, 0, 0) == ERROR)
#endif /* _WRS_VXWORKS_5_X */
	{
	ftpdSessionDelete (pSlot);
	return (ERROR);
	}

    FOREVER
	{

	taskDelay (1);		/* time share among same priority tasks */

	/* Check error in writting to the control socket */

	if (pSlot->cmdSockError == ERROR)
	    {
	    ftpdSessionDelete (pSlot);
	    return (ERROR);
	    }

        /*
         * Stop processing client requests if a server halt is in progress.
         * These tests of the shutdown flag are not protected with the
         * mutual exclusion semaphore to prevent unnecessary synchronization
         * between client sessions. Because the secondary tasks execute at
         * a lower priority than the primary task, the worst case delay
         * before ending this session after shutdown has started would only
         * allow a single additional command to be performed.
         */

        if (ftpsShutdownFlag)
            break;

	/* get a request command */

	FOREVER
	    {
	    taskDelay (1);	/* time share among same priority tasks */

	    if ((numRead = read (sock, pBuf, 1)) <= 0)
		{
                /*
                 * The primary server task will close the control connection
                 * when a halt is in progress, causing an error on the socket.
                 * In this case, ignore the error and exit the command loop
                 * to send a termination message to the connected client.
                 */

                if (ftpsShutdownFlag)
                    {
                    *pBuf = EOS;
                    break;
                    }

                /* 
                 * Send a final message if the control socket 
                 * closed unexpectedly.
                 */

		if (numRead == 0)
		    ftpdCmdSend (pSlot, sock, 221, messages [MSG_NO_GOOD_BYE],
		    0, 0, 0, 0, 0, 0);

		ftpdSessionDelete (pSlot);
		return ERROR;
		}
	    
	    /* Skip the CR in the buffer. */
	    if ( *pBuf == '\r' )
		continue;

	    /* End Of Command delimeter. exit loop and process command */
	    if ( *pBuf == '\n' )
	    {
		*pBuf = EOS;
		break;
	    }
	    pBuf++;		/* Advance to next character to read */
	    }

	/*  Reset Buffer Pointer before we use it */
	pBuf = &pSlot->buf [0];

	/* convert the command to upper-case */

	for (upperCommand = pBuf; (*upperCommand != ' ') &&
	     (*upperCommand != EOS); upperCommand++)
	    *upperCommand = toupper (*upperCommand);


	ftpdDebugMsg ("read command %s\n", (int)pBuf,0,0,0);

        /*
         * Send an abort message to the client if a server
         * shutdown was started while reading the next command.
         */

        if (ftpsShutdownFlag)
            {
            ftpdCmdSend (pSlot, sock, 421, 
                         "Service not available, closing control connection",
                         0, 0, 0, 0, 0, 0);
            break;
            }

	if (strncmp (pBuf, "USER", 4) == 0)
	    {
	    /* check user name */

           /* Actually copy the user name into a buffer and save it */
           /* till the password comes in. Name is located one space */
           /* character after USER string */
           if ( *(pBuf + 4) == '\0' )
               pSlot->user[0] = '\0';          /* NOP user for null user */
           else
               strncpy(pSlot->user, pBuf+5, MAX_LOGIN_NAME_LEN);

           pSlot->status &= ~FTPD_USER_OK;


	    if (ftpdCmdSend (pSlot, sock, 331, messages [MSG_PASSWORD_REQUIRED],
			 0, 0, 0, 0, 0, 0) == ERROR)
		{
		ftpdSessionDelete (pSlot);
		return (ERROR);
		}
	    continue;
	    }
	else if (strncmp (pBuf, "PASS", 4) == 0)
	    {
	    /* check user passwd */

           /* Actually check it against earlier supplied user name */
	   if ( loginVerifyRtn != (FUNCPTR)NULL )
	       {
	       if ( (FUNCPTR *)(loginVerifyRtn)(pSlot->user, pBuf+5) != OK )
		   {
		   if (ftpdCmdSend (pSlot, sock,
                                    530, messages [MSG_USER_LOGIN_FAILED], 
                                    0, 0, 0, 0, 0, 0) == ERROR)
		       {
		       ftpdSessionDelete (pSlot);
		       return (ERROR);
		       }
		   pSlot->status &= ~FTPD_USER_OK;
		   continue;
		   }
	        }

	    pSlot->status |= FTPD_USER_OK;
	    if (ftpdCmdSend (pSlot, sock, 230, messages [MSG_USER_LOGGED_IN],
			 0, 0, 0, 0, 0, 0) == ERROR)
		{
		ftpdSessionDelete (pSlot);
		return (ERROR);
		}
	    continue;
	    }
	else if (strncmp (pBuf, "QUIT", 4) == 0)
	    {
	    /* sayonara */

	    ftpdCmdSend (pSlot, sock, 221, messages [MSG_SEE_YOU_LATER],
			 0, 0, 0, 0, 0, 0);
	    ftpdSessionDelete (pSlot);
	    return OK;
	    }
	else if (strncmp (pBuf, "HELP", 4) == 0)
	    {
	    /* send list of supported commands with multiple line response */

	    if (ftpdCmdSend (pSlot, sock, FTPD_MULTI_LINE | 214,
			messages [MSG_COMMAND_LIST_BEGIN], 0, 0, 0, 0, 0, 0) 
			 == ERROR)
		{
		ftpdSessionDelete (pSlot);
		return (ERROR);
		}

	    if (write (pSlot->cmdSock, ftpdCommandList,
				strlen (ftpdCommandList)) <= 0)
		{
		ftpdSessionDelete (pSlot);
		return (ERROR);
		}

	    /* this signifies the end of the multiple line response */

	    if (ftpdCmdSend (pSlot, sock, 214, messages [MSG_COMMAND_LIST_END],
			 0, 0, 0, 0, 0, 0) == ERROR)
		
		{
		ftpdSessionDelete (pSlot);
		return (ERROR);
		}
	    continue;		/* All is well go wait for the next command */
	    }
	else if ((pSlot->status & FTPD_USER_OK) == 0)	/* validated yet? */
	    {
	    /* user is not validated yet.  tell him to log in first */

	    if (ftpdCmdSend (pSlot, sock, 530, messages [MSG_USER_PASS_REQ],
			 0, 0, 0, 0, 0, 0) == ERROR)
		
		{
		ftpdSessionDelete (pSlot);
		return (ERROR);
		}

	    /* do not proceed further until he's legit */

	    continue;
	    }

	if (strncmp (pBuf, "LIST", 4) == 0 ||
		 strncmp (pBuf, "NLST", 4) == 0)
	    {
	    STATUS retVal;

	    /* client wants to list out the contents of a directory */

	    /* if no directory specified or "." specified as a directory
	     * we use the currently active directory name
	     */

	    if (strlen (pBuf) < 6 || pBuf [5] == '.')
	        dirName = &pSlot->curDirName [0];
	    else if (pBuf [5] != '/')
		{
		if (pSlot->curDirName [strlen (pSlot->curDirName) - 1] == '/')
		    (void) sprintf (newPath,
				    "%s%s", pSlot->curDirName, &pBuf [5]);
		else
		    (void) sprintf (newPath,
				    "%s/%s", pSlot->curDirName, &pBuf [5]);
	        dirName = newPath;
               } 
            else
               dirName = &pBuf [5];

	    ftpdDebugMsg ("LIST %s\n", (int)dirName,0,0,0);

	    /* get a new data socket connection for the transmission of
	     * the directory listing data
	     */

	    if (ftpdDataConnGet (pSlot) == ERROR)
		{
		if (ftpdCmdSend (pSlot, sock, 
                                 426, messages [MSG_DATA_CONN_ERROR],
                                 0, 0, 0, 0, 0, 0) == ERROR)
		    
		    {
		    ftpdSessionDelete (pSlot);
		    return (ERROR);
		    }
		continue;
		}

	    /* print out the directory contents over the data connection */

	    retVal = ftpdDirListGet (pSlot->dataSock, dirName,
				     (strncmp (pBuf, "LIST", 4) == 0));

	    if (retVal == ERROR)
		{
		if (ftpdCmdSend (pSlot, sock, 550, messages [MSG_DIR_ERROR],
			     0, 0, 0, 0, 0, 0) == ERROR)
		    
		    {
		    ftpdSessionDelete (pSlot);
		    return (ERROR);
		    }
		}
	    else
		{
                if (ftpdCmdSend (pSlot, sock, 
                                 226, messages [MSG_TRANS_COMPLETE],
                                 0, 0, 0, 0, 0, 0) == ERROR)
		    {
		    ftpdSessionDelete (pSlot);
		    return (ERROR);
		    }
		}

	    /* free up the data socket */

	    ftpdSockFree (&pSlot->dataSock);
	    }
	else if (strncmp (pBuf, "RETR", 4) == 0)
	    {
	    /* retrieve a file */

	    /* open the file to be sent to the client */

	    if (pBuf [5] != '/')
		{
		if (pSlot->curDirName [strlen (pSlot->curDirName) - 1] == '/')
		    (void) sprintf (newPath,
				    "%s%s", pSlot->curDirName, &pBuf [5]);
		else
		    (void) sprintf (newPath,
				    "%s/%s", pSlot->curDirName, &pBuf [5]);

		pFileName = newPath;
		}
	    else
	        pFileName = &pBuf [5];

	    ftpdDebugMsg ("RETR %s\n", (int)pFileName,0,0,0);

	    if ((inStream = fopen (pFileName, "r")) == NULL)
	        {
		if (ftpdCmdSend (pSlot, sock, 550, messages [MSG_FILE_ERROR],
			     (int)(&pBuf[5]), 0, 0, 0, 0, 0) == ERROR)
		    
		    {
		    ftpdSessionDelete (pSlot);
		    return (ERROR);
		    }
		continue;
		}

	    /* ship it away */

	    ftpdDataStreamSend (pSlot, inStream);
	    (void) fclose (inStream);
	    }
	else if (strncmp (pBuf, "STOR", 4) == 0)
	    {
	    /* store a file */

	    /* create a local file */

	    if (pBuf [5] != '/')
		{
		if (pSlot->curDirName [strlen (pSlot->curDirName) - 1] == '/')
		    (void) sprintf (newPath,
				    "%s%s", pSlot->curDirName, &pBuf [5]);
		else
		    (void) sprintf (newPath,
				    "%s/%s", pSlot->curDirName, &pBuf [5]);

		pFileName = newPath;
		}
	    else
	        pFileName = &pBuf [5];

	    ftpdDebugMsg ("STOR %s\n", (int)pFileName,0,0,0);

	    if ((outStream = fopen (pFileName, "w")) == NULL)
	        {
		if (ftpdCmdSend (pSlot, sock, 553, messages[MSG_CREATE_ERROR],
			(int)(&pBuf[5]), 0, 0, 0, 0, 0) == ERROR)

		    {
		    ftpdSessionDelete (pSlot);
		    return (ERROR);
		    }
		continue;
		}

	    /* receive the file */

	    ftpdDataStreamReceive (pSlot, outStream);
	    (void) fclose (outStream);
	    }
	else if (strncmp (pBuf, "CWD", 3) == 0)
	    {
	    /* change directory */

	    dirName = &pBuf [4];

	    /* there is no default device for the specified directory */

#ifdef _WRS_VXWORKS_5_X
	    if (iosDevFind (dirName, (char **)&pTail) == NULL)
#else
	    if (iosDevFind (dirName, (const char **)&pTail) == NULL)
#endif
		{
                if (ftpdCmdSend (pSlot, sock, 
                                 501, messages [MSG_DIR_NOT_PRESENT],
                                 0, 0, 0, 0, 0, 0) == ERROR)
			    
		    {
		    ftpdSessionDelete (pSlot);
		    return (ERROR);
		    }
		continue;
		}

	    /* dirName doesn't start with a device name, prepend old path */

	    if (dirName == pTail)
		{
		(void) strcpy (curDirName, pSlot->curDirName);
		(void) pathCat (curDirName, dirName, newPath);
		}
	    else				/* it starts with a dev name */
		(void) strcpy (newPath, dirName);/* use the whole thing */

	    pathCondense (newPath);		/* condense ".." shit */

	    /* remember where we are */

	    (void) strcpy (pSlot->curDirName, newPath);

	    /* notify successful chdir */

	    if (ftpdCmdSend (pSlot, sock, 250, messages [MSG_CHANGED_DIR],
			    (int)newPath, 0, 0, 0, 0, 0) == ERROR)

		{
		ftpdSessionDelete (pSlot);
		return (ERROR);
		}
	    }
	else if (strncmp (pBuf, "TYPE", 4) == 0)
	    {
	    /* we only support BINARY and ASCII representation types */

	    if (pBuf [5] == 'I' || pBuf [5] == 'i' ||
		pBuf [5] == 'L' || pBuf [5] == 'l')
		{
	        pSlot->status |= FTPD_BINARY_TYPE;
		pSlot->status &= ~FTPD_ASCII_TYPE;
		if (ftpdCmdSend (pSlot, sock, 200, messages [MSG_TYPE_BINARY],
			     0, 0, 0, 0, 0, 0) == ERROR)
			
		    {
		    ftpdSessionDelete (pSlot);
		    return (ERROR);
		    }
		}
	    else if (pBuf [5] == 'A' || pBuf [5] == 'a')
		{
	        pSlot->status |= FTPD_ASCII_TYPE;
		pSlot->status &= ~FTPD_BINARY_TYPE;
		if (ftpdCmdSend (pSlot, sock, 200, messages [MSG_TYPE_ASCII],
			     0, 0, 0, 0, 0, 0) == ERROR)

		    {
		    ftpdSessionDelete (pSlot);
		    return (ERROR);
		    }
		}
	    else
		{
		if (ftpdCmdSend (pSlot, sock, 504, messages [MSG_PARAM_BAD],
			     0, 0, 0, 0, 0, 0) == ERROR)
		    
		    {
		    ftpdSessionDelete (pSlot);
		    return (ERROR);
		    }
		}
	    }
	else if (strncmp (pBuf, "PORT", 4) == 0)
	    {
	    /* client specifies the port to be used in setting up
	     * active data connections later on (see ftpdDataConnGet ()).
	     * format:  first four decimal digits separated by commas
	     * indicate the internet address; the last two decimal
	     * digits separated by a comma represents hi and low
	     * bytes of a port number.
	     */

	    (void) sscanf (&pBuf [5], "%d,%d,%d,%d,%d,%d",
			   &portNum [0], &portNum [1], &portNum [2],
			   &portNum [3], &portNum [4], &portNum [5]);

	    pSlot->dataAddr.sin_port = portNum [4] * 256 + portNum [5];

	    /* convert port number to network byte order */

	    pSlot->dataAddr.sin_port = htons (pSlot->dataAddr.sin_port);

            /* Set remote host to given value. */

            value = (portNum [0] << 24) | (portNum [1] << 16) |
                    (portNum [2] << 8) | portNum [3];
            pSlot->dataAddr.sin_addr.s_addr = htonl (value);

	    if (ftpdCmdSend (pSlot, sock, 200, messages [MSG_PORT_SET],
			 0, 0, 0, 0, 0, 0) == ERROR)

		{
		ftpdSessionDelete (pSlot);
		return (ERROR);
		}
	    }
	else if (strncmp (pBuf, "PWD", 3) == 0)
	    {
	    /* get current working directory */

	    (void) strcpy (pBuf, pSlot->curDirName);
	    if (ftpdCmdSend (pSlot, sock, 257, messages [MSG_CUR_DIR],
			 (int)pBuf, 0, 0, 0, 0, 0) == ERROR)

		{
		ftpdSessionDelete (pSlot);
		return (ERROR);
		}
	    }
	else if (strncmp (pBuf, "STRU", 4) == 0)
	    {
	    /* specify the file structure */

	    /* we only support normal byte stream oriented files;
	     * we don't support IBM-ish record block oriented files
	     */

	    if (pBuf [5] == 'F' || pBuf [5] == 'f')
		{
	        if (ftpdCmdSend (pSlot, sock, 200, messages [MSG_FILE_STRU],
			     0, 0, 0, 0, 0, 0) == ERROR)
		    
		    {
		    ftpdSessionDelete (pSlot);
		    return (ERROR);
		    }
		}
	    else
		{
	        if (ftpdCmdSend (pSlot, sock, 504, messages [MSG_PARAM_BAD],
			     0, 0, 0, 0, 0, 0) == ERROR)

		    {
		    ftpdSessionDelete (pSlot);
		    return (ERROR);
		    }
		}
	    }
	else if (strncmp (pBuf, "MODE", 4) == 0)
	    {
	    /* specify transfer mode */

	    /* we only support stream mode -- no block or compressed mode */

	    if (pBuf [5] == 'S' || pBuf [5] == 's')
		{
	        if (ftpdCmdSend (pSlot, sock, 200, messages [MSG_STREAM_MODE],
			     0, 0, 0, 0, 0, 0) == ERROR)

		    {
		    ftpdSessionDelete (pSlot);
		    return (ERROR);
		    }
		}
	    else
		{
	        if (ftpdCmdSend (pSlot, sock, 504, messages [MSG_PARAM_BAD],
			     0, 0, 0, 0, 0, 0) == ERROR)
		    {
		    ftpdSessionDelete (pSlot);
		    return (ERROR);
		    }
		}
	    }
	else if (strncmp (pBuf, "ALLO", 4) == 0 ||
		 strncmp (pBuf, "ACCT", 4) == 0)
	    {
	    /* allocate and account commands are not need */

	    if (ftpdCmdSend (pSlot, sock, 202, messages [MSG_ALLOC_ACCOUNT],
			 0, 0, 0, 0, 0, 0) == ERROR)

		{
		ftpdSessionDelete (pSlot);
		return (ERROR);
		}
	    }
	else if (strncmp (pBuf, "PASV", 4) == 0)
	    {
	    /* client wants to connect to us instead of waiting
	     * for us to make a connection to its data connection
	     * socket
	     */

	    ftpdSockFree (&pSlot->dataSock);

	    /* we need to open a socket and start listening on it
	     * to accommodate his request.
	     */

	    if ((pSlot->dataSock = socket (AF_INET, SOCK_STREAM, 0)) < 0)
		{
		if (ftpdCmdSend (pSlot, sock, 425, messages [MSG_PASSIVE_ERROR],
			     0, 0, 0, 0, 0, 0) == ERROR)

		    {
		    ftpdSessionDelete (pSlot);
		    return (ERROR);
		    }
		}
	    else
		{
                int outval1;
                int outval2;
                int outval3;
                int outval4;
                int outval5;
                int outval6;

                if (getsockname (pSlot->cmdSock,
                                 (struct sockaddr *) &pSlot->dataAddr,
                                 &addrLen) < 0)
                    {
                    /* Couldn't find address for local end of connection. */

                    if (ftpdCmdSend (pSlot, sock,
                                     425, messages [MSG_PASSIVE_ERROR],
                                     0, 0, 0, 0, 0, 0) == ERROR)
                        {
                        ftpdSessionDelete (pSlot);
                        return (ERROR);
                        }
                    }

                /*
                 * Find an ephemeral port for the expected connection
                 * and initialize connection queue. 
                 */

                pSlot->dataAddr.sin_port = htons (0);
                addrLen = sizeof (struct sockaddr_in);

		if (bind (pSlot->dataSock, (struct sockaddr *)&pSlot->dataAddr,
			  sizeof (struct sockaddr_in)) < 0 ||
		    getsockname (pSlot->dataSock,
				 (struct sockaddr *) &pSlot->dataAddr,
				 &addrLen) < 0 ||
		    listen (pSlot->dataSock, 1) < 0)
		    {
		    ftpdSockFree (&pSlot->dataSock);
                    if (ftpdCmdSend (pSlot, sock, 
                                     425, messages [MSG_PASSIVE_ERROR],
                                     0, 0, 0, 0, 0, 0) == ERROR)
			{
			ftpdSessionDelete (pSlot);
			return (ERROR);
			}
		    continue;
		    }

		/* we're passive, let us keep that in mind */

		pSlot->status |= FTPD_PASSIVE;

                value = pSlot->dataAddr.sin_addr.s_addr;
                outval1 = ( (u_char *)&value)[0];
                outval2 = ( (u_char *)&value)[1];
                outval3 = ( (u_char *)&value)[2];
                outval4 = ( (u_char *)&value)[3];

                 /* Separate port number into bytes. */

                outval5 = ( (u_char *)&pSlot->dataAddr.sin_port)[0];
                outval6 = ( (u_char *)&pSlot->dataAddr.sin_port)[1];

		/* tell the client to which port to connect */

                if (ftpdCmdSend (pSlot, pSlot->cmdSock, 
                                 227, messages [MSG_PASSIVE_MODE],
                                 outval1, outval2, outval3, outval4,
                                 outval5, outval6) == ERROR)

		    {
		    ftpdSessionDelete (pSlot);
		    return (ERROR);
		    }
		}
	    }
	else if (strncmp (pBuf, "NOOP", 4) == 0)
	    {
	    /* don't do anything */

	    if (ftpdCmdSend (pSlot, sock, 200, messages [MSG_NOOP_OKAY],
			 0, 0, 0, 0, 0, 0) == ERROR)

		{
		ftpdSessionDelete (pSlot);
		return (ERROR);
		}
	    }
        else if (strncmp (pBuf, "DELE", 4) == 0)
            {
             if (pBuf [5] != '/')
                 {
                 if (pSlot->curDirName [strlen (pSlot->curDirName) - 1] == '/')
                     (void) sprintf (newPath,
                                     "%s%s", pSlot->curDirName, &pBuf [5]);
                 else
                     (void) sprintf (newPath,
                                     "%s/%s", pSlot->curDirName, &pBuf [5]);

                 pFileName = newPath;
                 }
             else
                 pFileName = &pBuf [5];

             ftpdDebugMsg ("DELE %s\n", (int)pFileName,0,0,0);

             if (remove (pFileName) != OK)
                 {
                 if (ftpdCmdSend (pSlot, sock, 550, messages [MSG_FILE_ERROR],
                                  (int) pFileName, 0, 0, 0, 0, 0) == ERROR)
                     {
                      ftpdSessionDelete (pSlot);
                      return (ERROR);
                      }
                 continue;
                 }
             else
                 {
                  if (ftpdCmdSend (pSlot, sock, 250, messages [MSG_DELE_OKAY],
                                   0, 0, 0, 0, 0, 0) == ERROR)
                      {
                       ftpdSessionDelete (pSlot);
                       return (ERROR);
                      }
                 }
           }
       else
           {
	    /* unrecognized command or command not supported */

	    if (ftpdCmdSend (pSlot, sock, 500, messages [MSG_BAD_COMMAND],
			 0, 0, 0, 0, 0, 0) == ERROR)

		{
		ftpdSessionDelete (pSlot);
		return (ERROR);
		}
	    }
	}

    /*
     * Processing halted due to pending server shutdown.
     * Remove all resources and exit.
     */

    ftpdSessionDelete (pSlot);
    return (OK);
    }

/*******************************************************************************
*
* ftpdDataConnGet - get a fresh data connection socket for FTP data transfer
*
* FTP uses upto two connections per session (as described above) at any
* time.  The command connection (cmdSock) is maintained throughout the
* FTP session to pass the request command strings and replies between
* the client and the server.  For commands that require bulk data transfer
* such as contents of a file or a list of files in a directory, FTP
* sets up dynamic data connections separate from the command connection.
* This function, ftpdDataConnGet, is responsible for creating
* such connections.
*
* Setting up the data connection is performed in two ways.  If the dataSock
* is already initialized and we're in passive mode (as indicated by the
* FTPD_PASSIVE bit of the status field in the FTPD_SESSION_SLOT) we need to
* wait for our client to make a connection to us -- so we just do an accept
* on this already initialized dataSock.  If the dataSock is already
* initialized and we're not in passive mode, we just use the already
* existing connection.  Otherwise, we need to initialize a new socket and
* make a connection to the the port where client is accepting new
* connections.  This port number is in general set by "PORT" command (see
* ftpdWorkTask()).
*/

LOCAL STATUS ftpdDataConnGet
    (
    FTPD_SESSION_DATA   *pSlot          /* pointer to the work slot */
    )
    {
    FAST int		newSock;	/* new connection socket */
    int			addrLen;	/* to be used with accept */
    struct sockaddr_in	addr;		/* to be used with accept */
    int			on = 1;		/* to be used to turn things on */
    int			retry = 0;	/* retry counter initialized to zero */

    /* command socket is invalid, return immediately */

    if (pSlot->cmdSock == FTPD_SOCK_FREE)
        return (ERROR);

    pSlot->byteCount = 0;

    if (pSlot->dataSock != FTPD_SOCK_FREE)
        {
	/* data socket is already initialized */

	/* are we being passive? (should we wait for client to connect
	 * to us rather than connecting to the client?)
	 */

	if (pSlot->status & FTPD_PASSIVE)
	    {
	    /* we're being passive.  wait for our client to connect to us. */

	    addrLen = sizeof (struct sockaddr);

	    if ((newSock = accept (pSlot->dataSock, (struct sockaddr *) &addr,
				   &addrLen)) < 0)
		{
                ftpdCmdSend (pSlot, pSlot->cmdSock,
                             425, "Can't open data connection",
                             0, 0, 0, 0, 0, 0);

                ftpdSockFree (&pSlot->dataSock);

		/* we can't be passive no more */

		pSlot->status &= ~FTPD_PASSIVE;

		return (ERROR);
		}

            /*
             * Enable the keep alive option to prevent misbehaving clients
             * from locking the server.
             */

            if (setsockopt (newSock, SOL_SOCKET, SO_KEEPALIVE, (char *) &on,
                            sizeof (on)) != 0)
                {
                ftpdSockFree (&pSlot->dataSock);
                return (ERROR);
                }

	/* Check for window size validity */

        if (ftpdWindowSize < 0 || ftpdWindowSize > 65536)
            ftpdWindowSize = FTPD_WINDOW_SIZE;

        /* set the window size  */

        if (setsockopt(newSock, SOL_SOCKET, SO_SNDBUF,
            (char *)&ftpdWindowSize, sizeof (ftpdWindowSize)))
                printf("Couldn't set the Send Window to 10k\n");

        if (setsockopt(newSock, SOL_SOCKET, SO_RCVBUF,
            (char *)&ftpdWindowSize, sizeof (ftpdWindowSize)))
                printf("Couldn't set the Send Window to 10k\n");

	    /* replace the dataSock with our new connection */

	    (void) close (pSlot->dataSock);
	    pSlot->dataSock = newSock;

	    /* N.B.: we stay passive */

            if (ftpdCmdSend (pSlot, pSlot->cmdSock,
                             150, "Opening %s mode data connection",
                             pSlot->status & FTPD_ASCII_TYPE ? (int) "ASCII"
                             : (int) "BINARY", 0, 0, 0, 0, 0) == ERROR)
		{
		(void) close (pSlot->dataSock);
		return (ERROR);
		}

	    return (OK);
	    }
	else
	    {
	    /* reuse the old connection -- it's still useful */

            if (ftpdCmdSend (pSlot, pSlot->cmdSock, 
                             125, "Using existing data connection",
                             0, 0, 0, 0, 0, 0) == ERROR)
		{
	    	ftpdSockFree (&pSlot->dataSock);
		return (ERROR);
		}
	    return (OK);
	    }
	}
    else
        {
        /* Determine address for local end of connection. */

        addrLen = sizeof (struct sockaddr);

        if (getsockname (pSlot->cmdSock, (struct sockaddr *) &addr, &addrLen)
                < 0)
            {
            return (ERROR);
            }

        /* Replace control port with default data port. */

        addr.sin_port = htons (FTP_DATA_PORT);

	/* open a new data socket */

	if ((pSlot->dataSock = socket (AF_INET, SOCK_STREAM, 0)) < 0)
	    { 
	    return (ERROR);
	    }

	if (setsockopt (pSlot->dataSock, SOL_SOCKET,
			SO_REUSEADDR, (char *) &on, sizeof (on)) < 0 ||
	    bind (pSlot->dataSock, (struct sockaddr *) &addr,
		  sizeof (addr)) < 0)
	    {
	    ftpdSockFree (&pSlot->dataSock);
	    return (ERROR);
	    }


        /* Set socket address to PORT command values or default. */

        bcopy ( (char *)&pSlot->dataAddr, (char *)&addr, 
               sizeof (struct sockaddr_in));

	/* try until we get a connection to the client's port */

	while (connect (pSlot->dataSock,
			(struct sockaddr *) &addr, sizeof (addr)) < 0)
	    {
	    if ((errno & 0xffff) == EADDRINUSE && retry < FTPD_WAIT_MAX)
	        {
		taskDelay (FTPD_WAIT_INTERVAL * sysClkRateGet ());
		retry += FTPD_WAIT_INTERVAL;
		continue;
		}

	    /* timeout -- we give up */
            ftpdCmdSend (pSlot, pSlot->cmdSock,
                         425, "Can't build data connection",
                         0, 0, 0, 0, 0, 0);
	    ftpdSockFree (&pSlot->dataSock);
	    return (ERROR);
	    }

            /*
             * Enable the keep alive option to prevent misbehaving clients
             * from locking the secondary task during file transfers.
             */

            if (setsockopt (pSlot->dataSock, SOL_SOCKET, SO_KEEPALIVE, 
                            (char *) &on, sizeof (on)) != 0)
                {
                ftpdSockFree (&pSlot->dataSock);
                return (ERROR);
                }

	    /* Check for window size validity */

	    if (ftpdWindowSize < 0 || ftpdWindowSize > 65536)
		ftpdWindowSize = FTPD_WINDOW_SIZE;

	    /* set the window size  */

	    if (setsockopt(pSlot->dataSock, SOL_SOCKET, SO_SNDBUF,
		(char *)&ftpdWindowSize, sizeof (ftpdWindowSize)))
		    printf("Couldn't set the Send Window to 10k\n");

	    if (setsockopt(pSlot->dataSock, SOL_SOCKET, SO_RCVBUF,
		(char *)&ftpdWindowSize, sizeof (ftpdWindowSize)))
		    printf("Couldn't set the Send Window to 10k\n");

            if (ftpdCmdSend (pSlot, pSlot->cmdSock, 
                             150, "Opening %s mode data connection",
                             pSlot->status & FTPD_ASCII_TYPE ? (int) "ASCII" :
                             (int) "BINARY", 0, 0, 0, 0, 0) == ERROR)
		{
		ftpdSockFree (&pSlot->dataSock);
		return (ERROR);
		}
	}

    return (OK);
    }

/*******************************************************************************
*
* ftpdDataStreamSend - send FTP data over data connection
*
* When our FTP client does a "RETR" (send me a file) and we find an existing
* file, ftpdWorkTask() will call us to perform the actual shipment of the
* file in question over the data connection.
*
* We do the initialization of the new data connection ourselves here
* and make sure that everything is fine and dandy before shipping the
* contents of the file.  Special attention is given to the type of
* the file representation -- ASCII or BINARY.  If it's binary, we
* don't perform the prepending of "\r" character in front of each
* "\n" character.  Otherwise, we have to do this for the ASCII files.
*
* SEE ALSO:
* ftpdDataStreamReceive  which is symmetric to this function.
*/

LOCAL void ftpdDataStreamSend
    (
    FTPD_SESSION_DATA   *pSlot,         /* pointer to our session slot */
    FILE                *inStream       /* pointer to the input file stream */
    )
    {
    FAST char	*pBuf;			/* pointer to the session buffer */
    FAST int	netFd;			/* output socket */
    FAST int	fileFd;			/* input file descriptor */
    FAST char	ch;			/* character holder */
    FAST int	cnt;			/* number of chars read/written */
    FAST FILE	*outStream;		/* buffered output socket stream */
    int		retval = 0;

    /* get a fresh connection or reuse the old one */

    if (ftpdDataConnGet (pSlot) == ERROR)
	{
	dataError (pSlot);
	return;
	}

    pBuf = &pSlot->buf [0];

    if (pSlot->status & FTPD_ASCII_TYPE)
	{
	/* ASCII representation */

	/* get a buffered I/O stream for this output data socket */

	if ((outStream = fdopen (pSlot->dataSock, "w")) == NULL)
	    {
	    dataError (pSlot);
	    return;
	    }

	/* write out the contents of the file and do the '\r' prepending */

	while ((ch = getc (inStream)) != (char) EOF)
	    {
	    pSlot->byteCount++;

	    /* if '\n' is encountered, we prepend a '\r' */

	    if (ch == '\n')
		{
		if (ferror (outStream))
		    {
		    dataError (pSlot);
		    fclose (outStream);
		    return;
		    }

		if (putc ('\r', outStream) == EOF)
		    {
		    dataError (pSlot);
		    fclose (outStream);
		    return;
		    }
		}

	    	if (putc (ch, outStream) == EOF)
		{
		dataError (pSlot);
		fclose (outStream);
		return;
		}

            /* Abort the file transfer if a shutdown is in progress. */

            if (ch == '\n' && ftpsShutdownFlag)
                {
		dataError (pSlot);
                fclose (outStream);
                return;
                }
	    }

	/* flush it out */

	(void) fflush (outStream);

	if (ferror (inStream))
	    {
	    /* error in reading the file */

	    fileError (pSlot);
	    fclose (outStream);
	    return;
	    }

	if (ferror (outStream))
	    {
	    /* error in sending the file */

	    dataError (pSlot);
	    fclose (outStream);
	    return;
	    }

	fclose (outStream);

	/* everything is okay */
	transferOkay (pSlot);
	}
    else if (pSlot->status & FTPD_BINARY_TYPE)
	{
	/* BINARY representation */

	netFd = pSlot->dataSock;

	/* get a raw descriptor for this input file */

	fileFd = fileno (inStream);

	/* unbuffered block I/O between file and network */

	while ((cnt = read (fileFd, pBuf, BUFSIZE)) > 0 &&
	       (retval = write (netFd, pBuf, cnt)) == cnt)
            {
	    pSlot->byteCount += cnt;

            if (ftpsShutdownFlag)
                {
                /* Abort the file transfer if a shutdown is in progress. */

                cnt = 1;
                break;
                }
            }

	/* cnt should be zero if the transfer ended normally */

	if (cnt != 0)
	    {
	    if (cnt < 0)
		{
		fileError (pSlot);
		return;
		}

	    ftpdDebugMsg ("read %d bytes, wrote %d bytes\n", cnt, retval,0,0);

	    dataError (pSlot);
	    return;
	    }

	transferOkay (pSlot);
	}
    else
	unImplementedType (pSlot);	/* invalide representation type */
    }

/*******************************************************************************
*
* ftpdDataStreamReceive - receive FTP data over data connection
*
* When our FTP client requests "STOR" command and we were able to
* create a file requested, ftpdWorkTask() will call ftpdDataStreamReceive
* to actually carry out the request -- receiving the contents of the
* named file and storing it in the new file created.
*
* We do the initialization of the new data connection ourselves here
* and make sure that everything is fine and dandy before receiving the
* contents of the file.  Special attention is given to the type of
* the file representation -- ASCII or BINARY.  If it's binary, we
* don't perform the handling of '\r' character in front of each
* '\n' character.  Otherwise, we have to do this for the ASCII files.
*
* SEE ALSO:
* ftpdDataStreamSend which is symmetric to this function.
*/

LOCAL void ftpdDataStreamReceive
    (
    FTPD_SESSION_DATA   *pSlot,
    FILE                *outStream
    )
    {
    FAST char	*pBuf;		/* pointer to the session buffer */
    FAST char 	ch;		/* character holder */
    FAST FILE	*inStream;	/* buffered input file stream for data socket */
    FAST int	fileFd;		/* output file descriptor */
    FAST int	netFd;		/* network file descriptor */
    FAST BOOL	dontPutc;	/* flag to prevent bogus chars */
    FAST int	cnt;		/* number of chars read/written */

    /* get a fresh data connection or reuse the old one */

    if (ftpdDataConnGet (pSlot) == ERROR)
	{
	dataError (pSlot);
	return;
	}

    pBuf = &pSlot->buf [0];

    if (pSlot->status & FTPD_ASCII_TYPE)
	{
	/* ASCII representation */

	/* get a buffer I/O stream for the input data socket connection */

	if ((inStream = fdopen (pSlot->dataSock, "r")) == NULL)
	    {
	    dataError (pSlot);
	    return;
	    }

	/* read in the contents of the file while doing the '\r' handling */

	while ((ch = getc (inStream)) != (char) EOF)
	    {
	    dontPutc = FALSE;

	    pSlot->byteCount++;

	    /* a rather strange handling of sequences of '\r' chars */

	    while (ch == '\r')
		{
		if (ferror (outStream))
		    {
		    dataError (pSlot);
		    fclose (inStream);
		    return;
		    }

		/* replace bogus chars between '\r' and '\n' chars with '\r' */

		if ((ch = getc (inStream)) != '\n')
		    {
		    (void) putc ('\r', outStream);

		    if (ch == '\0' || ch == (char) EOF)
			{
			dontPutc = TRUE;
			break;
			}
		    }
		}

	    if (dontPutc == FALSE)
		(void) putc (ch, outStream);

            /* Abort file transfer if a shutdown is in progress. */

            if (ch == '\n' && ftpsShutdownFlag)
	        {
	        dataError (pSlot);
	        fclose (inStream);
	        return;
                }
            }

	/* flush out to the file */

	(void) fflush (outStream);

	if (ferror (inStream))
	    {
	    /* network input error */

	    dataError (pSlot);
	    fclose (inStream);
	    return;
	    }

	if (ferror (outStream))
	    {
	    /* file output error */

	    fileError (pSlot);
	    fclose (inStream);
	    return;
	    }

	/* we've succeeded! */

	fclose (inStream);
	transferOkay (pSlot);
	}
    else if (pSlot->status & FTPD_BINARY_TYPE)
	{
	/* BINARY representation */

	/* get a raw descriptor for output file stream */

	fileFd = fileno (outStream);

	netFd  = pSlot->dataSock;

	/* perform non-buffered block I/O between network and file */

	while ((cnt = read (netFd, pBuf, BUFSIZE)) > 0)
	    {
	    if (write (fileFd, pBuf, cnt) != cnt)
		{
		fileError (pSlot);
		return;
		}

	    pSlot->byteCount += cnt;

            /* Abort the file transfer if a shutdown is in progress. */

            if (ftpsShutdownFlag)
                {
                cnt = -1;
                break;
                }
	    }

	if (cnt < 0)
	    {
	    dataError (pSlot);
	    return;
	    }

	/* we've done it */

	transferOkay (pSlot);
	}
    else
	unImplementedType (pSlot);	/* invalid representation type */
    }

/*******************************************************************************
*
* ftpdSockFree - release a socket
*
* This function is used to examine whether or not the socket pointed
* by pSock is active and release it if it is.
*/

LOCAL void ftpdSockFree
    (
    int *pSock
    )
    {

    if (*pSock != FTPD_SOCK_FREE)
        {
        int off = 0;
        struct linger so_linger = {1, 0};

        setsockopt (*pSock, SOL_SOCKET, SO_LINGER, (void *)&so_linger, sizeof (so_linger));
        setsockopt (*pSock, SOL_SOCKET, SO_KEEPALIVE, (char *)&off, sizeof (off));

        /* 
         * Now that we have said we do not want to keep the PCBs around for 60
         * seconds (which is what happens is SO_LINGER is enabled in vxWorks), we
         * need to call shutdown to gracefully terminate the connection without
         * loosing any data.
         */

        shutdown (*pSock, 2);

        if (ftpdDebug & 0x01)
            ftpdDebugMsg ("ftpdLib: (%d) Closing sock %d\n", __LINE__, *pSock, 0, 0);

        (void) close (*pSock);
        *pSock = FTPD_SOCK_FREE;
        }
    }

/*******************************************************************************
*
* ftpdDirListGet - list files in a directory for FTP client
*
* This function performs the client's request to list out all
* the files in a given directory.  The VxWorks implementation of
* stat() does not work on RT-11 filesystem drivers, it is simply not supported.
*
* This command is similar to UNIX ls.  It lists the contents of a directory
* in one of two formats.  If <doLong> is FALSE, only the names of the files
* (or subdirectories) in the specified directory are displayed.  If <doLong>
* is TRUE, then the file name, size, date, and time are displayed.  If
* doing a long listing, any entries that describe subdirectories will also
* be flagged with a "DIR" comment.
*
* The <dirName> parameter specifies the directory to be listed.  If
* <dirName> is omitted or NULL, the current working directory will be
* listed.
*
* Empty directory entries and MS-DOS volume label entries are not
* reported.
*
* INTERNAL
* Borrowed from ls() in usrLib.c.
*
* RETURNS:  OK or ERROR.
*
* SEE ALSO: ls(), stat()
*/

LOCAL STATUS ftpdDirListGet
    (
    int         sd,             /* socket descriptor to write on */
    char        *dirName,       /* name of the directory to be listed */
    BOOL        doLong          /* if TRUE, do long listing */
    )
    {
    FAST STATUS		status;		/* return status */
    FAST DIR		*pDir;		/* ptr to directory descriptor */
    FAST struct dirent	*pDirEnt;	/* ptr to dirent */
    struct stat		fileStat;	/* file status info    (long listing) */
    struct tm		fileDate;	/* file date/time      (long listing) */
    char		*pDirComment;	/* dir comment         (long listing) */
    BOOL		firstFile;	/* first file flag     (long listing) */
    char 		fileName [MAX_FILENAME_LENGTH];
					/* buffer for building file name */
    static char 	*monthNames[] = {"???", "Jan", "Feb", "Mar", "Apr",
				    	 "May", "Jun", "Jul", "Aug", "Sep",
				    	 "Oct", "Nov", "Dec"};

    /* If no directory name specified, use "." */

    if (dirName == NULL)
	dirName = ".";

    /* Open dir */

    if ((pDir = opendir (dirName)) == NULL)
	{
	fdprintf (sd, "Can't open \"%s\".\n", dirName);
	return (ERROR);
	}


    /* List files */

    status = OK;
    firstFile = TRUE;

    do
	{
	errno = OK;

    	pDirEnt = readdir (pDir);

	if (pDirEnt != NULL)
	    {
	    if (doLong)				/* if doing long listing */
		{
		if (firstFile)
		    {
	    	    if (fdprintf (sd, 
			"  size          date       time       name\n")==ERROR)
    			return ( ERROR | closedir (pDir));

	    	    if (fdprintf (sd,
		       "--------       ------     ------    --------\n")==ERROR)
    			return ( ERROR | closedir (pDir));

	    	    firstFile = FALSE;
	    	    }

		/* Construct path/filename for stat */

		(void) pathCat (dirName, pDirEnt->d_name, fileName);


		/* Get and print file status info */

		if (stat (fileName, &fileStat) != OK)
		    {
		    status = ERROR;
		    break;
		    }

		/* Break down file modified time */

		localtime_r (&fileStat.st_mtime, &fileDate);


		if (S_ISDIR (fileStat.st_mode))		/* if a directory */
		    pDirComment = "<DIR>";
		else
		    pDirComment = "";


		if (fdprintf (sd, 
			"%8d    %s-%02d-%04d  %02d:%02d:%02d   %-16s  %s\n",
			fileStat.st_size, 		/* size in bytes */
			monthNames [fileDate.tm_mon + 1],/* month */
			fileDate.tm_mday,		/* day of month */
			fileDate.tm_year + 1900,	/* year */
			fileDate.tm_hour,		/* hour */
			fileDate.tm_min,		/* minute */
			fileDate.tm_sec,		/* second */
			pDirEnt->d_name,		/* name */
			pDirComment) == ERROR)		/* "<DIR>" or "" */
		    return ( ERROR | closedir (pDir));

		}
	    else				/* just listing file names */
		{
	    	if (fdprintf (sd, "%s\n", pDirEnt->d_name) == ERROR)
		    return ( ERROR | closedir (pDir));
		}
	    }
	else					/* readdir returned NULL */
	    {
	    if (errno != OK)			/* if real error, not dir end */
		{
		if (fdprintf (sd, "error reading entry: %x\n", errno) == ERROR)
		    return ( ERROR | closedir (pDir));
		status = ERROR;
		}
	    }

	} while (pDirEnt != NULL);		/* until end of dir */


    fdprintf (sd, "\n");

    /* Close dir */
    status |= closedir (pDir);

    return (status);
    }

/*******************************************************************************
*
* unImplementedType - send FTP invalid representation type error reply
*/

LOCAL void unImplementedType
    (
    FTPD_SESSION_DATA   *pSlot
    )
    {
    ftpdCmdSend (pSlot, pSlot->cmdSock, 550, messages [MSG_TYPE_ERROR],
		 FTPD_REPRESENTATION (pSlot), 0, 0, 0, 0, 0);
    ftpdSockFree (&pSlot->dataSock);
    }

/*******************************************************************************
*
* dataError - send FTP data connection error reply
*
* Send the final error message about connection error and delete the session.
*/

LOCAL void dataError
    (
    FTPD_SESSION_DATA   *pSlot
    )
    {
    ftpdCmdSend (pSlot, pSlot->cmdSock, 426, messages [MSG_DATA_CONN_ERROR], 
                 0, 0, 0, 0, 0, 0);
    ftpdSockFree (&pSlot->dataSock);
    }

/*******************************************************************************
*
* fileError - send FTP file I/O error reply
*
* Send the final error message about file error and delete the session.
*/

LOCAL void fileError
    (
    FTPD_SESSION_DATA  *pSlot
    )
    {
    ftpdCmdSend (pSlot, pSlot->cmdSock, 551, messages [MSG_INPUT_FILE_ERROR], 
                 0, 0, 0, 0, 0, 0);
    ftpdSockFree (&pSlot->dataSock);
    }

/*******************************************************************************
*
* transferOkay - send FTP file transfer completion reply
*/

LOCAL void transferOkay
    (
    FTPD_SESSION_DATA   *pSlot
    )
    {
    ftpdSockFree (&pSlot->dataSock);
    ftpdCmdSend (pSlot, pSlot->cmdSock, 226, messages [MSG_TRANS_COMPLETE], 
                 0, 0, 0, 0, 0, 0);
    }

/*******************************************************************************
*
* ftpdCmdSend - send a FTP command reply
*
* In response to a request, we send a reply containing completion
* status, error status, and other information over a command connection.
*/

LOCAL STATUS ftpdCmdSend
    (
    FTPD_SESSION_DATA  *pSlot,         /* pointer to the session slot */
    int 		controlSock, 	/* control socket for reply */
    int                 code,           /* FTP status code */
    char                *format,        /* printf style format string */
    int                 arg1,
    int                 arg2,
    int                 arg3,
    int                 arg4,
    int                 arg5,
    int                 arg6
    )
    {
    int                 buflen;
    char		buf [BUFSIZE];		/* local buffer */
    FAST char 		*pBuf = &buf [0];	/* pointer to buffer */
    BOOL 		lineContinue =
				(code & FTPD_MULTI_LINE) == FTPD_MULTI_LINE;

    /*
     * If this routine is called before a session is established, the
     * pointer to session-specific data is NULL. Otherwise, exit with
     * an error if an earlier attempt to send a control message failed.
     */

    if ( (pSlot != NULL) && (pSlot->cmdSockError == ERROR))
        return (ERROR);

    code &= ~FTPD_MULTI_LINE;	/* strip multi-line bit from reply code */

    /* embed the code first */

    (void) sprintf (pBuf, "%d%c", code, lineContinue ? '-' : ' ');
    pBuf += strlen (pBuf);

    (void) sprintf (pBuf, format, arg1, arg2, arg3, arg4, arg5, arg6);
    pBuf += strlen (pBuf);

    /* telnet style command terminator */

    (void) sprintf (pBuf, "\r\n");

    /* send it over to our client */

    buflen = strlen (buf);

    if ( write (controlSock, buf, buflen) != buflen )
        {
        if (pSlot != NULL)
            pSlot->cmdSockError = ERROR;
    	ftpdDebugMsg ("sent %s Failed on write\n", (int)buf,0,0,0);
        return (ERROR);    /* Write Error */
        }

    ftpdDebugMsg ("sent %s\n", (int)buf,0,0,0);
    return (OK);			/* Command Sent Successfully */
    }

/*******************************************************************************
*
* ftpdDebugMsg - print out FTP command request and replies for debugging.
*/

LOCAL void ftpdDebugMsg
    (
    char        *format,
    int         arg1,
    int         arg2,
    int         arg3,
    int         arg4
    )
    {
    if (ftpdDebug == TRUE)
	logMsg (format, arg1, arg2, arg3, arg4, 0, 0);
    }

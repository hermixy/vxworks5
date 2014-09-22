/* telnetdLib.c - telnet server library */

/* Copyright 1984 - 2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"


/*
modification history
--------------------
03n,06jun02,elr  Documentation (SPR #78015)
03m,07may02,kbw  man page edits
03l,02may02,elr  Removed telnetdMutexSem in telnetdParserSet()  (SPR #76641)
                 Corrected uninitialized session data pointers 
                 Removed superfluous telnetdInitialized = FALSE
03k,30apr02,elr  Moved password authentication to telnetd context (SPR 30687)
                 Corrected problems with messy session disconnects leaving 
                      many resources still open (SPR 75891) (SPR 72752) (SPR 5059)
                 Corrected security problems
                 Moved the context of authentication/login to telnetd from shell
                 Improved documentation and included an example of a user shell
                 Simplified code as a result of the static creation merge 
                    version 03h
                 Added macro for debug messages
03j,19oct01,rae  merge from truestack ver03j, base 03f (memory leak, doc)
03i,24may01,mil  Bump up telnet task stack size to 8000.
03h,14feb01,spm  merged from version 03f of tor2_0_x branch (base 03e):
                 general overhaul of telnet server (SPR #28675)
03g,08nov99,pul  T2 cumulative patch 2
03f,30jun99,cno  Enable changing the telnet port number (SPR27680)
03e,14mar99,jdi  doc: removed refs to config.h and/or configAll.h (SPR 25663).
03d,05oct98,jmp  doc: cleanup.
03c,30oct96,dgp  doc: change task names for telnetd() per SPR #5901
03b,09aug94,dzb  fixed activeFlag race with cleanupFlag (SPR #2050).
                 made telnetdSocket global (SPR #1941).
		 added logFdFromRlogin (SPR #2212).
03a,02may94,ms   increased stack size for SIMHPPA.
02z,11aug93,jmm  Changed ioctl.h and socket.h to sys/ioctl.h and sys/socket.h
02y,01feb93,jdi  documentation cleanup for 5.1.
02x,18jul92,smb  Changed errno.h to errnoLib.h.
02w,26may92,rrr  the tree shuffle
		  -changed includes to have absolute path from h/
02v,24apr92,rfs  Fixed flaky shell restart upon connection termination.
                 The functions telnetInTask() and telnetdExit() were changed.
                 This is fixing SPR #1427.  Also misc ANSI noise.
02u,13dec91,gae  ANSI cleanup.
02t,14nov91,jpb  moved remCurIdSet to shellLogout (shellLib.c).
02s,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -fixed #else and #endif
		  -changed READ, WRITE and UPDATE to O_RDONLY O_WRONLY and ...
		  -changed VOID to void
		  -changed copyright notice
02r,01aug91,yao  fixed to pass 6 args to excJobAdd() call.
02q,13may91,shl  undo'ed 02o.
02p,30apr91,jdi	 documentation tweaks.
02o,29apr91,shl  added call to restore original machine name, user and
		 group ids (spr 916).
02n,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by dnw.
02m,24mar91,jdi  documentation cleanup.
02l,05oct90,shl  fixed telnetExit() to restore original user and password.
02k,02oct90,hjb  added a call to htons() where needed.
02j,08aug90,dnw  changed declaration of tnInput from void to int.
		 added forward declaration of setMode().
02i,07may90,shl  changed entry point of tTelnetd back to telnetd.
02h,18apr90,shl  added shell security.
		 changed telnetd name to tTelnetd.
02g,20aug89,gae  bumped telnetTaskStackSize from 1500 to 5000 for SPARC.
02f,29sep88,gae  documentation.
02e,06jun88,dnw  changed taskSpawn/taskCreate args.
02d,30may88,dnw  changed to v4 names.
02c,28may88,dnw  changed to use shellOrigStdSet (...) instead of shellSetOrig...
		 changed not to use shellTaskId global variable.
02b,01apr88,gae  made it work with I/O system revision.
02a,27jan88,jcf  made kernel independent.
01g,14dec87,dnw  fixed bug in telnetdIn() that caused system crashes.
01f,19nov87,dnw  changed telnetd to wait for shell to exist before accepting
		   remote connections.
01e,17nov87,ecs  lint.
01d,04nov87,ecs  documentation.
	     &   fixed bug in use of setsockopt().
	    dnw  changed to call shellLogoutInstall() instead of logoutInstall.
01c,24oct87,gae  changed setOrig{In,Out,Err}Fd() to shellSetOrig{In,Out,Err}().
		 made telnetdOut() exit on EOF from master pty fd.
		 made telnetInit() not sweat being called more than once.
		 added shellLock() to telnetd to get exclusive use of shell.
01g,20oct87,gae  added logging device for telnet shell; made telnetInit()
		   create pty device.
01f,05oct87,gae  made telnetdExit() from telnetdIn() - used by logout().
		 removed gratuitous standard I/O ioctl's.
		 made "disconnection" cleaner by having shell do restart.
01e,26jul87,dnw  changed default priority of telnet tasks from 100 to 2.
		 changed task priority and ids to be global variables so
		   they can be accessed from outside this module.
01d,04apr87,dnw  de-linted.
01c,27mar87,dnw  documentation
		 fixed bug causing control sequences from remote to be
		   misinterpreted.
		 added flushing of pty in case anything was left before
		   remote login.
01b,27feb87,dnw  changed to spawn telnet tasks UNBREAKABLE.
01a,20oct86,dnw  written.
*/

/*
DESCRIPTION
The telnet protocol enables users on remote systems to login to VxWorks.

This library implements a telnet server which accepts remote telnet login
requests and transfers input and output data between a command interpreter
and the remote user. The default configuration redirects the input and output
from the VxWorks shell if available. The telnetdParserSet() routine allows
the installation of an alternative command interpreter to handle the remote
input and provide the output responses. If INCLUDE_SHELL is not defined,
installing a command interpreter is required.

The telnetdInit() routine initializes the telnet service when INCLUDE_TELNET
is defined. If INCLUDE_SHELL is also defined, the telnetdStart() routine
automatically starts the server. Client sessions will connect to the shell,
which only supports one client at a time.

VXWORKS AE PROTECTION DOMAINS
Under VxWorks AE, the telnet server runs within the kernel protection domain 
only.  This restriction does not apply under non-AE versions of VxWorks.  

INTERNAL
When connecting remote users to the VxWorks shell, the pseudo-terminal
driver ptyDrv provides data transfer between the telnet server and the shell.

INCLUDE FILES: telnetLib.h

SEE ALSO: rlogLib
*/

#include "vxWorks.h"
#include "sys/types.h"
#include "sys/socket.h"
#include "netinet/in.h"
#include "ioLib.h"
#include "taskLib.h"
#include "telnetLib.h"
#include "stdlib.h"
#include "unistd.h"
#include "errnoLib.h"
#include "string.h"
#include "stdio.h"
#include "fcntl.h"
#include "sockLib.h"
#include "shellLib.h"
#include "remLib.h"
#include "sysLib.h"
#include "tickLib.h"
#include "ptyDrv.h"
#include "logLib.h"
#include "excLib.h"

#define STDIN_BUF_SIZE		512
#define STDOUT_BUF_SIZE		512

/* telnet input states */

#define TS_DATA		0
#define TS_IAC		1
#define TS_CR		2
#define TS_BEGINNEG	3
#define TS_ENDNEG	4
#define TS_WILL		5
#define TS_WONT		6
#define TS_DO		7
#define TS_DONT		8

/* The maximum string length of pty i/o task name */

#define IO_TASK_NAME_MAX_LEN 128 

/* Pseudo TTY buffers (input and output) */
#define PTY_BUFFER_SIZE 1024

/* debugging definition for a log messages */

#define TELNETD_DEBUG(string,  param1, param2, param3, param4, param5, param6)\
   { \
   if (_func_logMsg != NULL) \
      (* _func_logMsg) (string, param1, param2, param3, param4, param5, \
       param6);\
   }

/* global variables */

#ifndef DEBUG
int telnetTaskOptions   = VX_SUPERVISOR_MODE | VX_UNBREAKABLE;
#else
int telnetTaskOptions   = VX_SUPERVISOR_MODE;
#endif

#if    (CPU_FAMILY == SIMHPPA) || (CPU_FAMILY == SIMSPARCSUNOS)
int telnetTaskStackSize = 10000;
#else  /* CPU_FAMILY == SIMHPPA */
int telnetTaskStackSize = 8000;
#endif /* CPU_FAMILY == SIMHPPA */
int telnetTaskPriority  = 55;	/* priority of telnet tasks */

int telnetdCurrentClients = 0;



/* local variables */

/* 
 * telnetdSessionList is a pointer to a linked list of active sessions.
 * This is maintained for cleanup during exit 
 */

LOCAL LIST 		telnetdSessionList;


LOCAL BOOL telnetdTaskFlag = TRUE;    /* Create tasks when client connects? */

/* telnetdTaskList - an array of all sessions (active or inactive) */

LOCAL TELNETD_TASK_DATA * telnetdTaskList; 

LOCAL int telnetdMaxClients = 1; /* Default limit for simultaneous sessions. */

LOCAL int telnetdTaskId;        /* task ID of telnet server task */
LOCAL int telnetdServerSock;
LOCAL SEM_ID telnetdMutexSem;

LOCAL BOOL telnetdInitialized = FALSE; 	/* Server initialized? */
LOCAL BOOL telnetdParserFlag = FALSE; 	/* Parser access task registered? */
LOCAL BOOL telnetdStartFlag = FALSE;    /* Server started? */

LOCAL TBOOL myOpts [256];	/* current option settings - this side */
LOCAL TBOOL remOpts [256];	/* current option settings - other side */

LOCAL BOOL raw;			/* TRUE = raw mode enabled */
LOCAL BOOL echo;		/* TRUE = echo enabled */

LOCAL FUNCPTR telnetdParserControl = NULL; /* Accesses command interpreter. */

LOCAL BOOL remoteInitFlag = FALSE; 	/* No remote users have connected. */
LOCAL char *ptyRemoteName  = "/pty/rmt";    /* terminal for remote user */
LOCAL int masterFd; 		/* master pty for remote users */

/* forward declarations */

void telnetdExit (UINT32 sessionId);

LOCAL void telnetdTaskDelete (int numTasks);

LOCAL int tnInput (int state, int slaveFd, int clientSock, int inputFd, 
                   char *buf, int n);
LOCAL STATUS remDoOpt (int opt, int slaveFd, BOOL enable, int clientSock, 
                       BOOL remFlag);
LOCAL STATUS localDoOpt (int opt, int slaveFd, BOOL enable, int clientSock, 
                         BOOL remFlag);
LOCAL void setMode (int slaveFd, int telnetOption, BOOL enable);
LOCAL STATUS telnetdIoTasksCreate (TELNETD_SESSION_DATA *pSlot);
LOCAL STATUS telnetdSessionPtysCreate (TELNETD_SESSION_DATA *pSlot);
LOCAL TELNETD_SESSION_DATA *telnetdSessionAdd (void);
LOCAL void telnetdSessionDisconnect (TELNETD_SESSION_DATA *pSlot, 
                                     BOOL pSlotdeAllocate);
LOCAL void telnetdSessionDisconnectFromShell (TELNETD_SESSION_DATA *pSlot);
LOCAL void telnetdSessionDisconnectFromRemote (TELNETD_SESSION_DATA *pSlot);


/*******************************************************************************
*
* telnetdInit - initialize the telnet services
*
* This routine initializes the telnet server, which supports remote login
* to VxWorks via the telnet protocol. It is called automatically when the
* configuration macro INCLUDE_TELNET is defined. The telnet server supports
* simultaneous client sessions up to the limit specified by the
* TELNETD_MAX_CLIENTS setting provided in the <numClients> argument. The
* <staticFlag> argument is equal to the TELNETD_TASKFLAG setting. It allows
* the server to create all of the secondary input and output tasks and allocate
* all required resources in advance of any connection. The default value of
* FALSE causes the server to spawn a task pair and create the associated data
* structures after each new connection.
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  This restriction does not apply under non-AE 
* versions of VxWorks.  
*
* RETURNS: OK, or ERROR if initialization fails
*/

STATUS telnetdInit
    (
    int numClients, /* maximum number of simultaneous sessions */
    BOOL staticFlag /* TRUE: create all tasks in advance of any clients */
    )
    {
    int count;
    int result;

    if (telnetdInitialized)
        return (OK);

    if (numClients <= 0)
        return (ERROR);

    /* 
     * If static initialization is selected,  
     *  then we must have a parser control installed 
     */

    if (staticFlag && (telnetdParserControl == NULL))
        {
        TELNETD_DEBUG ("telnetd: A shell has not been installed - can't initialize library. errno:%#x\n", 
                        errno, 0, 0, 0, 0, 0);
        return (ERROR);
        }

    telnetdMutexSem = semMCreate (SEM_Q_PRIORITY | SEM_DELETE_SAFE | 
                                  SEM_INVERSION_SAFE );  
    if (telnetdMutexSem == NULL)
        return (ERROR);

    if (remoteInitFlag == FALSE)
        {
        /* Create pty device for all remote sessions. */

        if (ptyDrv () == ERROR)
            {
            TELNETD_DEBUG ("telnetd: Unable to initialize ptyDrv().  errno:%#x\n", 
                               errno, 0, 0, 0, 0, 0);
            return ERROR;
            }

        remoteInitFlag = TRUE;   /* Disable further device creation. */
        }

    telnetdTaskFlag = staticFlag;   /* Create input/output tasks early? */

    telnetdMaxClients = numClients;

    telnetdTaskList = (TELNETD_TASK_DATA *)calloc (telnetdMaxClients,
                                                  sizeof (TELNETD_TASK_DATA));
    if (telnetdTaskList == NULL)
        {
        semDelete (telnetdMutexSem);
        return (ERROR);
        }

    /* Allocate all of the session data structures and initialize them */

    for (count = 0; count < numClients; count++)
        {
        telnetdTaskList[count].pSession = 
            (TELNETD_SESSION_DATA *)calloc (sizeof (TELNETD_SESSION_DATA), 
                                                1);
        if (telnetdTaskList[count].pSession == NULL)
            {
            telnetdTaskDelete (count);
            return (ERROR);
            }

        /* 
         * Initialize all elements of the structure to sane values.
         * Note that we use  '-1' is used in some cases so that we 
         *  can differentiate from valid i/o file descriptors such as stdin.
         *  Also we need invalid task id's for initialization so we can
         *  differentiate from taskIdSelf() since 0 is implied as our taskId
         */

        telnetdTaskList[count].pSession->socket         = -1;
        telnetdTaskList[count].pSession->inputFd        = -1;            
        telnetdTaskList[count].pSession->outputFd       = -1;          
        telnetdTaskList[count].pSession->slaveFd        = -1;          
        telnetdTaskList[count].pSession->outputTask     = -1;      
        telnetdTaskList[count].pSession->inputTask      = -1;      
        telnetdTaskList[count].pSession->parserControl  = 0; 
        telnetdTaskList[count].pSession->busyFlag       = FALSE;
        telnetdTaskList[count].pSession->loggedIn       = FALSE;

        /* 
         * Static initialization has all resources and tasks created up front.
         * Create and spawn all resources and associate them in the session and task structures.
         */

        if (telnetdTaskFlag)
            {
            /* 
             * Spawn static input/output tasks for each possible connection.
             * In this configuration, the command interpreter currently
             * assigned can never be changed.
             *
             * New entries are identical in construction to the 
             * 'one-at-a-time', dynamic telnet session entries.  
             * 
             * The spawned tasks will pend on a semaphore which signifies 
             * the acceptance of a new socket connection.
             *
             * The session entry will be filled in when the connection 
             * is accepted.
             */


            if ( telnetdSessionPtysCreate (telnetdTaskList[count].pSession) == 
                 ERROR)
                {
                TELNETD_DEBUG ("telnetd: Unable to create all sessions in advance. errno:%#x\n", 
                        errno, 0, 0, 0, 0, 0);
                telnetdTaskDelete (count);
                return ERROR;
                }

            if (telnetdIoTasksCreate (telnetdTaskList[count].pSession) == ERROR)
                {
                TELNETD_DEBUG ("telnetd: error spawning i/o tasks - can't initialize library. errno:%#x\n", 
                               errno, 0, 0, 0, 0, 0);
                telnetdTaskDelete (count);
                return ERROR;
                }

            /*
             * Announce to the installed parser control routine that 
             *  we have a new session installed.   No connection has been 
             *  established to this session at this point.
             */
    
            result = (*telnetdParserControl) (REMOTE_INIT, 
                                              (UINT32)telnetdTaskList[count].pSession,
                                              telnetdTaskList[count].pSession->slaveFd);

            /* Did we initialize the shell? */

            if (result == ERROR) /* No,  must have failed */
                {
                TELNETD_DEBUG ("telnetd: error pre-initializing shell. Note:vxWorks shell does not support static initialization! errno:%#x\n", 
                               errno, 0, 0, 0, 0, 0);
                telnetdTaskDelete (count);
                return ERROR;
                }

            /*
             * File descriptors are available. Save the corresponding
             * parser control routine.
             */

            telnetdTaskList[count].pSession->parserControl = 
                telnetdParserControl;
            }
        }

    telnetdInitialized = TRUE;

    return (OK);
    }

/*******************************************************************************
*
* telnetdParserSet - specify a command interpreter for telnet sessions
*
* This routine provides the ability to handle telnet connections using
* a custom command interpreter or the default VxWorks shell. It is
* called automatically during system startup (when the configuration macro
* INCLUDE_TELNET is defined) to connect clients to the command interpreter
* specified in the TELNETD_PARSER_HOOK parameter. The command interpreter in 
* use when the telnet server start scan never be changed.
*
* The <pParserCtrlRtn> argument provides a routine using the following
* interface:
* .CS
* STATUS parserControlRtn
*     (
*     int telnetdEvent,/@ start or stop a telnet session @/
*     UINT32 sessionId,/@ a unique session identifier @/
*     int ioFd         /@ file descriptor for character i/o @/
*     )
* .CE
*
* The telnet server calls the control routine with a <telnetdEvent>
* parameter of REMOTE_INIT during inititialization.  The telnet server then
* calls the control routine with a <telnetdEvent> parameter of REMOTE_START 
* when a client establishes a new connection.
* The <sessionId> parameter provides a unique identifier for the session.
* 
* In the default configuration, the telnet server calls the control routine
* with a <telnetdEvent> parameter of REMOTE_STOP when a session ends. 
*
* The telnet server does not call the control routine when a session ends
* if it is configured to spawn all tasks and allocate all resources in
* advance of any connections. The associated file descriptors will be reused
* by later clients and cannot be released. In that case, the REMOTE_STOP
* operation only occurs to allow the command interpreter to close those
* files when the server encounters a fatal error.
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  In addition, all arguments to this function can  
* reference only that data which is valid in the kernel protection domain. 
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS: OK if parser control routine installed, or ERROR otherwise.
*
* INTERNAL: Can be called before or after telnetdInit() and telnetdStart()
*/

STATUS telnetdParserSet
    (
    FUNCPTR     pParserCtrlRtn 	/* provides parser's file descriptors */
    )
    {

    /* We must have a valid parser */

    if (pParserCtrlRtn == NULL)
        return (ERROR);

    /* We can not change parsers */
 
    if (telnetdParserControl != NULL)
        return (ERROR);

    /* Store the provided control routine.  */

    telnetdParserControl = pParserCtrlRtn;

    /* Allow client connections. */

    telnetdParserFlag = TRUE; 	

    return (OK);
    }

/*******************************************************************************
*
* telnetdIoTasksCreate  - Create tasks to transferring i/o between socket and fd
* 
* Two tasks are created: An input task and an output task.  The name is based on
*    the pSlot argument for uniqueness.
*
* NOMANUAL
* 
* RETURNS: OK if parser control routine installed, or ERROR otherwise.
*/

LOCAL STATUS telnetdIoTasksCreate
    (
    TELNETD_SESSION_DATA *pSlot
    )
    {
    char sessionTaskName[IO_TASK_NAME_MAX_LEN];
    char sessionInTaskName[IO_TASK_NAME_MAX_LEN];
    char sessionOutTaskName[IO_TASK_NAME_MAX_LEN];
 
    int result;

    /*
     * Spawn the input and output tasks which transfer data between
     * the socket and the i/o file descriptor. 
     *
     * If created in advance (static) the task pend on a semaphore
     */

    sprintf (sessionTaskName, "_%x", (unsigned int)pSlot);
    sprintf (sessionInTaskName, "tTelnetIn%s", sessionTaskName);
    sprintf (sessionOutTaskName,"tTelnetOut%s", sessionTaskName);
  
    if (telnetdTaskFlag) 
        {
        pSlot->startOutput = semBCreate (SEM_Q_FIFO, SEM_EMPTY);
        pSlot->startInput = semBCreate (SEM_Q_FIFO, SEM_EMPTY);
        if ((pSlot->startInput == NULL)  || (pSlot->startInput == NULL))
            {
            TELNETD_DEBUG ("telnetd: Unable to create semaphore. errno:%#x\n", 
                           errno, 0, 0, 0, 0, 0);
            result = ERROR;
            return ERROR;
            }
        }

    pSlot->outputTask = taskSpawn (sessionOutTaskName, 
                                   telnetTaskPriority,
                                   telnetTaskOptions, 
                                   telnetTaskStackSize,
                                   (FUNCPTR)telnetOutTask, 
                                   (int)pSlot,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0);

    if (pSlot->outputTask == ERROR)
        {
        TELNETD_DEBUG ("telnetd: Unable to create task. errno:%#x\n", 
                       errno, 0, 0, 0, 0, 0);
        result = ERROR;
        return ERROR;
        }

    pSlot->inputTask = taskSpawn (sessionInTaskName, 
                                  telnetTaskPriority,
                                  telnetTaskOptions, 
                                  telnetTaskStackSize,
                                  (FUNCPTR)telnetInTask, 
                                  (int)pSlot,
                                  0, 0, 0, 0, 0, 0, 0, 0, 0);

    if (pSlot->inputTask == ERROR)
        {
        TELNETD_DEBUG ("telnetd: Unable to create task. errno:%#x\n", 
                       errno, 0, 0, 0, 0, 0);
        taskDelete (pSlot->outputTask);
        
        result = ERROR;
        return ERROR;
        }

    return OK;
    }

/*******************************************************************************
*
* telnetdSessionPtysCreate  - Create a pty pair and open them 
* 
* Two file descriptors are created: An input and an output fd.  The name is 
*    based on the pSlot argument for uniqueness.  The file descriptors are 
*    stored in the session structure.
*
* NOMANUAL
* 
* RETURNS: OK if successfull, or ERROR otherwise.
*/

LOCAL STATUS telnetdSessionPtysCreate 
    (
    TELNETD_SESSION_DATA *pSlot
    )
    {

    char sessionPtyRemoteName[PTY_DEVICE_NAME_MAX_LEN];
    char sessionPtyRemoteNameM[PTY_DEVICE_NAME_MAX_LEN];
    char sessionPtyRemoteNameS[PTY_DEVICE_NAME_MAX_LEN];

    /* Create unique names for the pty device */

    sprintf (sessionPtyRemoteName, "%s_%x.", ptyRemoteName, (int)pSlot);
    sprintf (sessionPtyRemoteNameM, "%sM", sessionPtyRemoteName);
    sprintf (sessionPtyRemoteNameS, "%sS", sessionPtyRemoteName);

    /* pseudo tty device creation */

    if (ptyDevCreate (sessionPtyRemoteName, 
                      PTY_BUFFER_SIZE, 
                      PTY_BUFFER_SIZE) == ERROR)
        {
        return ERROR;
        }

    /* Master-side open of pseudo tty */

    strcpy (pSlot->ptyRemoteName, sessionPtyRemoteName);

    if ((masterFd = open (sessionPtyRemoteNameM, O_RDWR, 0)) == ERROR)
        {
        return ERROR;
        }
    else
        {
        pSlot->inputFd = masterFd;
        pSlot->outputFd = masterFd;
        }

    /* Slave-side open of pseudo tty */

    if ((pSlot->slaveFd = open (sessionPtyRemoteNameS, O_RDWR, 0)) == ERROR)
        {
        return ERROR;
        }

    /* setup the slave device to act like a terminal */

    (void) ioctl (pSlot->slaveFd, FIOOPTIONS, OPT_TERMINAL);

    return OK;
    }

/*******************************************************************************
*
* telnetdStart - initialize the telnet services
*
* Following the telnet server initialization, this routine creates a socket
* for accepting remote connections and spawns the primary telnet server task.
* It executes automatically during system startup when the INCLUDE_TELNET
* configuration macro is defined since a parser control routine is available.
* The server will not accept connections otherwise.
*
* By default, the server will spawn a pair of secondary input and output
* tasks after each client connection. Changing the TELNETD_TASKFLAG setting
* to TRUE causes this routine to create all of those tasks in advance of
* any connection. In that case, it calls the current parser control routine
* repeatedly to obtain file descriptors for each possible client based on
* the <numClients> argument to the initialization routine. The server will
* not start if the parser control routine returns ERROR.
*
* The TELNETD_PORT constant provides the <port> argument, which assigns the
* port where the server accepts connections. The default value is the standard
* setting of 23. 
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  This restriction does not apply under non-AE 
* versions of VxWorks.  
*
* RETURNS: OK, or ERROR if startup fails
*/

STATUS telnetdStart
    (
    int port 	/* target port for accepting connections */
    )
    {
    struct sockaddr_in serverAddr;

    if (!telnetdInitialized)
        {
        TELNETD_DEBUG ("telnetd: Must be initialized with telnetdInit() first.\n", 
                       0, 0, 0, 0, 0, 0);
        return (ERROR);
        }

    if (telnetdStartFlag)
        return (OK);

    /* 
     * At this point (for both static and dynamic task initialization) we 
     * are ready to create the server socket.
     */

    telnetdServerSock = socket (AF_INET, SOCK_STREAM, 0);
    if (telnetdServerSock < 0)
        return (ERROR);

    bzero ((char *)&serverAddr, sizeof (serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port   = htons (port);

    if (bind (telnetdServerSock, (struct sockaddr *) &serverAddr,
              sizeof (serverAddr)) < 0)
        {
        close (telnetdServerSock);
        if (telnetdTaskFlag)
            telnetdTaskDelete (telnetdMaxClients);
        return (ERROR);
        }

    if (listen (telnetdServerSock, 5) < 0)
        {
        close (telnetdServerSock);
        if (telnetdTaskFlag)
            telnetdTaskDelete (telnetdMaxClients);
        return (ERROR);
        }
      
    /* Create a telnet server task to receive connection requests. */

    telnetdTaskId = taskSpawn ("tTelnetd", 
                               telnetTaskPriority,
                               telnetTaskOptions, 
                               telnetTaskStackSize,
                               (FUNCPTR)telnetd, 
                               0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    if (telnetdTaskId == ERROR)
        {
        close (telnetdServerSock);
        if (telnetdTaskFlag)
            telnetdTaskDelete (telnetdMaxClients);
        return (ERROR);
        }

    return (OK);
    }

/*******************************************************************************
*
* telnetdTaskDelete - remove task data from task list
*
* This routine releases the system objects which monitor the secondary input
* and output tasks for telnet clients. The server uses this routine to clean
* up the (partial) task list if an error occurs during multiple task enabled 
* startup.
*
* RETURNS: N/A
*
* NOMANUAL
*/

LOCAL void telnetdTaskDelete
    (
    int 	numTasks 	/* number of entries to remove */
    )
    {
    int 	count;

    BOOL telnetdTaskFlagSave; 

    telnetdTaskFlagSave = telnetdTaskFlag; /* Save original state */

    /* 
     * Make sure the flag is set to FALSE so we delete objects instead of
     * restart objects with  telnetdSessionDisconnectFromShell ()
     */

    telnetdTaskFlag = FALSE;

    for (count = 0; count < numTasks; count++)
        {
        if (telnetdParserControl != NULL)
           (*telnetdParserControl) (REMOTE_STOP, 
                                    telnetdTaskList [count].pSession,
                                    0);  
        if (telnetdTaskList [count].pSession != NULL)
            {
            telnetdSessionDisconnect (telnetdTaskList [count].pSession, TRUE);
            free (telnetdTaskList [count].pSession);
            }
        }
 
    free (telnetdTaskList);

    semDelete (telnetdMutexSem);

    telnetdTaskFlag = telnetdTaskFlagSave; /* Restore original state */
    return;
    }

/*******************************************************************************
*
* telnetdExit - close an active telnet session
*
* This routine supports the session exit command for a command interpreter
* (such as logout() for the VxWorks shell). Depending on the TELNETD_TASKFLAG
* setting, it causes the associated input and output tasks to restart or exit.
* The <sessionId> parameter must match a value provided to the command
* interpreter with the REMOTE_START option.
*
* INTERNAL
* This routine is used for termination via the shell.  The inTask may also 
*   terminate the connection via telnetdSessionDisconnectFromShell()
*
* RETURNS: N/A.
*
* ERRNO: N/A
*/

void telnetdExit
    (
    UINT32 sessionId 	/* identifies the session to be deleted */
    )
    {
    telnetdSessionDisconnectFromShell ((TELNETD_SESSION_DATA *)sessionId);
    return;
    }

/*******************************************************************************
*
* telnetdSessionAdd - add a new entry to the telnetd session slot list
*
* Each of the telnet clients is associated with an entry in the server's
* session list which records the session-specific context for each
* connection, including the file descriptors that access the command
* interpreter. This routine creates and initializes a new entry in the
* session list, unless the needed memory is not available or the upper
* limit for simultaneous connections is reached.
*
* RETURNS: A pointer to the session list entry, or NULL if none available.
*
* ERRNO: N/A
*
* NOMANUAL
*
*/

LOCAL TELNETD_SESSION_DATA *telnetdSessionAdd (void)
    {
    TELNETD_SESSION_DATA * 	pSlot;
    int count = 0;

    semTake (telnetdMutexSem, WAIT_FOREVER); 

    if (telnetdCurrentClients == telnetdMaxClients)
        {
        semGive (telnetdMutexSem); 
        return (NULL);
        }

    /* Find an idle pair of input/output tasks, if needed. */

    if (telnetdTaskFlag)     /* Tasks created during server startup? */
        {
        for (count = 0; count < telnetdMaxClients; count++)
            if (!telnetdTaskList [count].pSession->busyFlag)
                break;
        }
   
    /* Are there no more sessions available ? */ 

    if (telnetdTaskFlag && (count == telnetdMaxClients))
        {
        semGive (telnetdMutexSem); 
        return ((TELNETD_SESSION_DATA *) NULL);
        }
  

    /* get memory for the new session entry */

    if (telnetdTaskFlag)     
       pSlot = telnetdTaskList [count].pSession;
    else 
       {
       pSlot = (TELNETD_SESSION_DATA *) calloc (sizeof (TELNETD_SESSION_DATA), 
               1);
       if (pSlot == NULL)
           {
           semGive (telnetdMutexSem); 
           return (NULL);
           }
       telnetdTaskList [count].pSession = pSlot;
       }

    pSlot->busyFlag = TRUE;
    pSlot->loggedIn = FALSE;
    telnetdCurrentClients++;

    if (!telnetdTaskFlag)
        {

        /* 
         * Dynamic session creation needs all elements of the structure 
         * initialized to sane values 
         */

        pSlot->socket         = -1;
        pSlot->inputFd        = -1;            
        pSlot->outputFd       = -1;          
        pSlot->slaveFd        = -1;          
        pSlot->outputTask     = -1;      
        pSlot->inputTask      = -1;      
        pSlot->parserControl  = 0; 
        }

    /* Add new entry to the list of active sessions. */

    lstAdd (&telnetdSessionList, &pSlot->node);

    semGive (telnetdMutexSem); 
    return (pSlot);
    }

/*******************************************************************************
*
* telnetdSessionDisconnect - Shut down a session
*
* This routine removes a connection and all associated resources for it.
*
* This may be called because of login failure or end of a shell session
*
* NOMANUAL
*/

LOCAL void telnetdSessionDisconnect 
     (
     TELNETD_SESSION_DATA *pSlot, 
     BOOL pSlotDelete /* For DYNAMIC mode,  should pSlot be free'd when done? */
     )
     {

     if (pSlot == NULL)
         return;

     semTake (telnetdMutexSem, WAIT_FOREVER); 

     /* 
      * Make sure the task is still valid.  It is possible that 
      * the connection was killed during login (telnetInTask),
      *                    failed login (telnetd)
      *                    terminated by shell (shell task)
      */
      
     /* For STATIC sessions,  we need to clean/sanitize all of it's resources */

     if (telnetdTaskFlag)  
        {

        /* Halt the i/o tasks */

        if (pSlot->outputTask)
            taskSuspend (pSlot->outputTask);
        if (pSlot->inputTask)
            taskSuspend (pSlot->inputTask);

        /* Make sure the semaphores are empty */

        if (pSlot->startInput)
            semTake (pSlot->startInput, NO_WAIT);
        if (pSlot->startOutput)
            semTake (pSlot->startOutput, NO_WAIT);

        /* Restart the I/O tasks, they will pend on the empty semaphores */

        if (pSlot->outputTask)
            taskRestart (pSlot->outputTask);
        if (pSlot->inputTask)
            taskRestart (pSlot->inputTask);

        /* Clear out the i/o descriptors */

        if ((pSlot->outputFd) > STD_ERR)
            ioctl (pSlot->outputFd, FIOFLUSH, 0);
        if ((pSlot->slaveFd) > STD_ERR)
            ioctl (pSlot->slaveFd, FIOFLUSH, 0);

        /* Close the socket connection to the client */

        if ((pSlot->socket) > STD_ERR)
             close (pSlot->socket);
  
        /* 
         * Re-Initialize some elements of the structure to sane values 
         * We will not re-initialize the taskId's or the i/o file descriptors
         * because these resources are just reset and not removed.
         */

        pSlot->socket        = -1;
        pSlot->parserControl = telnetdParserControl;
        pSlot->loggedIn      = FALSE;
        pSlot->busyFlag      = FALSE;

        --telnetdCurrentClients;
        lstDelete (&telnetdSessionList, &pSlot->node);

        }
     else
        {

        /* For DYNAMIC sessions,  we need to free all of it's resources */

        /* Remove the i/o tasks */

        if (pSlot->outputTask)
            taskDelete (pSlot->outputTask);
        if (pSlot->inputTask)
            taskDelete (pSlot->inputTask);

        /* Delete the semaphores */

        if (pSlot->startInput)
            semDelete (pSlot->startInput);
        if (pSlot->startOutput)
            semDelete (pSlot->startOutput);

        /* Close the i/o descriptors */

        if ((pSlot->outputFd) > STD_ERR)
            close (pSlot->outputFd);
        if ((pSlot->slaveFd) > STD_ERR)
            close (pSlot->slaveFd);

        /* Remove the pseudo terminal for the session. */

        (void) ptyDevRemove (pSlot->ptyRemoteName);

        /* Close the socket connection to the client */

        if ((pSlot->socket) > STD_ERR)
             close (pSlot->socket);
  
        /* Re-Initialize all elements of the structure to sane values */

        pSlot->socket         = -1;
        pSlot->inputFd        = -1;            
        pSlot->outputFd       = -1;          
        pSlot->slaveFd        = -1;          
        pSlot->outputTask     = 0;      
        pSlot->inputTask      = 0;      
        pSlot->parserControl  = 0; 
        pSlot->busyFlag       = FALSE;

        --telnetdCurrentClients;
        lstDelete (&telnetdSessionList, &pSlot->node);

        /* 
         * If we are not logged in, then the connection must have been shut down
         * during login.   When the login routine terminates, the pSlot will be
         * free'd. This will be determined by the telnetd if parserControl == 0
         */

        if ((pSlot->loggedIn == TRUE) && 
            (pSlotDelete == TRUE))
            free (pSlot);
        else
            pSlot->loggedIn = FALSE; 
        }

     semGive (telnetdMutexSem); 

     }

/*******************************************************************************
*
* telnetdSessionDisconnectFromShellJob - terminate a session from the shell
*
* This is called from the shell context during a excTask() and is invoked 
* by calling telnetdSessionDisconnectFromShell.
*
* NOTE: The inTask may also terminate the connection via 
*       telnetdSessionDisconnectFromRemote()
*
* RETURNS N/A
*
* NOMANUAL
*/
LOCAL void telnetdSessionDisconnectFromShellJob (TELNETD_SESSION_DATA *pSlot)
    {

    /* Shut down the connection */

    telnetdSessionDisconnect (pSlot, FALSE);

    if (*telnetdParserControl) 
        (*telnetdParserControl) (REMOTE_STOP, pSlot, 0);  

    if (!telnetdTaskFlag)
        free (pSlot);
    }

/*******************************************************************************
*
* telnetdSessionDisconnectFromShell - terminate a session from the shell
*
* This is called from the shell context during a logout() call 
* NOTE: The inTask may also terminate the connection via 
*       telnetdSessionDisconnectFromRemote()
*
* RETURNS N/A
*
* NOMANUAL
*/
LOCAL void telnetdSessionDisconnectFromShell (TELNETD_SESSION_DATA *pSlot)
     {
     excJobAdd (telnetdSessionDisconnectFromShellJob, 
                (int) pSlot, 
                0, 0, 0, 0, 0);
     taskDelay (sysClkRateGet());
     }

/*******************************************************************************
*
* telnetdSessionDisconnectFromRemote - terminate a session from the shell
*
* This is called from the telnetInTask context if the user terminates the 
* connection.
*
* RETURNS N/A
*
* NOMANUAL
*/
LOCAL void telnetdSessionDisconnectFromRemote (TELNETD_SESSION_DATA *pSlot)
    {

    /* Make the shell restart locally */

    (*telnetdParserControl) (REMOTE_STOP, pSlot, 0); 

    /* The shell will terminate the connection for us */

    if (pSlot->loggedIn == TRUE)
        {

        /* Shut down the connection */

        telnetdSessionDisconnect (pSlot, TRUE);
        }

    taskDelay (sysClkRateGet()); 
    }

/*******************************************************************************
*
* telnetd - primary telnet server task
*
* This routine monitors the telnet port for connection requests from clients.
* It is the entry point for the primary telnet task created during the
* library initialization.
*
* The server will only accept telnet connection requests when a task is
* available to handle the input and provide output to the remote user.
* The default configuration uses the shell for this purpose by redirecting
* the `stdin', `stdout', and `stderr' file descriptors away from the console.
* When the remote user disconnects, those values are restored to their
* previous settings and the shell is restarted.
*
* RETURNS: N/A
*
* NOMANUAL
*/
void telnetd (void)
    {
    struct sockaddr_in clientAddr;
    int clientAddrLen;
    int newSock;
    int optval;
    TELNETD_SESSION_DATA *pSlot;
    BOOL startFlag;  /* Command interpreter started successfully? */

    STATUS result;

    FOREVER
        {
        clientAddrLen = sizeof (clientAddr);
        pSlot = NULL;
        result = OK;
        startFlag = FALSE;

        newSock = accept (telnetdServerSock,
                          (struct sockaddr *) &clientAddr, &clientAddrLen);

        if (newSock == ERROR)
            {
            break;     /* Exit if unable to accept connection. */
            }

        /* 
         * Get (static) or create (dynamic) a new session entry for this 
         * connection */

        pSlot = telnetdSessionAdd ();

        /* Check if the maximum number of connections has been reached. */

        if (pSlot == NULL)  
            {
            fdprintf (newSock, "\r\nSorry, session limit reached.\r\n");

            /* Prevent denial of service attack by waiting 5 seconds */

            taskDelay (sysClkRateGet () * 5);  
            close (newSock);
            result = ERROR;
            continue;
            }

        pSlot->socket = newSock;

        /* If we haven't created the pseudo tty device, so so now */

        if (remoteInitFlag == FALSE)
            {

            /* Create pty device for all remote sessions. */

            if (ptyDrv () == ERROR)
                {
                fdprintf (newSock, "\n\ntelnetd: Unable to initialize ptyDrv().  errno:%#x\n", 
                          errno, 0, 0, 0, 0, 0);

                /* Prevent denial of service attack by waiting 5 seconds */

                taskDelay (sysClkRateGet () * 5);  
                close (newSock);
                continue;
                result = ERROR;
                }
            remoteInitFlag = TRUE;   /* Disable further device creation. */
            }

        /* Check if we are in the dynamic mode */

        if (!telnetdTaskFlag)
            {

            /*
             * Create the pseudo terminal for a session. The remote machine
             * will write to the master side and read from the slave side.
             * 
             */

            result = telnetdSessionPtysCreate (pSlot);

            if (result == OK)
                result = telnetdIoTasksCreate (pSlot);

            if (result == ERROR)
                {
                fdprintf (newSock, 
                          "\r\ntelnetd: Sorry, session limit reached. Unable to spawn tasks. errno %#x\r\n", 
                          errno);

                /* Prevent denial of service attack by waiting 5 seconds */

                taskDelay (sysClkRateGet () * 5); 
                telnetdSessionDisconnect (pSlot, TRUE);
                result = ERROR;
                continue;
                }
            }

        /* setup the slave device to act like a terminal */

        (void) ioctl (pSlot->slaveFd, FIOOPTIONS, OPT_TERMINAL);

        /* Check if a parser has been installed */

        if (!telnetdParserFlag)
            {
            fdprintf (newSock, 
                      "\r\ntelnetd: Sorry, a shell has not been installed.\r\n"
                     );

            /* Prevent denial of service attack by waiting 5 seconds */

            taskDelay (sysClkRateGet () * 5);  
            telnetdSessionDisconnect (pSlot, TRUE);
            continue;
            }

        /*
         * Spawn (or wake up) the input task which transfers data from
         * the client socket and the output task which transfers data to
         * the socket. 
         */

        if (telnetdTaskFlag)
            {
            semGive (pSlot->startInput);
            semGive (pSlot->startOutput);
            }

        /* turn on KEEPALIVE so if the client crashes, we'll know about it */

        optval = 1;
        setsockopt (newSock, SOL_SOCKET, SO_KEEPALIVE,
                    (char *) &optval, sizeof (optval));

        /* initialize modes and options and offer to do remote echo */

        raw = FALSE;
        echo = TRUE;
        bzero ((char *)myOpts, sizeof (myOpts));
        bzero ((char *)remOpts, sizeof (remOpts));

        (void)localDoOpt (pSlot->slaveFd, TELOPT_ECHO, TRUE, newSock, FALSE);

        /*
         * File descriptors are available. Save the corresponding
         * parser control routine.
         */

        pSlot->parserControl = telnetdParserControl;

        /*
         * Tell the parser control function to activate the shell 
         *  for this session.
         */

        result = (*telnetdParserControl) (REMOTE_START, 
                                          (UINT32)pSlot,
                                          pSlot->slaveFd);

        /* 
         * If parserControl == 0, then the connection was terminated
         *  during login.  The inTask has already cleaned up all resources
         *  for us except the memory for pSlot.
         */

        if (pSlot->parserControl == 0)
            {
            if (!telnetdTaskFlag)
                free (pSlot);
            continue;
            }

        /* 
         * Was the REMOTE_START of the shell successful? 
         * Did we fail the login or did the remote user disconnect?
         */ 

        if (result == ERROR) /* No,  must have failed to login */
            {
            telnetdSessionDisconnect (pSlot, TRUE);
            continue;
            }
        else  
            {

            /* 
             * We must keep track of a successfull login with 
             * 'loggedIn' element since failure to login means that the 
             * shell was not spawned.
             *
             * Future session cleanup's will know that the login has been 
             * a success.  Therefore, the parser control will need a 
             * REMOTE_STOP if the connection is ever broken.
             */

            pSlot->loggedIn = TRUE; 
            }
        }
    }

/*******************************************************************************
*
* telnetOutTask - relay output to remote user
*
* This routine transfers data from the registered input/output task to
* the remote user. It is the entry point for the output task which is
* deleted when the client disconnects.
*
* INTERNAL
* 
* This routine blocks within the read() call until the command interpreter
* sends a response. When the telnet server creates all input/output tasks
* during startup (i.e. telnetdTaskFlag is TRUE), this routine cannot send
* any data to the socket until the primary server task (telnetd) awakens 
* the input routine.
*
* NOMANUAL
* but not LOCAL for i()

*/

void telnetOutTask
    (
    TELNETD_SESSION_DATA *pSlot  /* pointer to the connection information */
    )
    {
    FAST int n;
    char buf [STDOUT_BUF_SIZE];

    int sock;     /* Socket for individual telnet session */
    int outputFd; /* Output from command interpreter */

    /*
     * Wait for a connection if the server creates this task in advance. 
     */

    if (telnetdTaskFlag)  /* static task creation  */
        semTake (pSlot->startOutput, WAIT_FOREVER);

    sock = pSlot->socket;
    outputFd = pSlot->outputFd;

    /*
     * In the default configuration, the following read loop exits after
     * the connection between the client and server ends because closing
     * the socket to the remote client causes the input task to close the
     * output file descriptor.
     *
     * When the server creates the input and output tasks in advance, that
     * operation does not occur. To prevent writing to a non-existent
     * socket, the input task restarts the output task after using the
     * FIOFLUSH ioctl to prevent stale data from reaching the next session.
     */

    while ((n = read (outputFd, buf, sizeof (buf))) > 0)
        {

        /* XXX should scan for IAC and double 'em to escape 'em */

        write (sock, buf, n);
        }

    return; /* This point is never reached */
    }

/*******************************************************************************
*
* telnetInTask - relay input from remote user
*
* This routine transfers data from the remote user to the registered
* input/output task. It is deleted when the client disconnects. The
* <slot> argument has two possible meanings. In the default setup,
* it provides access to the session data which includes two file
* descriptors: one for a socket (to the remote client) and another
* provided by the command interpreter for the connection. That
* information is not available when the server creates the input/output
* tasks in advance. 
*
* RETURNS: N/A.
*
* NOMANUAL
* but not LOCAL for i()
*/

void telnetInTask
    (
    TELNETD_SESSION_DATA * pSlot
    )
    {

    int n;
    int state = TS_DATA;
    char buf [STDIN_BUF_SIZE];

    int sock;    /* Socket for individual telnet session */
    int inputFd; /* Input to command interpreter */
    int slaveFd; /* Command interpreter stdin */

    FOREVER
        {

        /* Wait for a connection if the server creates this task in advance. */

        if (telnetdTaskFlag)  /* static creation */
            semTake (pSlot->startInput, WAIT_FOREVER);

        /*
         * Transfer data from the socket to the command interpreter, after
         * filtering out the telnet commands and options.
         */

        sock = pSlot->socket;
        inputFd = pSlot->inputFd;
        slaveFd = pSlot->slaveFd;

        while ((n = read (sock, buf, sizeof (buf))) > 0)
            state = tnInput (state, slaveFd, sock, inputFd, buf, n);

        /*
         * Terminate the session.  This is done as a seperate job because 
         *  the function telnetdSessionDisconnectFromRemote() deletes 
         *  the inTask before completion and that is our context (in this case)!
         */

        excJobAdd (telnetdSessionDisconnectFromRemote, 
                   (int)pSlot, 
                   0, 0, 0, 0, 0);

        taskSuspend (0); /* Wait to be deleted by telnetdSessionDisconnect */
        }

    return;
    }

/*******************************************************************************
*
* tnInput - process input from remote user
*
* This routine transfers input data from a telnet client's <clientSock>
* socket to the command interpreter through the <inputFd> file descriptor.
* The <state> parameter triggers interpretation of the input as raw data or
* as part of a command sequence or telnet option.
*
* RETURNS: state value for next data bytes
*
* NOMANUAL
*/

LOCAL int tnInput
    (
    FAST int state,         /* state of telnet session's input handler */
    FAST int slaveFd,       /* local fd, some options adjust it via ioctl */
    FAST int clientSock,    /* socket connected to telnet client */
    FAST int inputFd,       /* input to command interpreter */
    FAST char *buf,         /* buffer containing client data */
    FAST int n              /* amount of client data in buffer */
    )
    {
    char cc;
    int ci;

    while (--n >= 0)
    {
        cc = *buf++;               /* get next character */
        ci = (unsigned char) cc;   /* convert to int since many values
                                    * are negative characters */
    switch (state)
        {
        case TS_CR: 	/* doing crmod; ignore add'l linefeed */
		state = TS_DATA;
		if ((cc != EOS) && (cc != '\n'))
		    write (inputFd, &cc, 1);	/* forward char */
		break;

	case TS_DATA:	/* just pass data */
		if (ci == IAC)
		    state = TS_IAC;
		else
		    {
		    write (inputFd, &cc, 1);	/* forward char */
		    if (!myOpts [TELOPT_BINARY] && (cc == '\r'))
			state = TS_CR;
		    }
		break;

	    case TS_IAC:

		switch (ci)
		    {
		    case BREAK:		/* interrupt from remote */
		    case IP:
			/* XXX interrupt (); */
			state = TS_DATA;
			break;

		    case AYT:		/* Are You There? */
			{
			static char aytAnswer [] = "\r\n[yes]\r\n";

			write (clientSock, aytAnswer, sizeof (aytAnswer) - 1);
			state = TS_DATA;
			break;
			}

		    case EC:		/* erase character */
			write (inputFd, "\b", 1);
			state = TS_DATA;
			break;

		    case EL:		/* erase line */
			write (inputFd, "\025", 1);
			state = TS_DATA;
			break;

		    case DM:		/* data mark */
			state = TS_DATA;
			break;

		    case SB:		/* sub-option negotiation begin */
			state = TS_BEGINNEG;
			break;

		    case WILL: state = TS_WILL;	break;	/* remote will do opt */
		    case WONT: state = TS_WONT;	break;	/* remote wont do opt */
		    case DO:   state = TS_DO;	break;	/* req we do opt */
		    case DONT: state = TS_DONT;	break;	/* req we dont do opt */

		    case IAC:
			write (inputFd, &cc, 1);	/* forward char */
			state = TS_DATA;
			break;
		    }
		break;

	    case TS_BEGINNEG:
		/* ignore sub-option stuff for now */
		if (ci == IAC)
		    state = TS_ENDNEG;
		break;

	    case TS_ENDNEG:
		state = (ci == SE) ? TS_DATA : TS_BEGINNEG;
		break;

	    case TS_WILL:		/* remote side said it will do opt */
		(void)remDoOpt (slaveFd, ci, TRUE, clientSock, TRUE);
		state = TS_DATA;
		break;

	    case TS_WONT:		/* remote side said it wont do opt */
		(void)remDoOpt (slaveFd, ci, FALSE, clientSock, TRUE);
		state = TS_DATA;
		break;

	    case TS_DO:			/* remote wants us to do opt */
		(void)localDoOpt (slaveFd, ci, TRUE, clientSock, TRUE);
		state = TS_DATA;
		break;

	    case TS_DONT:		/* remote wants us to not do opt */
		(void)localDoOpt (slaveFd, ci, FALSE, clientSock, TRUE);
		state = TS_DATA;
		break;

	    default:
		TELNETD_DEBUG ("telnetd: invalid state = %d\n", state, 
                               1, 2, 3, 4, 5);
		break;
	    }
	}

    return (state);
    }

/*******************************************************************************
*
* remDoOpt - request/acknowledge remote enable/disable of option
*
* This routine will try to accept the remote's enable or disable,
* as specified by "will", of the remote's support for the specified option.
* If the request is to disable the option, the option will always be disabled.
* If the request is to enable the option, the option will be enabled, IF we
* are capable of supporting it.  The remote is notified to DO/DONT support
* the option.
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS remDoOpt
    (
    FAST int slaveFd,/* slave fd */
    FAST int opt,    /* option to be enabled/disabled */
    BOOL enable,     /* TRUE = enable option, FALSE = disable */
    int clientSock,  /* socket connection to telnet client */
    BOOL remFlag     /* TRUE = request is from remote */
    )
    {
    BOOL doOpt = enable;

    if (remOpts [opt] == enable)
	return (OK);

    switch (opt)
	{
	case TELOPT_BINARY:
	case TELOPT_ECHO:
	    setMode (slaveFd, opt, enable);
	    break;

	case TELOPT_SGA:
	    break;

	default:
	    doOpt = FALSE;
	    break;
	}

    if ((remOpts [opt] != doOpt) || remFlag)
	{
	char msg[3];

	msg[0] = IAC;
	msg[1] = doOpt ? DO : DONT;
	msg[2] = opt;

	write (clientSock, msg, 3);

	remOpts [opt] = doOpt;
	}

    return ((enable == doOpt) ? OK : ERROR);
    }

/*******************************************************************************
*
* localDoOpt - offer/acknowledge local support of option
*
* This routine will try to enable or disable local support for the specified
* option.  If local support of the option is already in the desired mode, no
* action is taken.  If the request is to disable the option, the option will
* always be disabled.  If the request is to enable the option, the option
* will be enabled, IF we are capable of supporting it.  The remote is
* notified that we WILL/WONT support the option.
*
* NOMANUAL
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS localDoOpt
    (
    FAST int slaveFd,     /* slave fd */
    FAST int opt,    /* option to be enabled/disabled */
    BOOL enable,        /* TRUE = enable option, FALSE = disable */
    int clientSock,     /* socket connection to telnet client */
    BOOL remFlag     /* TRUE = request is from remote */
    )
    {
    BOOL will = enable;

    if (myOpts [opt] == enable)
	return (OK);

    switch (opt)
	{
	case TELOPT_BINARY:
	case TELOPT_ECHO:
	    setMode (slaveFd, opt, enable);
	    break;

	case TELOPT_SGA:
	    break;

	default:
	    will = FALSE;
	    break;
	}

    if ((myOpts [opt] != will) || remFlag)
	{
	char msg[3];

	msg[0] = IAC;
	msg[1] = will ? WILL : WONT;
	msg[2] = opt;

	write (clientSock, msg, 3);

	myOpts [opt] = will;
	}

    return ((will == enable) ? OK : ERROR);
    }

/*******************************************************************************
*
* setMode - set telnet option
*
* RETURNS: N/A.
*
* NOMANUAL
*
*/

LOCAL void setMode
    (
    int fd,
    int telnetOption,
    BOOL enable
    )
    {
    FAST int ioOptions;

    switch (telnetOption)
        {
        case TELOPT_BINARY: raw  = enable; break;
        case TELOPT_ECHO:   echo = enable; break;
        }

    if (raw)
        ioOptions = 0;
    else
        {
        ioOptions = OPT_7_BIT | OPT_ABORT | OPT_TANDEM | OPT_LINE;
        if (echo)
            {
            ioOptions |= (OPT_ECHO | OPT_CRMOD);
            }
    }

    (void) ioctl (fd, FIOOPTIONS, ioOptions);
    }

/*******************************************************************************
*
* telnetdStaticTaskInitializationGet - report whether tasks were pre-started by telnetd
*
* This function is called by a custom shell parser library to determine if a shell 
* is to be spawned at the time a connection is requested.
*
* RETURNS  
*
* TRUE, if all tasks are pre-spawned; FALSE, if tasks are spawned at the 
* time a connection is requested.
*
* SEE ALSO: telnetdInit(), telnetdParserSet()
*
* INTERNAL
* This routine is used by custom shells and is demonstrated in the 
* unsupported user shell echoShell.c
*
*/
BOOL telnetdStaticTaskInitializationGet()
    {
    return (telnetdTaskFlag);
    } 

/* rlogLib.c - remote login library */

/* Copyright 1984 - 2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
03m,07may02,kbw  man page edits
03l,30apr02,fmk  notify shell of remote connection/disconnection
03k,15oct01,rae  merge from truestack ver 03l, base 03i (AE support)
03j,24may01,mil  Bump up rlogin task stack size to 8000.
03i,14mar99,jdi  doc: removed refs to config.h and/or configAll.h (SPR 25663).
03h,05oct98,jmp  doc: cleanup.
03g,30oct96,dgp  doc: change task names for rlogind() per SPR #5902
03f,09aug94,dzb  fixed activeFlag race with cleanupFlag (SPR #2050).
                 made rlogindSocket global (SPR #1941).
	   +jpb  added username to rlogin "engaged" messages (SPR #3274).
03e,02may94,ms   increased stack size for SIMHPPA.
03d,24aug93,dvs  cleaned up removing pty from log fd list.
03c,23aug93,dvs  added logFdFromRlogin (SPR #2212).
03b,11aug93,jmm  Changed ioctl.h and socket.h to sys/ioctl.h and sys/socket.h
03a,02feb93,jdi  documentation tweak on configuration.
02z,20jan93,jdi  documentation cleanup for 5.1.
02y,19aug92,smb  Changed systime.h to sys/times.h.
02x,18jul92,smb  Changed errno.h to errnoLib.h.
02w,26may92,rrr  the tree shuffle
		  -changed includes to have absolute path from h/
02v,24apr92,rfs  Fixed flaky shell restart upon connection termination.
                 The functions rlogInTask() and rlogExit() were changed.
                 This is fixing SPR #1427.  Also misc ANSI noise.
02u,13dec91,gae  ANSI cleanup.
02t,19nov91,rrr  shut up some ansi warnings.
02s,14nov91,jpb  moved remCurIdSet to shellLogout (shellLib.c).
02r,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed READ, WRITE and UPDATE to O_RDONLY O_WRONLY and ...
		  -changed VOID to void
		  -changed copyright notice
02q,01aug91,yao  added to pass 6 args to excJobAdd() call.
02p,13may91,shl  undo'ed 02n.
02o,30apr91,jdi	 documentation tweaks.
02n,29apr91,shl  added call to restore original machine name, user and
		 group ids (spr 916).
02m,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by dnw.
02l,08mar91,jaa	 documentation cleanup.
02k,04oct90.shl	 fixed rlogExit() to restore original user and password.
02j,30sep90.del	 added htons macros to port references
02i,08may90,shl  changed entry point of tRlogind back to rlogind.
02h,18apr90,shl  added shell security.
		 changed rlogind name to tRlogind.
02g,07apr89,ecs  bumped rlogTaskStackSize from 2000 to 5000 for SPARC.
02f,07jun89,gae  changed SOCKADDR back to "struct sockaddr".
02e,06jun88,dnw  changed taskSpawn/taskCreate args.
02d,30may88,dnw  changed to v4 names.
02c,28may88,dnw  changed to use shellOrigStdSet (...) instead of shellSetOrig...
		 changed not to use shellTaskId global variable.
02b,31mar88,gae  made it work with I/O system revision.
02a,26jan88,jcf  made kernel independent.
01x,19nov87,dnw  made rlogin set terminal 7-bit option so that "~." and
		   XON-XOFF will be reliably detected.
		 cleaned-up rlogin loop.
		 changed rlogind to wait for shell to exist before accepting
		   remote connections.
	   +gae  fixed problem of rlogin() exiting before a possible
	           rlogind session gets to finish.
01w,18nov87,ecs  lint.
		 added include of inetLib.h.
01v,15nov87,dnw  changed rlogInit() to return status.
01u,03nov87,ecs  documentation.
	     &   fixed bug in use of setsockopt().
	    dnw  changed rlogin() to accept inet address as well as host name.
		 changed to call shellLogoutInstall() instead of logoutInstall.
01t,24oct87,gae  changed setOrig{In,Out,Err}Fd() to shellSetOrig{In,Out,Err}().
		 made rlogindOut() exit on EOF from master pty fd.
		 made rlogin return VxWorks prompt when client disconnects.
		 made rlogInit() not sweat being called more than once.
		 added shellLock() to rlogind & rlogin to get exclusive use
		   of shell.
01s,20oct87,gae  added logging device for rlogin shell; made rlogInit()
		   create pty device.
01r,03oct87,gae  moved logout to usrLib.c and made rlogindExit() from rlogindIn.
		 removed gratuitious standard I/O ioctl()'s.
		 made "disconnection" cleaner by having shell do the restart.
01q,04sep87,llk  added logout().  Made rlogindSock global so that logout() could
		   access it.
		 changed rlogind so that it doesn't exit when an accept fails.
01p,22apr87,dnw  changed rlogin to turn on xon-xoff to terminal.
		 fixed handling of termination sequence "~.".
		 changed default priority of rlogin tasks from 100 to 2.
		 made priority and task ids be global variables so they
		   can be accessed from outside this module.
01o,02apr87,ecs	 changed references to "struct sockaddr" to "SOCKADDR".
01n,27mar87,dnw  documentation.
01m,23mar87,jlf  documentation.
01l,27feb87,dnw  changed to spawn rlog tasks as UNBREAKABLE.
01k,20dec86,dnw  changed to use new call to remGetCurId().
		 changed old socket calls to normal i/o calls.
		 changed to not get include files from default directories.
		 added rlogInit ().
01j,27oct86,rdc  everything is now spawned in system mode with bigger stacks.
		 delinted.
01i,10oct86,gae  'remUser' made available through remGetCurId()...
		   included remLib.h.  Housekeeping.
01h,04sep86,jlf  documentation.
01g,31jul86,llk  uses new spawn
01f,27jul86,llk  added standard error fd which prints to the rlogin terminal.
		 Error messages go to standard error.
01e,07jul86,rdc  made rlogindInChild restart the shell after disconnect.
		 Documentation.
01d,19jun86,rdc  make rlogin close the socket after disconnect.
01c,18jun86,rdc  delinted.
01b,29apr86,rdc  rlogin now tries multiple local port numbers in its attempt
		 to connect to a remote host.
		 rlogind now uses KEEPALIVE option on sockets connected to
		 clients.
		 made rlogind unbreakable.
01a,02apr86,rdc  written.
*/

/*
DESCRIPTION
This library provides a remote login facility for VxWorks based on the UNIX 
`rlogin' protocol (as implemented in UNIX BSD 4.3).  On a VxWorks terminal, 
this command gives users the ability to log in to remote systems on the 
network.  

Reciprocally, the remote login daemon, rlogind(), allows remote users to 
log in to VxWorks.  The daemon is started by calling rlogInit(), which is 
called automatically when INCLUDE_RLOGIN is defined.  The remote login 
daemon accepts remote login requests from another VxWorks or UNIX system, 
and causes the shell's input and output to be redirected to the remote user.

Internally, rlogind() provides a tty-like interface to the remote
user through the use of the VxWorks pseudo-terminal driver ptyDrv.

INCLUDE FILES: rlogLib.h

SEE ALSO:
ptyDrv, telnetLib, UNIX BSD 4.3 manual entries for `rlogin', `rlogind', 
and `pty'
*/

#include "vxWorks.h"
#include "sys/socket.h"
#include "netinet/in.h"
#include "inetLib.h"
#include "ioLib.h"
#include "remLib.h"
#include "taskLib.h"
#include "sys/times.h"
#include "sockLib.h"
#include "stdio.h"
#include "unistd.h"
#include "string.h"
#include "errnoLib.h"
#include "excLib.h"
#include "logLib.h"
#include "hostLib.h"
#include "sysLib.h"
#include "ptyDrv.h"
#include "shellLib.h"
#include "fcntl.h"
#include "netLib.h"

#define LOGIN_SERVICE		513

#define STDIN_BUF_SIZE		200
#define STDOUT_BUF_SIZE		200

#define MAX_CONNECT_TRIES	5

char *rlogShellName	= "tShell";	/* task name we connect to */
char *rlogTermType	= "dumb/9600";	/* default terminal type */

/* rlogin task parameters */

int rlogTaskPriority	= 2;		/* task priority of rlogin tasks */
int rlogTaskOptions	= VX_SUPERVISOR_MODE | VX_UNBREAKABLE;

#if    (CPU_FAMILY == SIMHPPA) || (CPU_FAMILY == SIMSPARCSUNOS)
int rlogTaskStackSize   = 10000;         /* stack size of rlogin tasks */
#else  /* CPU_FAMILY == SIMHPPA */
int rlogTaskStackSize   = 8000;         /* stack size of rlogin tasks */
#endif /* CPU_FAMILY == SIMHPPA */

int rlogindId;				/* rlogind task ID */
int rlogInTaskId;			/* rlogInTask task ID */
int rlogOutTaskId;			/* rlogOutTask task ID */
int rlogChildTaskId;			/* rlogChildTask task ID */

int rlogindSocket;			/* rlogind socket fd */

/* externals */

IMPORT	int logFdFromRlogin;		/* fd of pty for rlogin */

/* local variables */

LOCAL char *ptyRlogName  = "/pty/rlogin.";
LOCAL char *ptyRlogNameM = "/pty/rlogin.M";
LOCAL char *ptyRlogNameS = "/pty/rlogin.S";

LOCAL BOOL activeFlag = FALSE;	/* TRUE if there is an active connection */
LOCAL BOOL cleanupFlag;		/* TRUE if exit cleanup has occurred */
LOCAL int rlogindM;		/* rlogind master pty */
LOCAL int rlogindS;		/* rlogind slave pty */
LOCAL int rloginSocket;		/* rlogin socket */

LOCAL int shellInFd;		/* original console input */
LOCAL int shellOutFd;		/* original console output */
LOCAL int shellErrFd;		/* original console error output */


/* forward declarations */

void rlogind ();
void rlogInTask ();
void rlogOutTask ();
void rlogChildTask ();


/* forward static functions */

static void rlogExit (BOOL usedLogout);
static STATUS recvStr (int sd, char *buf);
static void childTaskSpawn (int	priority, int options, int stackSize);


/*******************************************************************************
*
* rlogInit - initialize the remote login facility
*
* This routine initializes the remote login facility.  It creates a pty 
* (pseudo tty) device and spawns rlogind().  If INCLUDE_RLOGIN is included,
* rlogInit() is called automatically at boot time.
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  This restriction does not apply under non-AE 
* versions of VxWorks.  
*
* RETURNS: OK or ERROR.
*
* SEE ALSO: ptyDrv
*/

STATUS rlogInit (void)
    {
    static BOOL done;	/* FALSE = not done */

    if (done)
	{
	printErr ("rlogInit: already initialized.\n");
	return (ERROR);
	}
    else
	done = TRUE;

    if (ptyDrv () == ERROR || ptyDevCreate (ptyRlogName, 1024, 1024) == ERROR)
	{
	printErr ("rlogInit: unable to create pty device.\n");
	return (ERROR);
	}

    rlogindId = taskSpawn ("tRlogind", rlogTaskPriority,
			   rlogTaskOptions, rlogTaskStackSize,
			   (FUNCPTR)rlogind, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    if (rlogindId == ERROR)
	{
	printErr ("rlogInit: unable to spawn rlogind.\n");
	return (ERROR);
	}

    return (OK);
    }
/*******************************************************************************
*
* rlogind - the VxWorks remote login daemon
*
* This routine provides a facility for remote users to log in to VxWorks over
* the network.  If INCLUDE_RLOGIN is defined, rlogind() is spawned by
* rlogInit() at boot time.
*
* Remote login requests will cause `stdin', `stdout', and `stderr' to be
* directed away from the console.  When the remote user disconnects,
* `stdin', `stdout', and `stderr' are restored, and the shell is restarted.
* The rlogind() routine uses the remote user verification protocol specified
* by the UNIX remote shell daemon documentation, but ignores all the
* information except the user name, which is used to set the VxWorks remote
* identity (see the manual entry for iam()).
*
* The remote login daemon requires the existence of a pseudo-terminal
* device, which is created by rlogInit() before rlogind() is spawned.  The
* rlogind() routine creates two child processes, `tRlogInTask' and
* `tRlogOutTask', whenever a remote user is logged in.  These processes exit
* when the remote connection is terminated.
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  In addition, all arguments to this function can  
* reference only that data which is valid in the kernel protection domain. 
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS: N/A
*
* SEE ALSO: rlogInit(), iam()
*/

void rlogind (void)
    {
    char remUser [MAX_IDENTITY_LEN];	/* remote user trying to log in */
    char curUser [MAX_IDENTITY_LEN];	/* current user already logged in */
    char buf [STDIN_BUF_SIZE];
    int optval;
    struct sockaddr_in myAddress;
    struct sockaddr_in clientAddress;
    int clientAddrLen;
    int masterFd;
    int slaveFd;
    int client;
    int sd;

    /* open a socket and wait for a client */

    sd = socket (AF_INET, SOCK_STREAM, 0);

    bzero ((char *) &myAddress, sizeof (myAddress));
    myAddress.sin_family = AF_INET;
    myAddress.sin_port   = htons (LOGIN_SERVICE);

    if (bind (sd, (struct sockaddr *) &myAddress, sizeof (myAddress)) == ERROR)
	{
	printErr ("rlogind: bind failed.\n");
	return;
	}

    listen (sd, 1);

    FOREVER
	{
	/* wait for shell to exist */

	while (taskNameToId (rlogShellName) == ERROR)
	    taskDelay (sysClkRateGet ());

	errnoSet (OK);		/* clear errno for pretty i() display */

	/* now accept connection */

	clientAddrLen = sizeof (clientAddress);
	client = accept (sd, (struct sockaddr *)&clientAddress, &clientAddrLen);

	if (client == ERROR)
	    {
	    printErr ("rlogind: accept failed - status = 0x%x", errnoGet());
	    continue;
	    }

	/* turn on KEEPALIVE so if the client crashes, we'll know about it */

	optval = 1;
	setsockopt (client, SOL_SOCKET, SO_KEEPALIVE,
		    (char *)&optval, sizeof (optval));


	/* read in initial strings from remote rlogin */

	if ((recvStr (client, buf) == ERROR) ||	    /* ignore stderr */
	    (recvStr (client, remUser) == ERROR) || /* get local user name */
	    (recvStr (client, buf) == ERROR) ||	    /* ignore remote user name*/
	    (recvStr (client, buf) == ERROR))	    /* ignore terminal stuff */
	    {
	    close (client);
	    continue;
	    }

	/* acknowlege connection */

	write (client, "", 1);

	/* check to see if there's already an active connection */

	if (activeFlag)
	    {
	    char msg [STDOUT_BUF_SIZE];

	    sprintf (msg, "\r\nSorry, this system is engaged by user: %s.\r\n",
		curUser);

	    write (client, msg, strlen (msg));
	    close (client);
	    continue;
	    }

	/* create the pseudo terminal:
	 * the master side is connected to the socket to the
	 * remote machine - two processes rlogInTask & rlogOutTask
	 * handle input and output.
	 */

	if ((masterFd = open (ptyRlogNameM, O_RDWR, 0)) == ERROR)
	    {
	    char *msg = "\r\nSorry, trouble with pty.\r\n";

	    printErr ("rlogind: error opening %s\n", ptyRlogNameM);
	    write (client, msg, strlen (msg));
	    close (client);
	    continue;
	    }

	if ((slaveFd = open (ptyRlogNameS, O_RDWR, 0)) == ERROR)
	    {
	    char *msg = "\r\nSorry, trouble with pty.\r\n";

	    printErr ("rlogind: error opening %s\n", ptyRlogNameS);
	    write (client, msg, strlen (msg));
	    close (client);
	    close (masterFd);
	    continue;
	    }

	if (!shellLock (TRUE))
	    {
	    char *msg = "\r\nSorry, shell is locked.\r\n";

	    printErr ("rlogind: user: %s tried to login.\n", remUser);
	    write (client, msg, strlen (msg));
	    close (client);
	    close (masterFd);
	    close (slaveFd);
	    continue;
	    }

	/* setup the slave device to act like a terminal */

	ioctl (slaveFd, FIOOPTIONS, OPT_TERMINAL);

        strcpy (curUser, remUser);
	printf ("\nrlogind: This system *IN USE* via rlogin by user: %s.\n",
	    curUser);

	shellLogoutInstall ((FUNCPTR)rlogExit, TRUE);
	activeFlag    = TRUE;
	rlogindSocket = client;
	rlogindM      = masterFd;
	rlogindS      = slaveFd;

	/* flush out pty device */

	ioctl (slaveFd, FIOFLUSH, 0 /*XXX*/);

	/* save the shell's standard I/O fd's so we can restore them later */

	shellInFd  = ioGlobalStdGet (STD_IN);
	shellOutFd = ioGlobalStdGet (STD_OUT);
	shellErrFd = ioGlobalStdGet (STD_ERR);

	/* set shell's standard I/O to pty device; add extra logging device */

	shellOrigStdSet (STD_IN,  slaveFd);
	shellOrigStdSet (STD_OUT, slaveFd);
	shellOrigStdSet (STD_ERR, slaveFd);

	logFdAdd (slaveFd);
	logFdFromRlogin = slaveFd;	/* store pty fd for logFdSet() */
	
	/* Notify shell of remote connection */
	
	shellIsRemoteConnectedSet (TRUE);

	/* the shell is currently stuck in a read from the console,
	 * so restart it */

	excJobAdd (shellRestart, TRUE, 0, 0, 0, 0, 0);

	/* spawn the process which transfers data from the master pty
	 * to the socket;
	 * spawn the process which transfers data from the client socket
	 * to the master pty.
	 */

	if (((rlogOutTaskId = taskSpawn("tRlogOutTask", rlogTaskPriority,
					rlogTaskOptions, rlogTaskStackSize,
					(FUNCPTR)rlogOutTask, client, masterFd,
					0, 0, 0, 0, 0, 0, 0, 0)) == ERROR) ||
	    ((rlogInTaskId = taskSpawn ("tRlogInTask", rlogTaskPriority,
					rlogTaskOptions, rlogTaskStackSize,
					(FUNCPTR)rlogInTask, client, masterFd,
					0, 0, 0, 0, 0, 0, 0, 0)) == ERROR))
	    {
	    printErr ("rlogind: error spawning %s child - status = 0x%x\n",
		      (rlogOutTaskId != ERROR) ? "output" : "input",
		      errnoGet ());

	    if (rlogOutTaskId != ERROR)
		taskDelete (rlogOutTaskId);

	    rlogExit (FALSE);		/* try to do tidy clean-up */
	    }
	}
    }
/*******************************************************************************
*
* rlogOutTask - stdout to socket process
*
* This routine gets spawned by the rlogin daemon to move
* data between the client socket and the pseudo terminal.
* The task exits when the pty has been closed.
*
* NOMANUAL
* but not LOCAL for i()
*/

void rlogOutTask
    (
    FAST int sock,      /* socket to copy output to */
    int ptyMfd          /* pty Master fd */
    )
    {
    int n;
    char buf [STDOUT_BUF_SIZE];

    while ((n = read (ptyMfd, buf, sizeof (buf))) > 0)
	write (sock, buf, n);
    }

/*******************************************************************************
*
* rlogInTask - socket to stdin process
*
* This routine gets spawned by the rlogin daemon to move
* data between the pseudo terminal and the client socket.
* The task exits, calling rlogExit(), when the client disconnects.
*
* NOMANUAL
* but not LOCAL for i()
*/

void rlogInTask
    (
    FAST int sock,      /* socket to copy input from */
    int ptyMfd          /* pty Master fd */
    )
    {
    FAST int n;
    char buf [STDIN_BUF_SIZE];

    cleanupFlag = FALSE;	/* exit cleanup has not been done... */

    /* Loop, reading from the socket and writing to the pty. */

    while ((n = read (sock, buf, sizeof (buf))) > 0)
        write (ptyMfd, buf, n);

    /* Exit and cleanup.  The above loop will exit when the socket is
     * closed.  The socket can be closed as a result of the connection
     * terminating from the remote host, or as a result of the logout()
     * command issued to our shell.  When the logout() command is used,
     * the rlogExit() routine below is called and the socket is explicitly
     * closed.  In this case, there is no need to call rlogExit() again.
     * We use the cleanupFlag to determine this case.  To summarize, if the
     * cleanupFlag is set, we are here because the connection was terminated
     * remotely and cleanup is required.
     */

    if (!cleanupFlag)
        rlogExit (FALSE);
    }

/*******************************************************************************
*
* rlogExit - exit and cleanup for rlogind
*
* This is the support routine for logout().
* The client socket is closed, shell standard I/O is redirected
* back to the console, and the shell is restarted.
*/

LOCAL void rlogExit
    (
    BOOL usedLogout    /* true if called by logout() */
    )
    {
    /* This routine is called either as a result of the logout() command
     * being issued from the shell, or from the rlogInTask above.  It is
     * therefore run in the context of the shell or rlogInTask.  The
     * caller indicates itself to us by the usedLogout argument.  The
     * state of this argument affects our behavior, as explained below.
     */

    cleanupFlag = TRUE;			/* release rlogInTask() from duty */
    shellLogoutInstall ((FUNCPTR) NULL, 0);     /* uninstall logout function */

    if (logFdFromRlogin != NONE)
	{
    	logFdDelete (logFdFromRlogin);          /* cancel extra log device */
    	logFdFromRlogin = NONE;			/* reset fd */
	}

    shellOrigStdSet (STD_IN,  shellInFd);       /* restore shell's stnd I/O */
    shellOrigStdSet (STD_OUT, shellOutFd);
    shellOrigStdSet (STD_ERR, shellErrFd);

    shellLock (FALSE);                          /* unlock shell */
    
    /* Notify shell of remote disconnection */
	
    shellIsRemoteConnectedSet (FALSE);
    
    write (rlogindSocket, "\r\n", 2);
    close (rlogindSocket);                      /* close the socket */

    /* For typical remote sessions, there is no need to restart the shell.
     * If we are in shell context, simply restoring the standard I/O
     * descriptors is enough to get the shell back on track upon return
     * from this function.  If we are in rlogInTask context, the closing
     * of the pty device will cause the shell to unblock from its read()
     * and do subsequent I/O from the restored descriptors.
     * However, problems can occur upon logout if the remote user has
     * disabled the line editor and/or put the pty device in raw mode.
     * The problem caused is that the shell does not resume properly.
     * It is therefore deemed prudent to always restart the shell, thereby
     * avoiding any funny business.
     *
     * The previous version attempted to send a ctrl-D up the pty device
     * to wakeup and restart the shell.  Unfortunately, ctrl-D only has
     * special meaning when the device is in line mode, and hence did
     * not work in raw mode.
     *
     * The pty device is closed after the shell is restarted, when called
     * from rlogInTask, to avoid waking the existing shell and causing an
     * additional prompt to appear on the console.
     */

    if (usedLogout)                             /* called from shell */
        {
        close (rlogindM);
        close (rlogindS);
        activeFlag = FALSE;			/* allow new connection */
        taskRestart (0);                        /* never returns */
        }
    else                                        /* called from rlogInTask */
        {
        excJobAdd (shellRestart, FALSE, 0, 0, 0, 0, 0);
        close (rlogindM);
        close (rlogindS);
        activeFlag = FALSE;			/* allow new connection */
        }
    }

/*******************************************************************************
*
* recvStr - receive a null terminated string from a socket
*
* Similar to fioRdString, but for sockets.
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS recvStr
    (
    FAST int sd,        /* socket */
    FAST char *buf      /* where to put the string */
    )
    {
    do
	{
	if (read (sd, buf, 1) != 1)
	    return (ERROR);
	}
    while (*(buf++) != EOS);

    return (OK);
    }

/*******************************************************************************
*
* childTaskSpawn - spawn rlogChildTask via netJob queue
*
* RETURNS: N/A
*/
LOCAL void childTaskSpawn
    (
    int		priority,
    int		options,
    int		stackSize
    )
    {
    rlogChildTaskId = taskSpawn ("tRlogChildTask", priority, options,
				 stackSize, (FUNCPTR)rlogChildTask,
				 0,0,0,0,0,0,0,0,0,0);
    if (rlogChildTaskId == ERROR)
        {
        printErr ("rlogin: trouble spawning child - status = 0x%x\n",
		  errnoGet ());
        close (rloginSocket);
	}

    }


/*******************************************************************************
*
* rlogin - log in to a remote host
*
* This routine allows users to log in to a remote host.  It may be called from
* the VxWorks shell as follows:
* .CS
*    -> rlogin "remoteSystem"
* .CE
* where <remoteSystem> is either a host name, which has been previously added
* to the remote host table by a call to hostAdd(), or an Internet address in
* dot notation (e.g., "90.0.0.2").  The remote system will be logged into
* with the current user name as set by a call to iam().
*
* The user disconnects from the remote system by typing:
* .CS
*    ~.
* .CE
* as the only characters on the line, or by simply logging out from the remote
* system using logout().
*
* RETURNS:
* OK, or ERROR if the host is unknown, no privileged ports are available,
* the routine is unable to connect to the host, or the child process cannot
* be spawned.
*
* SEE ALSO: iam(), logout()
*/
STATUS rlogin
    (
    char *host          /* name of host to connect to */
    )
    {
    char remUser [MAX_IDENTITY_LEN];
    char remPasswd [MAX_IDENTITY_LEN];
    struct sockaddr_in hostAddr;
    int port;
    char ch;
    int quitFlag;
    int status;
    int nTries;

    /* connect to host */

    if (((hostAddr.sin_addr.s_addr = hostGetByName (host)) == ERROR) &&
        ((hostAddr.sin_addr.s_addr = inet_addr (host)) == ERROR))
	{
	printErr ("rlogin: unknown host.\n");
	return (ERROR);
	}

    hostAddr.sin_family = AF_INET;
    hostAddr.sin_port   = htons (LOGIN_SERVICE);

    remCurIdGet (remUser, remPasswd);

    port   = 1000;
    status = ERROR;

    for (nTries = 0; status == ERROR && nTries < MAX_CONNECT_TRIES; nTries++)
	{
	if ((rloginSocket = rresvport (&port)) == ERROR)
	    {
	    printErr ("rlogin: no available privileged ports.\n");
	    return (ERROR);
	    }

	status = connect (rloginSocket, (struct sockaddr *)&hostAddr,
			    sizeof(hostAddr));
	port--;
	}

    if (status == ERROR)
	{
	printErr ("rlogin: could not connect to host.\n");
	close (rloginSocket);
	return (ERROR);
	}

    /* send a null (no seperate STDERR) */

    write (rloginSocket, "", 1);

    /* send the local and remote user names */

    write (rloginSocket, remUser, strlen (remUser) + 1);
    write (rloginSocket, remUser, strlen (remUser) + 1);

    /* send the terminal type */

    write (rloginSocket, rlogTermType, strlen (rlogTermType) + 1);

    /* spawn a process to handle stdin */

    status = netJobAdd ((FUNCPTR)childTaskSpawn, rlogTaskPriority,
                        rlogTaskOptions, rlogTaskStackSize, 0, 0);

    if (status != OK)	/* netJob queue overflow, error printed by panic */
	{
        close (rloginSocket);
	return (ERROR);
	}

    /* force shell to be locked, shellLock is FALSE if already locked */

    shellLock (TRUE);

    /* set console to RAW mode except with XON-XOFF and 7 bit */

    ioctl (STD_IN, FIOOPTIONS, OPT_TANDEM | OPT_7_BIT);

    quitFlag = 0;

    while ((rloginSocket != ERROR) && (read (STD_IN, &ch, 1) == 1))
	{
	/* track input of "<CR>~.<CR>" to terminate connection */

	if ((quitFlag == 0) && (ch == '~'))
	    quitFlag = 1;
	else if ((quitFlag == 1) && (ch == '.'))
	    quitFlag = 2;
	else if ((quitFlag == 2) && (ch == '\r'))
	    break;		/* got "<CR>~.<CR>" */
	else
	    quitFlag = (ch == '\r') ? 0 : -1;

	write (rloginSocket, &ch, 1);
	}

    /* wait for other tasks to finish up, i.e. rlogind */

    taskDelay (sysClkRateGet () / 2);

    if (rloginSocket != ERROR)
	{
	taskDelete (rlogChildTaskId);
	close (rloginSocket);
	}

    /* reset console */

    ioctl (STD_IN, FIOOPTIONS, OPT_TERMINAL);
    printf ("\nClosed connection.\n");

    shellLock (FALSE);

    return (OK);
    }
/*******************************************************************************
*
* rlogChildTask - rlogin child
*
* This routine gets spawned by rlogin() to read data from
* the rloginSocket and write it to stdout.
* The task exits when the client disconnects;
* rlogin is informed by setting rloginSocket to ERROR.
*
* NOMANUAL
* but not LOCAL for i()
*/

void rlogChildTask (void)
    {
    char buf [STDOUT_BUF_SIZE];
    int n;

    while ((n = read (rloginSocket, buf, sizeof (buf))) > 0)
	write (STD_OUT, buf, n);

    close (rloginSocket);

    /* indicate that client side caused termination,
     * and stop rlogin from reading stdin */

    rloginSocket = ERROR;

    ioctl (STD_IN, FIOCANCEL, 0 /*XXX*/);
    }

/* remShellLib.c - remote access to target shell */

/* Copyright 1984 - 2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01b,30apr02,elr  Login stdin / stdout isolated from shell until login completed
                 to correct security holes SPR 30687, 5059
           +fmk  added notification to shell of remote session
01a,14feb01,spm  inherited from version 01a of tor2_0_x branch 
*/

/*
DESCRIPTION
This library contains the support routines for remote access to the VxWorks
target shell for clients using the telnet or rlogin protocols. It supplies
file descriptors to connection telnet or rlogin sessions to the shell's
command interpreter.

INCLUDE FILES: remShellLib.h, shellLib.h
*/

#include "vxWorks.h"
#include "ioLib.h"
#include "logLib.h"
#include "ptyDrv.h"
#include "shellLib.h"
#include "taskLib.h"
#include "telnetLib.h"
#include "stdio.h"
#include "fioLib.h"
#include "sysLib.h"

IMPORT  int logFdFromRlogin;    /* fd of pty for remote sessions */

/* local variables */

LOCAL int shellInFd;            /* original console input */
LOCAL int shellOutFd;           /* original console output */
LOCAL int shellErrFd;           /* original console error output */

LOCAL UINT32 remoteId = 0; 		/* Identifies current remote session */

/*******************************************************************************
*
* shellParserControl - handle connecting and disconnecting remote users
*
* This routine configures the shell to connect new telnet or rlogin sessions
* to the command interpreter by redirecting standard input and standard
* output and restores the original settings when those sessions exit. This
* routine is the default parser control installed when both INCLUDE_TELNET
* and INCLUDE_SHELL are defined. It only supports a single remote session
* at a time.
*
* RETURNS: OK or ERROR.
* 
* INTERNAL: In the future should be two functions  setupIoForTelnet() called 
*            from telnetd and resumeLocalIo() called from logout()
*
*
* NOMANUAL
*/

STATUS shellParserControl
    (
    UINT32 remoteEvent,	/* Starting or stopping a connection? */
    UINT32 sessionId, 	/* Unique identifier for each session */
    UINT32 slaveFd      /* File descriptor for character i/o  */
    )
    {

    if ((taskNameToId ("tShell")) == ERROR)   /* Shell not started yet. */
        return (ERROR);

    if (remoteEvent == REMOTE_START)
        {
        /* Handle a new telnet or rlogin session. */

        if (remoteId != 0)    /* Failed request - only one session allowed. */
            return (ERROR); 

        if (!shellLock (TRUE)) 	/* Shell is not available. */
            {
	    fdprintf (slaveFd, "The shell is currently in use.\n");
            return (ERROR);
            }

        /* Let the user try to login */
	if (shellLogin (slaveFd) != OK)
	    { 
 	    shellLock (FALSE);
            return (ERROR);
            }

        /* setup the slave device to act like a terminal */

        (void) ioctl (slaveFd, FIOOPTIONS, OPT_TERMINAL);

        shellLogoutInstall ((FUNCPTR) telnetdExit, sessionId);

        /* get the shell's standard I/O fd's so we can restore them later */

        shellInFd  = ioGlobalStdGet (STD_IN);
        shellOutFd = ioGlobalStdGet (STD_OUT);
        shellErrFd = ioGlobalStdGet (STD_ERR);

        /* set shell's standard I/O to new device; add extra logging device */

        shellOrigStdSet (STD_IN, slaveFd);
        shellOrigStdSet (STD_OUT, slaveFd);
        shellOrigStdSet (STD_ERR, slaveFd);

        logFdAdd (slaveFd);
        logFdFromRlogin = slaveFd;      /* store new fd for logFdSet() */

        /* Store the session identifier. */

        remoteId = sessionId;

        /* notify the shell we have started a remote session */
        
        shellIsRemoteConnectedSet (TRUE);

        printErr ("\ntelnetd: This system *IN USE* via telnet.\n");

        /* Prevent network denial of service attacks by waiting a second */

        taskDelay (sysClkRateGet() / 2); 
       
        /* Restart the shell to access the redirected file descriptors. */

        excJobAdd (shellRestart, TRUE, 0, 0, 0, 0, 0);

        return (OK);
        }
    else if (remoteEvent == REMOTE_STOP)
        {
        /*
         * End an active telnet or rlogin session. This event occurs
         * after the server closes the socket.
         */

        if (remoteId != sessionId)    /* Unknown remote session. */
            return (ERROR);

        shellLogoutInstall ((FUNCPTR) NULL, 0);  /* remove logout function */

        if (logFdFromRlogin != NONE)
            {
            logFdDelete (logFdFromRlogin);       /* cancel extra log device */
            logFdFromRlogin = NONE;              /* reset fd */
            }

        shellOrigStdSet (STD_IN,  shellInFd);    /* restore shell's stnd I/O */
        shellOrigStdSet (STD_OUT, shellOutFd);
        shellOrigStdSet (STD_ERR, shellErrFd);

        shellLock (FALSE);                       /* unlock shell */

        /*
         * For typical remote sessions, restoring the standard I/O
         * descriptors is enough to reconnect the shell to the console
         * because closing the pty device will cause the shell to unblock
         * from its read() and use the restored descriptors. However,
         * problems can occur upon logout if the remote user has disabled
         * the line editor and/or put the pty device in raw mode, so the
         * shell is restarted in all cases.
         */

        remoteId = 0;    /* Allow a new session. */

       /* notify the shell we have ended a remote session */
        
        shellIsRemoteConnectedSet (FALSE);

        excJobAdd (shellRestart, FALSE, 0, 0, 0, 0, 0);

        return (OK);
        }

    return (ERROR);    /* Ignore unknown control operations. */
    }

/* remLib.c - remote command library */

/* Copyright 1984 - 2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02u,10may02,kbw  making man page edits
02t,15oct01,rae  merge from truestack ver 02w, base 02s (SPR #62484)
02s,11aug93,jmm  Changed ioctl.h and socket.h to sys/ioctl.h and sys/socket.h
02r,24jan93,jdi  documentation cleanup for 5.1.
02q,04sep92,jmm  changed bindresvport() to correctly check the value of port
                   (spr #1469)
02p,19aug92,smb  Changed systime.h to sys/times.h.
02o,18jul92,smb  Changed errno.h to errnoLib.h.
02n,26may92,rrr  the tree shuffle
		  -changed includes to have absolute path from h/
02m,19nov91,rrr  shut up some ansi warnings.
02l,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -changed VOID to void
		  -changed copyright notice
02k,30apr91,jdi	 documentation tweaks.
02j,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by dnw.
02i,11mar91,jaa	 documentation cleanup.
02h,02oct90,hjb  added calls to htons() and ntohs() where needed.
02g,19jul90,dnw  mangen fix
02f,19apr90,hjb  added bindresvport(), modified rresvport()
02e,07mar90,jdi  documentation cleanup.
02d,07jun89,gae  changed SOCKADDR back to "struct sockaddr".
02c,03may89,dnw  moved iam() and whoami() here remLib.c from usrLib.c
		   to prevent usrLib from dragging in the network.
02b,30may88,dnw  changed to v4 names.
02a,28may88,dnw  removed host table routines to hostLib.
		 removed iam, whoami to usrLib.
		 changed call to fioStdErr to STD_ERR.
01w,04mar88,jcf  changed semaphores, and sem calls for new semLib.
01v,09jan88,gae  made hostTable a list -- no longer a fixed size.
01u,15dec87,gae  used inet_ntoa_b() instead of inet_ntoa(); misc. cleanups.
01t,14dec87,gae  documentation & checked malloc's for NULL.
01s,11nov87,jlf  documentation
01r,06nov87,dnw  changed remRcmd to take either host name or inet address.
01q,11jun87,llk  added remShowHosts() which shows all hosts in the host table
		   reachable from this VxWorks system.
		 changed remInetAddr() to call inet_addr.  remInetAddr() is
		   now considered obsolete.  Use inet_addr() instead.
		 implemented host name aliases in host table.
		 moved nethelp() from here to netLib.c.
		 added remFillHostName().
		 changed closeSocket() calls to close().
01p,02apr87,ecs  added include of strLib.h.
		 changed references to "struct sockaddr" to "SOCKADDR".
01o,24mar87,jlf  documentation.
01n,20dec86,dnw  changed iam() and remGetCurId to take password as 2nd param.
		 updated nethelp().
		 changed to not get include files from default directories.
01m,08nov86,dnw  change remRresvport to choose different port numbers
		   each time.  This is a TEMP (!?) fix for new net timing bug.
		 fixed remRcmd() not cleaning up correctly on some errors.
01l,29oct86,rdc	 deleted references to exLib.h and exos.h.
01k,28oct86,rdc	 lint.
01j,06oct86,gae	 got rid of rcd() and rpwd()...netDrv handles it.
		 Moved HOST and HOSTNAME here from remLib.h for better hiding.
01i,04sep86,jlf  documentation.
01h,30jul86,rdc  fixed remRcmd to comply with (undocumented) rshd protocol.
01g,17jul86,rdc  added seperate stderr stream to remRcmd.
		 remRcmd now checks for recv errors correctly.
01f,11jul86,rdc  wrote nethelp.
01e,21may86,llk	 moved remCat to netDrv.c, called it netGet.
		 changed memAllocate to malloc.
	    rdc  delinted.
01d,27mar86,rdc  made remCat a little less sensitive to crashed sockets.
		 made remCat test for write errors properly.
01c,28jan86,jlf  documentation
01b,08oct85,rdc  de-linted
01a,03oct85,rdc  written
*/

/*
This library provides routines that support remote command functions.  
The rcmd() and rresvport() routines use protocols implemented in BSD 4.3; 
they support remote command execution, and the opening of a socket with 
a bound privileged port, respectively.  For more information, 
see <Unix Network Programming> by W. Richard Stevens. This library also 
includes routines that authorize network file access via 'netDrv'.

To include 'remLib' in a VxWorks image, include the NETWRS_REMLIB 
configuration component.  This component contains one parameter, 
RSH_STDERR_SETUP_TIMEOUT.  Use this parameter to specify how  
long an rcmd() call should wait for a return from its internal 
call to select().  Valid values for RSH_STDERR_SETUP_TIMEOUT 
are 0 (NO_WAIT), -1 (WAIT_FOREVER), or a positive integer 
from 1 to 2147483647 inclusive.  This positive integer specifies 
the wait in seconds.  The default value for RSH_STDERR_SETUP_TIMEOUT 
is -1 (WAIT_FOREVER).

INCLUDE FILES
remLib.h

SEE ALSO
inetLib
*/

#include "vxWorks.h"
#include "sys/socket.h"
#include "netinet/in.h"
#include "remLib.h"
#include "inetLib.h"
#include "string.h"
#include "errnoLib.h"
#include "sys/times.h"
#include "sockLib.h"
#include "stdio.h"
#include "unistd.h"
#include "hostLib.h"
#include "tickLib.h"

#define MAX_RETRY_CONNECT 5

LOCAL char remUser [MAX_IDENTITY_LEN];
LOCAL char remPasswd [MAX_IDENTITY_LEN];

int remLastResvPort = IPPORT_RESERVED - 1;
long remStdErrSetupTimeout = 30;		/* RSH's stderr setup timeout(sec) */
				        	/* select() timeout for 2nd port */ 

/*******************************************************************************
*
* remLibInit - initialize remLib
*
* RETURNS: N/A
* 
* NOMANUAL
*/

void remLibInit
    (
    long timeout			/* RSH_STDERR_SETUP_TIMEOUT */
    )
    {
    remStdErrSetupTimeout = timeout;
    }

/*******************************************************************************
*
* rcmd - execute a shell command on a remote machine
*
* This routine uses a remote shell daemon, 'rshd', to execute a command 
* on a remote system.  It is analogous to the BSD rcmd() routine.
*
* Internally, this rcmd() implementation uses a select() call to wait for 
* a response from the 'rshd' daemon.  If rcmd() receives a response within  
* its timeout, rcmd() calls accept() and completes by returning a socket 
* descriptor for the data generated on the remote machine.
*
* The default timeout lets the rcmd() call wait forever.  However, 
* you can change the timeout value using the RSH_STDERR_SETUP_TIMEOUT 
* parameter associated with the NETWRS_REMLIB configuration component.
*
* RETURNS
* A socket descriptor if the remote shell daemon accepts, or
* ERROR if the remote command fails.
*
* S_remLib_RSH_ERROR, S_remLib_RSH_STDERR_SETUP_FAILED
*
* SEE ALSO: BSD reference entry for rcmd()
*/

int rcmd
    (
    char *host,         /* host name or inet address */
    int remotePort,     /* remote port to connect to (rshd) */
    char *localUser,    /* local user name */
    char *remoteUser,   /* remote user name */
    char *cmd,          /* command */
    int *fd2p           /* if this pointer is non-zero, */
			/* stderr socket is opened and */
			/* socket descriptor is filled in */
    )
    {
    int sd;
    struct sockaddr_in sin;
    struct sockaddr_in mySin;
    int mySinLen;
    char c;
    int lport = 0;
    int nTries = 0;
    int stdErrPort;
    int stdErrSocket;
    char stringBuf [20];

    sin.sin_family = AF_INET;
    sin.sin_port = htons (remotePort);
    if (((sin.sin_addr.s_addr = hostGetByName (host)) == ERROR) &&
        ((sin.sin_addr.s_addr = inet_addr (host)) == ERROR))
	{
	return (ERROR);
	}

    do
	{
	sd = rresvport (&lport);
	if (sd == ERROR)
	    return (ERROR);

	if (connect (sd, (struct sockaddr *) &sin, sizeof (sin)) == ERROR)
	    {
	    close (sd);
	    lport--;
	    }
	else
	    break;
	}
    while (++nTries <= MAX_RETRY_CONNECT);

    if (nTries > MAX_RETRY_CONNECT)
	return (ERROR);

    if (fd2p == 0)
        {
        if (send (sd, "", 1, 0) <= 0)
	    {
	    close (sd);
	    return (ERROR);
	    }
        lport = 0;
        }
    else
        {
	fd_set  readfds;
	struct timeval *pT, t;

	stdErrPort = --lport;

	stdErrSocket = rresvport (&stdErrPort);
	if (stdErrSocket == ERROR)
	    {
	    close (sd);
	    return (ERROR);
	    }
	listen (stdErrSocket, 1);

	sprintf (stringBuf, "%d", stdErrPort);
	if (send (sd, stringBuf, strlen (stringBuf) + 1, 0) <= 0)
	    {
	    close (sd);
	    close (stdErrPort);
	    return (ERROR);
	    }
	    
	/* wait for rshd to connect. vxWorks will wait the connect     */
        /* by user-configurable timeout here(note: this timeout is     */
        /* just policy, so user can set whatever as far as the timeout */
        /* does not conflict with existing TCP timers.                 */

#ifndef MAX	
#define MAX(a, b)  ((a) > (b)? (a): (b))
#endif

	FD_ZERO(&readfds);
	FD_SET(sd, &readfds);
	FD_SET(stdErrSocket, &readfds);

        /* setup select timeout given by user */

        switch (remStdErrSetupTimeout)
            {
            case NO_WAIT:
	        bzero ((char *)&t, sizeof(struct timeval));
		pT = &t;
                break;

            case WAIT_FOREVER:
                pT = NULL;
                break;

            default:
	        bzero ((char *)&t, sizeof(struct timeval));
                t.tv_sec = remStdErrSetupTimeout;
		pT = &t;
            }

	if (select (MAX(sd, stdErrSocket)+1, &readfds, (fd_set *)NULL,
		    (fd_set *)NULL, pT) < 1 ||
	    !FD_ISSET(stdErrSocket, &readfds))
	    {
	    close (sd);
	    close (stdErrSocket);	    
	    errnoSet (S_remLib_RSH_STDERR_SETUP_FAILED);
	    return (ERROR);
	    }

	/* rshd got back to the secondary port */

	mySinLen = sizeof (mySin);
	*fd2p = accept (stdErrSocket, (struct sockaddr *)&mySin, &mySinLen);
	if (*fd2p == ERROR)
	    {
	    close (sd);
	    close (stdErrSocket);
	    return (ERROR);
	    }
	close (stdErrSocket);
	}

    if (send (sd, localUser, strlen(localUser) + 1, 0) <= 0)
	{
	close (sd);
	if (fd2p != NULL)
	    close (*fd2p);
	return (ERROR);
	}

    if (send (sd, remoteUser, strlen(remoteUser) + 1, 0) <= 0)
	{
	close (sd);
	if (fd2p != NULL)
	    close (*fd2p);
	return (ERROR);
	}

    if (send (sd, cmd, strlen(cmd) + 1, 0) <= 0)
	{
	close (sd);
	if (fd2p != NULL)
	    close (*fd2p);
	return (ERROR);
	}

    /* bsd documentation for rshd is incorrect - null byte is actually
       received on stdin socket */

    if (recv (sd, &c, 1, 0) <= 0)
	{
	close (sd);
	if (fd2p != NULL)
	    close (*fd2p);
	return (ERROR);
	}

    if (c != 0)
	{
	/* error will come in on stdin socket */
	while (recv (sd, &c, 1, 0) == 1)
	    {
	    write (STD_ERR, &c, 1);
	    if (c == '\n')
		break;
	    }

	errnoSet (S_remLib_RSH_ERROR);
	close (sd);

	if (fd2p != NULL)
	    close (*fd2p);

	return (ERROR);
	}

    return (sd);
    }
/*******************************************************************************
*
* rresvport - open a socket with a privileged port bound to it
*
* This routine opens a socket with a privileged port bound to it.
* It is analogous to the UNIX routine rresvport().
*
* RETURNS
* A socket descriptor, or ERROR if either the socket cannot be opened or all
* ports are in use.
*
* SEE ALSO: UNIX BSD 4.3 manual entry for rresvport()
*/

int rresvport
    (
    FAST int *alport    /* port number to initially try */
    )
    {
    struct sockaddr_in sin;
    int sd;

    sin.sin_family 	= AF_INET;
    sin.sin_addr.s_addr = 0;
    sin.sin_port	= htons (*alport);

    if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == ERROR)
	return (ERROR);

    if (bindresvport (sd, &sin) < 0)
	{
	close (sd);
	return (ERROR);
	}
    *alport = ntohs (sin.sin_port);

    return (sd);
    }
/*******************************************************************************
*
* remCurIdGet - get the current user name and password
*
* This routine gets the user name and password currently used for remote 
* host access privileges and copies them to <user> and <passwd>.  Either
* parameter can be initialized to NULL, and the corresponding item will not
* be passed.
*
* RETURNS: N/A
*
* SEE ALSO: iam(), whoami()
*/

void remCurIdGet
    (
    char *user,         /* where to return current user name */
    char *passwd        /* where to return current password */
    )
    {
    if (user != NULL)
	strcpy (user, remUser);

    if (passwd != NULL)
	strcpy (passwd, remPasswd);
    }
/*******************************************************************************
*
* remCurIdSet - set the remote user name and password
*
* This routine specifies the user name that will have access privileges
* on the remote machine.  The user name must exist in the remote
* machine's \f3/etc/passwd\fP, and if it has been assigned a password,
* the password must be specified in <newPasswd>.
*
* Either parameter can be NULL, and the corresponding item will not be set.
*
* The maximum length of the user name and the password is MAX_IDENTITY_LEN
* (defined in remLib.h).
*
* NOTE: A more convenient version of this routine is iam(), which is intended
* to be used from the shell.
*
* RETURNS: OK, or ERROR if the name or password is too long.
*
* SEE ALSO: iam(), whoami()
*/

STATUS remCurIdSet
    (
    char *newUser,      /* user name to use on remote */
    char *newPasswd     /* password to use on remote (NULL = none) */
    )
    {
    if (((newUser != NULL) && (strlen (newUser) > MAX_IDENTITY_LEN-1)) ||
        ((newPasswd != NULL) && (strlen (newPasswd) > MAX_IDENTITY_LEN-1)))
	{
	errnoSet (S_remLib_IDENTITY_TOO_BIG);
	return (ERROR);
	}

    if (newUser == NULL)
	remUser[0] = EOS;
    else
	strcpy (remUser, newUser);

    if (newPasswd == NULL)
	remPasswd[0] = EOS;
    else
	strcpy (remPasswd, newPasswd);

    return (OK);
    }
/*******************************************************************************
*
* iam - set the remote user name and password
*
* This routine specifies the user name that will have access privileges
* on the remote machine.  The user name must exist in the remote
* machine's \f3/etc/passwd\fP, and if it has been assigned a password,
* the password must be specified in <newPasswd>.
*
* Either parameter can be NULL, and the corresponding item will not be set.
*
* The maximum length of the user name and the password is MAX_IDENTITY_LEN
* (defined in remLib.h).
*
* NOTE: This routine is a more convenient version of remCurIdSet() and is
* intended to be used from the shell.
*
* RETURNS: OK, or ERROR if the call fails.
*
* SEE ALSO: whoami(), remCurIdGet(), remCurIdSet()
*/

STATUS iam
    (
    char *newUser,      /* user name to use on remote */
    char *newPasswd     /* password to use on remote (NULL = none) */
    )
    {
    if (remCurIdSet (newUser, newPasswd) != OK)
	{
	printErr ("User name or password too long\n");
	return (ERROR);
	}

    return (OK);
    }
/*******************************************************************************
*
* whoami - display the current remote identity
*
* This routine displays the user name currently used for remote machine
* access.  The user name is set with iam() or remCurIdSet().
*
* RETURNS: N/A
*
* SEE ALSO: iam(), remCurIdGet(), remCurIdSet()
*/

void whoami (void)
    {
    char user [MAX_IDENTITY_LEN];

    remCurIdGet (user, (char *) NULL);
    printf ("%s\n", user);
    }

/*******************************************************************************
*
* bindresvport - bind a socket to a privileged IP port
*
* This routine picks a port number between 600 and 1023 that is not being
* used by any other programs and binds the socket passed as <sd> to that
* port.  Privileged IP ports (numbers between and including 0 and 1023) are
* reserved for privileged programs.
*
* RETURNS:
* OK, or ERROR if the address family specified in <sin> is not supported or
* the call fails.
*/

STATUS bindresvport
    (
    int                         sd,     /* socket to be bound */
    FAST struct sockaddr_in     *sin    /* socket address -- value/result */
    )
    {
    FAST int		startPort;
    FAST int		port;
    struct sockaddr_in 	myaddr;

    if (sin == (struct sockaddr_in *)0)
	{
	sin = &myaddr;
	bzero((char *) sin, sizeof (*sin));
	sin->sin_family = AF_INET;
	}
    else if (sin->sin_family != AF_INET)
	{
	errnoSet (EPFNOSUPPORT);
	return (ERROR);
	}

    if (ntohs (sin->sin_port) == 0)
	sin->sin_port = htons (remLastResvPort);

    port = startPort = ntohs (sin->sin_port);

    FOREVER
	{
	--port;

	if (port <= IPPORT_RESERVED - 400)
	    port = IPPORT_RESERVED - 1;

	if (port == startPort)
	    {
	    errnoSet (S_remLib_ALL_PORTS_IN_USE);
	    return (ERROR);
	    }

	sin->sin_port = htons (port);

	if (bind (sd, (struct sockaddr *) sin, sizeof (*sin)) != ERROR)
	    {
	    remLastResvPort = port;
	    return (OK);
	    }
	}
    }

/* ftpLib.c - File Transfer Protocol (FTP) library */

/* Copyright 1984 - 2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02q,26jun02,elr  Correction for passive mode STOR and RETR command in 
                     ftpXfer() (SPR #79108)
02p,06jun02,elr  Documentation (SPR #78008)
02o,23may02,elr  Added ftplPasvModeDisable flag
02n,22may02,elr  added PASV mode to ftpDataConnInit() (SPR #77169) to
                     correct long delays during frequent reboots
                 changed doc of ftpReplyGet() and 
                     ftpReplyGetEnhanced() (SPR #76317)
                 changed ftpLibDebugLevelSet() to ftpLibDebugOptionsSet()
02m,10may02,kbw  making man page edits
02l,22mar02,elr  Cleanup compiler warnings
02k,14mar02,elr  Define host response return codes (SPR #68838)
                 Host response FTP_PRELIM is ok during ftpXfer (SPR #71954)
                 Limit FTP_TRANSIENT response retries during ftpXfer() 
                     ftpPrelimConfig() (SPR #70907) (SPR #6259) (SPR #33119)
                 Added ftpLibDebugLevelSet() and added debugging messages 
                 Added documentation about stack memory allocation 
                    and tuning (SPR #64220)
                 Moved some error message to FTPLDEBUG  (SPR #71496)
                    and removed ftpErrorSuppress in favor ftplDebug
                 Added documentation concerning ftpHookup() and ftpDataConnInit()
                    socket connections (SPR #62289) (SPR #30556)
02j,15oct01,rae  merge from truestack ver 02n, base 02i (SPR #67644)
02i,22sep99,cno  change FD_ISSET control & data connection tests (SPR27234)
02h,15mar99,elg  change erroneous example code in ftpXfer() (SPR 9989).
02g,12mar99,p_m  Fixed SPR# 9022 by publishing ftpLs().
02f,05oct98,jmp  doc: cleanup.
02e,23jan98,spm  fixed ftpXfer to expect correct return codes for commands
                 which do not involve data transfer (SPR #20017)
02d,05jun95,jag  Changed ftpXfer to handle error 425 at boot time.
02c,30aug93,jag  Changed ftpCommand to issue a single write (SPR #2492)
02b,11aug93,jmm  Changed ioctl.h and socket.h to sys/ioctl.h and sys/socket.h
02a,23feb93,jdi  doc: changed ftpCommand() examples to fixed no. of args.
01z,20jan93,jdi  documentation cleanup for 5.1.
01y,20sep92,kdl  added ftpLs; moved ftpErrorSuppress to funcBind.c.
01x,11sep92,jmm  added ftpErrorSuppress for lsOld() (SPR #1257)
01w,19aug92,smb  Changed systime.h to sys/times.h.
01v,18jul92,smb  Changed errno.h to errnoLib.h.
01u,26may92,rrr  the tree shuffle
		  -changed includes to have absolute path from h/
01t,19nov91,rrr  shut up some ansi warnings.
01s,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed copyright notice
01r,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by dnw.
01q,12feb91,jaa	 documentation.
01p,02oct90,hjb  added a call to htons() where needed.
01o,07may90,hjb  added some documentation to ftpXfer routine to issue a "QUIT"
		 at the end of the file transfer session via FTP.
01n,22feb90,jdi  documentation cleanup.
01m,07jun89,gae  changed SOCKADDR back to "struct sockaddr".
01l,23sep88,gae  documentation touchup.
01k,30may88,dnw  changed to v4 names.
01j,28may88,dnw  changed fioStdErr call to STD_ERR.
01i,05apr88,gae  changed fprintf() call to fdprintf()
01h,17nov87,ecs  lint: added include of inetLib.h.
01g,11nov87,jlf  documentation
01f,06nov87,dnw  fixed bug in use of setsockopt().
01e,01nov87,llk  changed remInetAddr() to UNIX compatible inet_addr().
01d,01apr87,ecs  hushed lint in ftpGetReply.
		 changed "VARARGS 2" to "VARARGS2" in ftpCommand
		 removed extraneous 4th arg from calls to bind, socket, accept,
		    & connect.
01c,19mar87,dnw  documentation
		 prepended FTP_ to ftp reply codes.
01b,14feb87,dnw  changed to use getsockname() instead of using privileged port.
01a,07nov86,dnw  written
*/

/*
DESCRIPTION
This library provides facilities for transferring files to and from a host
via File Transfer Protocol (FTP).  This library implements only the
"client" side of the FTP facilities.

FTP IN VXWORKS
For most purposes, you should access the services of ftpLib by means of 
netDrv, a VxWorks I/O driver that supports transparent access to remote 
files by means of standard I/O system calls.  Before attempting to access 
ftpLib services directly, you should check whether netDrv already provides 
the same access for less trouble.

HIGH-LEVEL INTERFACE
The routines ftpXfer() and ftpReplyGet() provide the highest level of
direct interface to FTP.  The routine ftpXfer() connects to a specified
remote FTP server, logs in under a specified user name, and initiates a
specified data transfer command.  The routine ftpReplyGet() receives
control reply messages sent by the remote FTP server in response to the
commands sent.

LOW-LEVEL INTERFACE
The routines ftpHookup(), ftpLogin(), ftpDataConnInit(), ftpDataConnGet(),
ftpCommand(), ftpCommandEnhanced()  provide the primitives necessary to 
create and use control and data connections to remote FTP servers.  The 
following example shows how to use these low-level routines.  It implements 
roughly the same function as ftpXfer().

.CS
char *host, *user, *passwd, *acct, *dirname, *filename;
int ctrlSock = ERROR; /@ This is the control socket file descriptor @/
int dataSock = ERROR; /@ This is the data path socket file descriptor @/

if (((ctrlSock = ftpHookup (host)) == ERROR)		                      ||
    (ftpLogin (ctrlSock, user, passwd, acct) == ERROR)	                      ||
    (ftpCommand (ctrlSock, "TYPE I", 0, 0, 0, 0, 0, 0) != FTP_COMPLETE)       ||
    (ftpCommand (ctrlSock, "CWD %s", dirname, 0, 0, 0, 0, 0) != FTP_COMPLETE) ||
    ((dataSock = ftpDataConnInit (ctrlSock)) == ERROR)	                      ||
    (ftpCommand (ctrlSock, "RETR %s", filename, 0, 0, 0, 0, 0) != FTP_PRELIM) ||
    ((dataSock = ftpDataConnGet (dataSock)) == ERROR))
    {
    /@ an error occurred; close any open sockets and return @/

    if (ctrlSock != ERROR)
	close (ctrlSock);
    if (dataSock != ERROR)
	close (dataSock);
    return (ERROR);
    }
.CE

For even lower-level access,  please note that the sockets provided by 
ftpHookup() and ftpDataConnInit() are standard TCP/IP sockets.  Developers may 
implement read(), write() and select() calls using these sockets for maximum 
flexibility.

To use this feature, include the following component:
INCLUDE_FTP

TUNING FOR MULTIPLE FILE ACCESS: 
Please note that accessing multiple files simultaneously may require 
increasing the memory available to the network stack.  You can examine 
memory requirements by using netStackSysPoolShow() and netStackDataPoolShow()
before opening and after closing files.

You may need to modify the following macro definitions according to your 
specific memory requirements:

 NUM_64
 NUM_128
 NUM_256
 NUM_512
 NUM_1024
 NUM_2048
 NUM_SYS_64
 NUM_SYS_128
 NUM_SYS_256
 NUM_SYS_512
 NUM_SYS_1024
 NUM_SYS_2048

Please also note that each concurrent file access requires three file 
descriptors (File, Control and Socket).  The following macro definition may 
need modification per your application:
NUM_FILES

Developers are encouraged to enable the error reporting facility during 
debugging using the function ftpLibDebugOptionsSet().  The output is displayed 
via the logging facility.

INCLUDE FILES: ftpLib.h

SEE ALSO: netDrv, logLib
*/

#include "vxWorks.h"
#include "ctype.h"
#include "sys/socket.h"
#include "netinet/in.h"
#include "ftpLib.h"
#include "inetLib.h"
#include "sys/times.h"
#include "hostLib.h"
#include "sockLib.h"
#include "stdio.h"
#include "string.h"
#include "unistd.h"
#include "errnoLib.h"
#include "iosLib.h"
#include "remLib.h"
#include "selectLib.h"
#include "taskLib.h"
#include "vwModNum.h"
#include "private/ftpLibP.h"
#include "private/funcBindP.h"

typedef struct sockaddr_in SOCKADDR_IN;

#define UCA(n)     (((int)(((char *)&dataAddr.sin_addr)[n])) & 0xff)
#define UCP(n)     (((int)(((char *)&dataAddr.sin_port)[n])) & 0xff)

#define FTPLDEBUG(string, debugLevel, param1, param2, param3, param4, param5, param6)\
   {\
   if ((_func_logMsg != NULL) && (ftplDebug & debugLevel))\
      (* _func_logMsg) (string, param1, param2, param3, param4, param5, param6);\
   }

#define FTP_PORT	21

#define PASV_REPLY_STRING_LENGTH 256

/* XXX ftplPasvModeDisable - XXX this will be removed in the next release XXX */

BOOL ftplPasvModeDisable = FALSE;

LOCAL STATUS ftpPasvReplyParse (char *, UINT32 *, UINT32 *, UINT32 *,  \
                                UINT32 *, UINT32 *, UINT32 *);
LOCAL BOOL ftpTransientFatal (UINT32 reply);
LOCAL char pasvReplyString[PASV_REPLY_STRING_LENGTH];


UINT32  ftplTransientMaxRetryCount = 1;   /* Retry just once for now */
UINT32  ftplTransientRetryInterval = 0;   /* Default with no delay */
FUNCPTR _func_ftpTransientFatal    = ftpTransientFatal;

/*******************************************************************************
*
* ftpCommand - send an FTP command and get the reply 
*
* This command has been superceded by ftpCommandEnhanced() 
*
* This routine sends the specified command on the specified socket, which
* should be a control connection to a remote FTP server.
* The command is specified as a string in printf() format with up
* to six arguments.
*
* After the command is sent, ftpCommand() waits for the reply from the
* remote server.  The FTP reply code is returned in the same way as in
* ftpReplyGet().
*
* EXAMPLE
* .CS
* ftpCommand (ctrlSock, "TYPE I", 0, 0, 0, 0, 0, 0);     /@ image-type xfer @/
* ftpCommand (ctrlSock, "STOR %s", file, 0, 0, 0, 0, 0); /@ init file write @/
* .CE
*
* RETURNS:
*
*  1 = FTP_PRELIM (positive preliminary)
*  2 = FTP_COMPLETE (positive completion)
*  3 = FTP_CONTINUE (positive intermediate)
*  4 = FTP_TRANSIENT (transient negative completion)
*  5 = FTP_ERROR (permanent negative completion)
*
* ERROR if there is a read/write error or an unexpected EOF.
*
* SEE ALSO: ftpReplyGet()
*
* VARARGS2
*/

int ftpCommand
    (
    int ctrlSock,       /* fd of control connection socket */
    char *fmt,          /* format string of command to send */
    int arg1,           /* first of six args to format string */
    int arg2,
    int arg3,
    int arg4,
    int arg5,
    int arg6
    )
    {
        /* return most significant digit of the reply */
        return (ftpCommandEnhanced (ctrlSock, fmt, arg1, arg2, 
                                    arg3, arg4, arg5, arg6, NULL, 0) / 100);
    }

/*******************************************************************************
*
* ftpCommandEnhanced - send an FTP command and get the complete RFC reply code
*
* This command supercedes ftpCommand() 
*
* This routine sends the specified command on the specified socket, which
* should be a control connection to a remote FTP server.
* The command is specified as a string in printf() format with up
* to six arguments.
*
* After the command is sent, ftpCommand() waits for the reply from the
* remote server.  The FTP reply code is returned in the same way as in
* ftpReplyGetEnhanced().
*
* EXAMPLE
* .CS
* ftpCommandEnhanced (ctrlSock, "TYPE I", 0, 0, 0, 0, 0, 0, 0, 0);     /@ image-type xfer @/
* ftpCommandEnhanced (ctrlSock, "STOR %s", file, 0, 0, 0, 0, 0, 0, 0); /@ init file write @/
* ftpCommandEnhanced (ctrlSock, "PASV", file, 0, 0, 0, 0, 0, reply, rplyLen); /@ Get port @/
* .CE
*
* RETURNS:
*  The complete FTP response code (see RFC #959)
*
* ERROR if there is a read/write error or an unexpected EOF.
*
* SEE ALSO: ftpReplyGetEnhanced(), ftpReplyGet()
*
* VARARGS2
*/

int ftpCommandEnhanced
    (
    int ctrlSock,       /* fd of control connection socket */
    char *fmt,          /* format string of command to send */
    int arg1,           /* first of six args to format string */
    int arg2,
    int arg3,
    int arg4,
    int arg5,
    int arg6,
    char *replyString, /* storage for the last line of the server response or NULL */
    int replyStringLength  /* Maximum character length of the replyString */
    )
    {
    char buffer [128];
    int len;

    if (ftplDebug & FTPL_DEBUG_OUTGOING)
        {
        printErr ("---> ");
        printErr (fmt, arg1, arg2, arg3, arg4, arg5, arg6);
        printErr ("\n");
        }

    /* Format Command to send to FTP server */

    /* The next line will generate a warning with gcc 2.96+, this is O.K. */
    sprintf (buffer, fmt, arg1, arg2, arg3, arg4, arg5, arg6);

    len = strlen(buffer);

    /* Append CR LF to format copy to force a single write to TCP */

    sprintf(&buffer[len],"%s","\r\n");
    len = strlen(buffer);

    if (write(ctrlSock, buffer, len) < len)
        {
        FTPLDEBUG ("ftpCommandEnhanced: error during write to control socket - errno:0x%08x",  \
                   FTPL_DEBUG_ERRORS,errno,1,2,3,4,5);
        return(ERROR);
        }

    return (ftpReplyGetEnhanced (ctrlSock, 
                                 !strcmp (fmt, "QUIT"), 
                                 replyString, 
                                 replyStringLength));
    }

/*******************************************************************************
*
* ftpXfer - initiate a transfer via FTP
*
* This routine initiates a transfer via a remote FTP server
* in the following order:
* .IP (1) 4
* Establishes a connection to the FTP server on the specified host.
* .IP (2)
* Logs in with the specified user name, password, and account,
* as necessary for the particular host.
* .IP (3)
* Sets the transfer type to image by sending the command "TYPE I".
* .IP (4)
* Changes to the specified directory by sending
* the command "CWD <dirname>".
* .IP (5)
* Sends the specified transfer command
* with the specified filename as an argument, and establishes a data connection.
* Typical transfer commands are "STOR %s", to write to a remote file,
* or "RETR %s", to read a remote file.
* .LP
* The resulting control and data connection file descriptors are returned
* via <pCtrlSock> and <pDataSock>, respectively.
*
* After calling this routine, the data can be read or written to the remote
* server by reading or writing on the file descriptor returned in
* <pDataSock>.  When all incoming data has been read (as indicated by 
* an EOF when reading the data socket) and/or all outgoing data has been
* written, the data socket fd should be closed.  The routine ftpReplyGet()
* should then be called to receive the final reply on the control socket,
* after which the control socket should be closed.
*
* If the FTP command does not involve data transfer, <pDataSock> should be 
* NULL, in which case no data connection will be established. The only 
* FTP commands supported for this case are DELE, RMD, and MKD.
*
* EXAMPLE
* The following code fragment reads the file "/usr/fred/myfile" from the
* host "server", logged in as user "fred", with password "magic"
* and no account name.
*
* .CS
*     #include "vxWorks.h"
*     #include "ftpLib.h"
*
*     int	ctrlSock;
*     int	dataSock;
*     char	buf [512];
*     int	nBytes;
*     STATUS	status;
*
*     if (ftpXfer ("server", "fred", "magic", "",
*                  "RETR %s", "/usr/fred", "myfile",
*                  &ctrlSock, &dataSock) == ERROR)
*         return (ERROR);
*
*     while ((nBytes = read (dataSock, buf, sizeof (buf))) > 0)
*         {
*         ...
*         }
*
*     close (dataSock);
*
*     if (nBytes < 0)             /@ read error? @/
*         status = ERROR;
*
*     if (ftpReplyGet (ctrlSock, TRUE) != FTP_COMPLETE)
*         status = ERROR;
*
*     if (ftpCommand (ctrlSock, "QUIT", 0, 0, 0, 0, 0, 0) != FTP_COMPLETE)
*         status = ERROR;
*
*     close (ctrlSock);
* .CE
*
* RETURNS:
* OK, or ERROR if any socket cannot be created or if a connection cannot be
* made.
*
* SEE ALSO: ftpReplyGet()
*/

STATUS ftpXfer
    (
    char *host,         /* name of server host */
    char *user,         /* user name for host login */
    char *passwd,       /* password for host login */
    char *acct,         /* account for host login */
    char *cmd,          /* command to send to host */
    char *dirname,      /* directory to 'cd' to before sending command */
    char *filename,     /* filename to send with command */
    int *pCtrlSock,     /* where to return control socket fd */
    int *pDataSock      /* where to return data socket fd, */
                        /* (NULL == don't open data connection) */
    )
    {
    FAST int ctrlSock = ERROR;
    FAST int dataSock = ERROR;
    UINT32 ftpReply   = 0;
    UINT32 retryCount = 0;
    int    cmdResult;      
    struct fd_set readFds; /* Used by select for PORT method */
    int    width;          /* Used by select for PORT method */
    BOOL   dataSockPassive = TRUE;


    FTPLDEBUG ("ftpXfer: host:%s user:%s passwd:%s acct:%s cmd:%s dir:%s\n",  \
               FTPL_DEBUG_ERRORS,host,user,passwd,acct,cmd,dirname);

    if (((ctrlSock = ftpHookup (host)) == ERROR)    ||
        (ftpLogin (ctrlSock, user, passwd, acct) != OK)    ||
        (ftpCommand (ctrlSock, "TYPE I",0,0,0,0,0,0) != FTP_COMPLETE)  ||
        ((dirname[0] != EOS) && (ftpCommand (ctrlSock, "CWD %s", 
        (int)dirname,0,0,0,0,0) != FTP_COMPLETE)))
        {
        /* Detected an error during command establishment */
        FTPLDEBUG ("ftpXfer: Detected an error during command establishment errno:0x%08x",  \
                    FTPL_DEBUG_ERRORS,errno,1,2,3,4,5);
        close (ctrlSock);
        return (ERROR);
        }

    /*
     * This is a special when an FTP command does not need to establish a
     * data connection.
     */

    if (pDataSock == NULL)
        {
        if (ftpCommand (ctrlSock, cmd, (int)filename, 0, 0, 0, 0, 0) 
                        != FTP_COMPLETE)
            {
            /* FTP command error. */
            FTPLDEBUG ("ftpXfer: error during command.  errno:0x%08x",  \
            FTPL_DEBUG_ERRORS, errno,1,2,3,4,5);

            close (ctrlSock);
            return (ERROR);
            }
        }
       
    /* 
     * At this point we are trying to establish the data socket.
     * We will first try using the modern, client-initiated PASV command.   
     * If PASV fails,  then we will fall back to the PORT command. 
     */

    /*  Set up local data port and send the PORT command */

    do 
        {

        if ((dataSock = ftpDataConnInitPassiveMode (ctrlSock)) != ERROR)
            {
            FTPLDEBUG ("ftpXfer: notice - mode succeeded.\n ", \
                FTPL_DEBUG_ERRORS, 0, 1, 2, 3, 4, 5);
            dataSockPassive = TRUE; /* We do not need to listen() on the socket */
            }
        else
            {
            FTPLDEBUG ("ftpXfer: notice - PASV mode failed. Now trying older PORT connect method", \
                FTPL_DEBUG_ERRORS, 0, 1, 2, 3, 4, 5);

            if ((dataSock = ftpDataConnInit (ctrlSock)) == ERROR)
                {
                FTPLDEBUG ("ftpXfer:  ftpDataConnInit - error during trying of another port. errno:0x%08x",  \
                    FTPL_DEBUG_ERRORS, errno, 1, 2, 3, 4, 5);
                close (ctrlSock);
                return (ERROR);
                } 
            else
                dataSockPassive = FALSE; /* We will need to listen() on the socket */
            }

        /* Send the FTP command.  */

        cmdResult = ftpCommandEnhanced (ctrlSock, 
                                        cmd, 
                                        (int)filename,
                                        0, 0, 0, 0, 0, NULL, 0);

        if ((cmdResult/100) != FTP_PRELIM)
            {
            /* 
             * The command has failed.  Close the data socket and decode the error.
             */

            close (dataSock);

            /* Check if something really bad happened: File not found, etc. */

            if ((cmdResult/100) == FTP_ERROR)
                {
                FTPLDEBUG ("ftpXfer:  response 0x%08x - aborting transfer.\n", \
                    FTPL_DEBUG_ERRORS, cmdResult, 1, 2, 3, 4, 5);
                close (ctrlSock);
                return (ERROR);
                }

            if ((cmdResult/100) == FTP_TRANSIENT && _func_ftpTransientFatal != NULL)
                {
                FTPLDEBUG ("ftpXfer:  calling user-supplied applette to see if 0x%08x FTP_TRANSIENT is fatal for this command.\n", \
                    FTPL_DEBUG_ERRORS, cmdResult, 1, 2, 3, 4, 5);
                if ((* _func_ftpTransientFatal) (cmdResult) == TRUE)
                    {
                    FTPLDEBUG ("ftpXfer:  user-supplied applette says 0x%08x FTP_TRANSIENT  ** IS ** fatal for this command.\n", \
                    FTPL_DEBUG_ERRORS, cmdResult, 1, 2, 3, 4, 5);
                    close (ctrlSock);
                    errno = S_ftpLib_FATAL_TRANSIENT_RESPONSE;
                    return (ERROR);
                    }

                FTPLDEBUG ("ftpXfer:  user-supplied applette says 0x%08x is  ** NOT ** fatal for this command.\n", \
                    FTPL_DEBUG_ERRORS, cmdResult, 1, 2, 3, 4, 5);
                }

            if ((ftpReply = (cmdResult/100)) == FTP_TRANSIENT)
                {
                /*
                 * If the error was due to transient error (e.x. the data port
                 * was not available) retry the command 
                 * ftplTransientMaxRetryCount times.  
                 */
                if (retryCount < ftplTransientMaxRetryCount)
                    {
                    ++retryCount;
                    FTPLDEBUG ("ftpXfer: warning - reply was %d(decimal) - FTP_PRELIM - #%d attempt in %d ticks.\n", \
                                FTPL_DEBUG_ERRORS,cmdResult,retryCount,
                                ftplTransientRetryInterval, 5, 6, 7);
                    if (ftplTransientRetryInterval)
                        taskDelay (ftplTransientRetryInterval);
                    continue; /* try another port */
                }
                else
                {

                /* Too many retries,  close socket and return failure */
                FTPLDEBUG ("ftpXfer: error - reply was %d(decimal) - FTP_PRELIM - attempt limit (%d) exceeded.\n", \
                           FTPL_DEBUG_ERRORS,
                           cmdResult,
                           ftplTransientMaxRetryCount,
                           5, 6, 7, 8);
                close (ctrlSock);
                errno = S_ftpLib_TRANSIENT_RETRY_LIMIT_EXCEEDED;
                return (ERROR);
                }

                /* Exit for any other error */
                FTPLDEBUG ("ftpXfer: error - ftpCommand != FTP_PRELIM.  errno:0x%08x\n", \
                           FTPL_DEBUG_ERRORS, errno, 1, 2, 3, 4, 5);
                close (ctrlSock);
                return (ERROR);
                }
            }

        if ( dataSockPassive == FALSE)
            {
            /* At this point do a select on the data & control socket */

            FTPLDEBUG ("ftpXfer: notice - cmdResult:%d dataSock:%d ctrlSock:%d  errno:0x%08x\n", \
                FTPL_DEBUG_ERRORS, cmdResult, dataSock, ctrlSock, errno, 4, 5);

            FD_ZERO (&readFds);
            FD_SET  (ctrlSock, &readFds);
            FD_SET  (dataSock, &readFds);
            width = (dataSock > ctrlSock) ? dataSock : ctrlSock;
            width++;

            if (select (width, &readFds, NULL, NULL, NULL) == ERROR)
                {
                FTPLDEBUG ("ftpXfer: error - select()==ERROR. errno:0x%08x\n", \
                           FTPL_DEBUG_ERRORS,errno, 1, 2, 3, 4, 5);
                close (dataSock);
                close (ctrlSock);
                return (ERROR);
                }

            /* If the control socket is ready process it and take a decision,
             * try again or return error. If the data socket is ready call
             * ftpDataConnGet next.
             */

            if (FD_ISSET (ctrlSock, &readFds) && ! FD_ISSET (dataSock, &readFds))
                {
                close (dataSock);

                FTPLDEBUG ("ftpXfer: warning - control socket ready but data socket not ready\n", \
                           FTPL_DEBUG_ERRORS,0, 1, 2, 3, 4, 5);

                if ((ftpReply = ftpReplyGet (ctrlSock, FALSE)) == FTP_TRANSIENT)
                    continue; /* Try another port */

                /* Regardless of response close sockets */

                FTPLDEBUG ("ftpXfer:  error - sending QUIT command to host. errno:0x%08x\n", \
                            FTPL_DEBUG_ERRORS,errno,1,2,3,4,5);
                (void) ftpCommand (ctrlSock, "QUIT", 0, 0, 0, 0, 0, 0);
                close (ctrlSock);
                return (ERROR);
                }
            } /* PORT method requires checking for data socket connection */
        } while ( ftpReply == FTP_TRANSIENT); /* Try again, we might need a different port */

    /* If we used PASV mode,  then the socket is ready for use */

    if (!dataSockPassive)
        {
        /* 
         * We used the PORT method to establish a connection.
         * The data socket connection is configured. Wait for the FTP server to connect
         * to us.
         */

        if ((dataSock = ftpDataConnGet (dataSock)) == ERROR)
            {
            FTPLDEBUG ("ftpXfer: error - ftpDataConnGet()==ERROR. errno:0x%08x\n", \
                FTPL_DEBUG_ERRORS,errno, 1, 2, 3, 4, 5);
            close (ctrlSock);
            return (ERROR);
            }
        }

    /* Store the control and data sockets */

    if (pCtrlSock != NULL)
        *pCtrlSock = ctrlSock;

    if (pDataSock != NULL)
        *pDataSock = dataSock;

    return (OK);
    }

/*******************************************************************************
*
* ftpReplyGet - get an FTP command reply
*
* This routine has been superceded by ftpReplyGetEnhanced()
*
* This routine gets a command reply on the specified control socket.
*
* The three-digit reply code from the first line is saved and interpreted.
* The left-most digit of the reply code identifies the type of code
* (see RETURNS below).
*
* The caller's error status is always set to the complete three-digit reply code
* regardless of the actual reply value (see the manual entry for errnoGet()).
* If the reply code indicates an error, the entire reply
* is printed if the ftp error printing is enabled (see the manual
* entry for ftpLibDebugOptionsSet()).
*
* If an EOF is encountered on the specified control socket, but no EOF was
* expected (<expecteof> == FALSE), then ERROR is returned.
*
* RETURNS:
*  1 = FTP_PRELIM (positive preliminary)
*  2 = FTP_COMPLETE (positive completion)
*  3 = FTP_CONTINUE (positive intermediate)
*  4 = FTP_TRANSIENT (transient negative completion)
*  5 = FTP_ERROR (permanent negative completion)
*
* ERROR if there is a read/write error or an unexpected EOF.
*/

int ftpReplyGet
    (
    int ctrlSock,       /* control socket fd of FTP connection */
    BOOL expecteof      /* TRUE = EOF expected, FALSE = EOF is error */
    )
    {
    /* return most significant digit of reply */
    return (ftpReplyGetEnhanced (ctrlSock, expecteof, NULL, 0) / 100);
    }

/*******************************************************************************
*
* ftpReplyGetEnhanced - get an FTP command reply
*
* This routine supercedes ftpReplyGet()
*
* This routine gets a command reply on the specified control socket.
*
* The three-digit reply code from the first line is saved and interpreted.
* The left-most digit of the reply code identifies the type of code
* (see RETURNS below).
*
* The caller's error status is always set to the complete three-digit reply code
* (see the manual entry for errnoGet()).
* If the reply code indicates an error, the entire reply
* is printed if the ftp error printing is enabled (see the manual
* entry for ftpLibDebugOptionsSet()).
*
* The last line of text retrieved from the servers response is stored
*     in the location specified by replyString.   If replyString is NULL
*     the parameter is ignored.
*
* If an EOF is encountered on the specified control socket, but no EOF was
* expected (<expecteof> == FALSE), then ERROR is returned.
*
* RETURNS:
*  The complete FTP response code (see RFC #959)
*
* ERROR if there is a read/write error or an unexpected EOF.
*/

int ftpReplyGetEnhanced
    (
    int ctrlSock,       /* control socket fd of FTP connection */
    BOOL expecteof,     /* TRUE = EOF expected, FALSE = EOF is error */
    char *replyString,  /* Location to store text of reply, or NULL */
    int  stringLengthMax /* Maximum length of reply (not including NULL) */ 
    )
    {
    char c;
    FAST int codeType;
    FAST int code;
    FAST int dig;
    int continuation;
    int origCode;
    int stringIndex;
    BOOL eof;

    /* read all lines of a reply:
     *    do
     *	      while not eof and not eol
     *	          process char
     *    while not eof and not last line of reply
     */

    origCode = 0;
    codeType = 0;

    do
        {
        /* read all characters of a line */

        dig  = 0;
        code = 0;
        stringIndex = 0;
        continuation = FALSE;

        while (!(eof = (read (ctrlSock, &c, 1) == 0)) && (c != '\n'))
            {

            /* Store the reply */

            if (replyString != NULL)
                {
                if (stringIndex < stringLengthMax)
                    {
                    replyString[stringIndex] = c;
                    stringIndex++;
                    }
                }

            dig++;
            

            if (dig == 1)    /* char 1 is code type */
                codeType = c - '0';

            if (dig <= 3)    /* chars 1-3 are code */
                {
                if (!isdigit ((int)c))
                    code = -1;
                else
                if (code != -1)
                    code = code * 10 + (c - '0');
                }

            if (dig == 4)    /* char 4 is continuation marker */
                continuation = (c == '-');

            if ((c != '\r') &&
                (((ftplDebug & FTPL_DEBUG_INCOMING) && (dig > 4))  ||
                (ftplDebug  && (codeType == FTP_ERROR)))
               )
                {
                write (STD_ERR, &c, 1);
                }
            }

        /* terminate the reply string */

        if (replyString != NULL)
            replyString[stringIndex] = c;

        /* print newline if we've been printing this reply */

        if ((ftplDebug & FTPL_DEBUG_INCOMING)  || 
            ((codeType == FTP_ERROR) && ftplDebug))
            printErr ("\n");

        /* save the original reply code */

        if (origCode == 0)
            origCode = code;
        }

    /* while not eof and not last line of reply */
    while (!eof && !((dig >= 3) && (code == origCode) && !continuation));

    /* set status to entire reply code */
    errno = (M_ftpLib | origCode);

    /* return error if unexpected eof encountered */

    if (eof & !expecteof)
        {
        FTPLDEBUG ("ftpReplyGet:  error - experienced eof - errno:0x%08x\n", \
            FTPL_DEBUG_ERRORS,errno,1,2,3,4,5);
        return (ERROR);
        }
    else
        return (origCode);    /* Return the complete code */
    }

/*******************************************************************************
*
* ftpHookup - get a control connection to the FTP server on a specified host
*
* This routine establishes a control connection to the FTP server on the
* specified host.  This is the first step in interacting with a remote FTP
* server at the lowest level.  (For a higher-level interaction with a remote
* FTP server, see the manual entry for ftpXfer().)
*
* RETURNS:
* The file descriptor of the control socket, or ERROR if the Internet
* address or the host name is invalid, if a socket could not be created, or
* if a connection could not be made.
*
* SEE ALSO: ftpLogin(), ftpXfer()
*/

int ftpHookup
    (
    char *host          /* server host name or inet address */
    )
    {
    FAST int ctrlSock;
    FAST int inetAddr;
    SOCKADDR_IN ctrlAddr;

    if (((inetAddr = (int) inet_addr (host)) == ERROR) &&
        ((inetAddr = hostGetByName (host)) == ERROR))
        {
        return (ERROR);
        }

    /* make our control socket */

    ctrlSock = socket (AF_INET, SOCK_STREAM, 0);
    if (ctrlSock < 0)
        {
        FTPLDEBUG ("ftpHookup: error - failure to get socket. errno:0x%08x\n", \
            FTPL_DEBUG_ERRORS, errno,1,2,3,4,5);
        return (ERROR);
        }

    /* bind a name with no inet address and let system pick port;
     * this is just so we can find our socket address later */

    ctrlAddr.sin_family      = AF_INET;
    ctrlAddr.sin_addr.s_addr = INADDR_ANY;
    ctrlAddr.sin_port        = htons (0);
    if (bind (ctrlSock, (struct sockaddr *)&ctrlAddr, sizeof (ctrlAddr)) < 0)
        {
        FTPLDEBUG ("ftpHookup: error - failure to bind socket. errno:0x%08x\n", \
            FTPL_DEBUG_ERRORS, errno,1,2,3,4,5);
        close (ctrlSock);
        return (ERROR);
        }

    /* connect to other side */

    ctrlAddr.sin_addr.s_addr = inetAddr;
    ctrlAddr.sin_port        = htons (FTP_PORT);
    if (connect (ctrlSock, (struct sockaddr *)&ctrlAddr, sizeof (ctrlAddr)) < 0)
        { 
        FTPLDEBUG ("ftpHookup: error - failure to connect socket. errno:0x%08x\n", \
            FTPL_DEBUG_ERRORS, errno,1,2,3,4,5);
        close (ctrlSock);
        return (ERROR);
        }

    ftpReplyGet (ctrlSock, FALSE);    /* read startup message from server */

    return (ctrlSock);
    }

/*******************************************************************************
*
* ftpLogin - log in to a remote FTP server
*
* This routine logs in to a remote server with the specified user name,
* password, and account name, as required by the specific remote host.  This
* is typically the next step after calling ftpHookup() in interacting with a
* remote FTP server at the lowest level.  (For a higher-level interaction
* with a remote FTP server, see the manual entry for ftpXfer()).
*
* RETURNS:
* OK, or ERROR if the routine is unable to log in.
*
* SEE ALSO: ftpHookup(), ftpXfer()
*/

STATUS ftpLogin
    (
    FAST int ctrlSock,  /* fd of login control socket */
    char *user,         /* user name for host login */
    char *passwd,       /* password for host login */
    char *account       /* account for host login */
    )
    {
    FAST int n;

    n = ftpCommand (ctrlSock, "USER %s", (int)user, 0, 0, 0, 0, 0);

    if (n == FTP_CONTINUE)
        n = ftpCommand (ctrlSock, "PASS %s", (int)passwd, 0, 0, 0, 0, 0);

    if (n == FTP_CONTINUE)
        n = ftpCommand (ctrlSock, "ACCT %s", (int)account, 0, 0, 0, 0, 0);

    if (n != FTP_COMPLETE)
        {
        FTPLDEBUG ("ftpLogin: error - failure to get complete login. errno:0x%08x\n",\
                    FTPL_DEBUG_ERRORS,errno,1,2,3,4,5);
        return (ERROR);
        }

    return (OK);
    }

/*******************************************************************************
*
* ftpDataConnInitPassiveMode - initialize an FTP data connection using PASV mode
*
* This routine sets up the client side of a data connection for the
* specified control connection.  It issues a PASV command and attempts to connect
* to the host-specified port.  If the host responds that it can not process the
* PASV command (command not supported) or fails to recognize the command, it will 
* return ERROR.
*
* This routine must be called \f2before\fP the data-transfer command is sent;
* otherwise, the server's connect may fail.
*
* This routine is called after ftpHookup() and ftpLogin() to establish a
* connection with a remote FTP server a low level.  (For a
* higher-level interaction with a remote FTP server, see ftpXfer().)
*
* This function is preferred over ftpDataConnInit() because 
* the remote system must preserve old port connection pairs even if the target 
* system suffers from a reboot (2MSL). Using PASV we encourage the host's 
* selection of a fresh port.
*
* RETURNS: The file descriptor of the data socket created, or ERROR.
*
* SEE ALSO: ftpHookup(), ftpLogin(), ftpCommandEnhanced(), ftpXfer(), ftpConnInit()
*
*/
int ftpDataConnInitPassiveMode
    (
    int ctrlSock        /* fd of associated control socket */
    )
    {
    FAST int dataSock;
    int result;
    int len;
    int portMsb;
    int portLsb;
    int hostDataPort;
    SOCKADDR_IN ctrlAddr;
    SOCKADDR_IN dataAddr;

    /* If configured to disable PASV mode, then just return ERROR */

    if (ftplPasvModeDisable)
        return (ERROR);

    /* find out our inet address */

    len = sizeof (ctrlAddr);
    if (getsockname (ctrlSock, (struct sockaddr *)&ctrlAddr, &len) < 0)
        {
        FTPLDEBUG ("ftpDataConnInitPassiveMode:  getsockname() failure. errno:0x%08x", \
                   FTPL_DEBUG_ERRORS,errno,1,2,3,4,5);
        return (ERROR);
        }

    result = ftpCommandEnhanced (ctrlSock, "PASV", 
                                 0,0,0,0,0,0, /* XXX - These arguments not needed */
                                 pasvReplyString,
                                 PASV_REPLY_STRING_LENGTH-1);
    
    if (result == FTP_PASSIVE_REPLY)  /* The remote FTP server supports PASSIVE mode */
        {

        /* Parse the last line of the reply */

        ftpPasvReplyParse (pasvReplyString, 0, 0, 0, 0, &portMsb, &portLsb);

        /* Convert port number */

        hostDataPort = portMsb * 256 + portLsb;

        /* make our data socket */

        dataSock = socket (AF_INET, SOCK_STREAM, 0);
        if (dataSock < 0)
            {
            FTPLDEBUG ("ftpDataConnInitPassiveMode: error - failure to get socket. errno:0x%08x\n", \
                FTPL_DEBUG_ERRORS, errno,1,2,3,4,5);
            return (ERROR);
            }

        bzero ((char *) &dataAddr, sizeof (SOCKADDR));

        /* Use the port given to us in the reply of our PASV command */

        dataAddr.sin_port        = htons (hostDataPort); 
        dataAddr.sin_family      = AF_INET;

        len = sizeof (SOCKADDR_IN);
        if (getpeername (ctrlSock, (struct sockaddr *)&ctrlAddr, &len) < 0)
            {
            FTPLDEBUG ("ftpDataConnInitPassiveMode:  getpeername() failure. errno:0x%08x", \
                       FTPL_DEBUG_ERRORS,errno,1,2,3,4,5);
            close (dataSock);
            return (ERROR);
            }

        dataAddr.sin_addr.s_addr = ctrlAddr.sin_addr.s_addr; 

        /* connect to the host */

        if (connect (dataSock, (struct sockaddr *)&dataAddr, sizeof (dataAddr)) < 0)
            { 
            FTPLDEBUG ("ftpDataConnInitPassiveMode: failure to connect. sock:%d sockMsb:%d sockLsb:%d errno:0x%08x\n", \
                FTPL_DEBUG_ERRORS, hostDataPort, portMsb, portLsb, errno, 5, 6);
            close (dataSock);
            return (ERROR);
            }

        else 
            {
            FTPLDEBUG ("ftpDataConnInitPassiveMode: passive ftp connect established to host:%#x port:%d sock:%d\n", \
                FTPL_DEBUG_ERRORS, dataAddr.sin_addr.s_addr, hostDataPort, dataSock,4,5,6);
            return (dataSock);
            }

        }
    else /* We have failed PASV mode */
        {
        FTPLDEBUG ("ftpDataConnInitPassiveMode: host failed to respond correctly to PASV command. errno:0x%08x\n", \
            FTPL_DEBUG_ERRORS, errno,1,2,3,4,5);

        return (ERROR);
        }
    }

/*******************************************************************************
*
* ftpDataConnInit - initialize an FTP data connection using PORT mode
*
* This routine sets up the client side of a data connection for the
* specified control connection using the PORT command.  
* It creates the data port, informs the
* remote FTP server of the data port address, and listens
* on that data port.  The server will then connect to this data port
* in response to a subsequent data-transfer command sent on the
* control connection (see the manual entry for ftpCommand()).
*
* This routine must be called \f2before\fP the data-transfer command is sent;
* otherwise, the server's connect may fail.
*
* This routine is called after ftpHookup() and ftpLogin() to establish a
* connection with a remote FTP server at the lowest level.  (For a
* higher-level interaction with a remote FTP server, see ftpXfer().)
*
* Please note that ftpDataConnInitPassiveMode() is recommended instead
* of ftpDataConnInit().
*
* RETURNS: The file descriptor of the data socket created, or ERROR.
*
* SEE ALSO: ftpDataConnInitPassiveMode(), ftpHookup(), ftpLogin(), 
*           ftpCommand(), ftpXfer()
*/

int ftpDataConnInit
    (
    int ctrlSock        /* fd of associated control socket */
    )
    {
    FAST int dataSock;
    int result;
    int len;
    int optval;
    SOCKADDR_IN ctrlAddr;
    SOCKADDR_IN dataAddr;

    /* find out our inet address */

    len = sizeof (ctrlAddr);
    if (getsockname (ctrlSock, (struct sockaddr *)&ctrlAddr, &len) < 0)
        {
        FTPLDEBUG ("ftpDataConnInit:  getsockname() failure. errno:0x%08x", \
                   FTPL_DEBUG_ERRORS,errno,1,2,3,4,5);
        return (ERROR);
        }

    /* first try - try to send port */

    dataSock = socket (AF_INET, SOCK_STREAM, 0);
    if (dataSock < 0)
        {
        FTPLDEBUG ("ftpDataConnInit:  socket() failure. errno:0x%08x", \
                   FTPL_DEBUG_ERRORS,errno,1,2,3,4,5);
        return (ERROR);
        }

    dataAddr = ctrlAddr;    /* set our inet address */
    dataAddr.sin_port = htons (0);    /* let system pick port num */
    if (bind (dataSock, (struct sockaddr *)&dataAddr, sizeof (dataAddr)) < 0)
        {
        FTPLDEBUG ("ftpDataConnInit:  bind() failure. errno:0x%08x", \
                    FTPL_DEBUG_ERRORS,errno,1,2,3,4,5);
        close (dataSock);
        return (ERROR);
        }

    if (listen (dataSock, 1) < 0)
        {
        FTPLDEBUG ("ftpDataConnInit:  listen() failure. errno:0x%08x", \
                   FTPL_DEBUG_ERRORS,errno,1,2,3,4,5);
        close (dataSock);
        return (ERROR);
        }

    /* try to send socket address to other side */

    len = sizeof (dataAddr);
    if (getsockname (dataSock, (struct sockaddr *)&dataAddr, &len) < 0)
        {
        FTPLDEBUG ("ftpDataConnInit:  getsockname() failure. errno:0x%08x", \
                   FTPL_DEBUG_ERRORS,errno,1,2,3,4,5);
        close (dataSock);
        return (ERROR);
        }

#define UCA(n)     (((int)(((char *)&dataAddr.sin_addr)[n])) & 0xff)
#define UCP(n)     (((int)(((char *)&dataAddr.sin_port)[n])) & 0xff)

    result = ftpCommand (ctrlSock, "PORT %d,%d,%d,%d,%d,%d",
                         UCA(0), UCA(1), UCA(2), UCA(3), UCP(0), UCP(1));

    if (result != FTP_ERROR)
        {
        if (result == FTP_PRELIM)
            {
            FTPLDEBUG ("ftpDataConnInit:warning got FTP_PRELIM. ", \
                       FTPL_DEBUG_ERRORS,0,1,2,3,4,5);
            }

        if (result != FTP_COMPLETE && result != FTP_PRELIM)
            {
            FTPLDEBUG ("ftpDataConnInit: error - reply was %d(decimal) - not FTP_COMPLETE or FTP_PRELIM.\n",\
                       FTPL_DEBUG_ERRORS,result,1,2,3,4,5);
            close (dataSock);
            return (ERROR);
            }
        else
            {
            return (dataSock);
            }
        }

    /* second try - try to get port # correct by default */

    close (dataSock);

    dataSock = socket (AF_INET, SOCK_STREAM, 0);
    if (dataSock < 0)
        {
        FTPLDEBUG ("ftpDataConnInit: error - 2nd try to get port number failed on socket(). errno:0x%08x\n",\
                    FTPL_DEBUG_ERRORS,errno,1,2,3,4,5);
        return (ERROR);
        }

    optval = 1;
    if ((setsockopt (dataSock, SOL_SOCKET, SO_REUSEADDR,
            (caddr_t) &optval, sizeof (optval)) < 0) ||
            (bind (dataSock, (struct sockaddr *)&ctrlAddr, sizeof (ctrlAddr)) < 0))
        {
        FTPLDEBUG ("ftpDataConnInit: error -  bind failed on 2nd attempt  errno :0x%08x\n", \
                    FTPL_DEBUG_ERRORS, errno,1,2,3,4,5);
        close (dataSock);
        return (ERROR);
        }

    if (listen (dataSock, 1) < 0)
        {
        FTPLDEBUG ("ftpDataConnInit: error -  listen failed on 2nd attempt  errno :0x%08x\n", \
                    FTPL_DEBUG_ERRORS,errno,1,2,3,4,5);
        close (dataSock);
        return (ERROR);
        }

    return (dataSock);
    }

/*******************************************************************************
*
* ftpDataConnGet - get a completed FTP data connection
*
* This routine completes a data connection initiated by a call to
* ftpDataConnInit().  It waits for a connection on the specified socket from
* the remote FTP server.  The specified socket should be the one returned by
* ftpDataConnInit().  The connection is established on a new socket, whose
* file descriptor is returned as the result of this function.  The original 
* socket, specified in the argument to this routine, is closed.
*
* Usually this routine is called after ftpDataConnInit() and ftpCommand() to
* initiate a data transfer from/to the remote FTP server.
*
* RETURNS:
* The file descriptor of the new data socket, or ERROR if the connection
* failed.
*
* SEE ALSO: ftpDataConnInit(), ftpCommand()
*/

int ftpDataConnGet
    (
    int dataSock        /* fd of data socket on which to await connection */
    )
    {
    int newDataSock;
    SOCKADDR_IN from;
    int fromlen = sizeof (from);

    newDataSock = accept (dataSock, (struct sockaddr *) &from, &fromlen);

    close (dataSock);

    return (newDataSock);
    }

/*******************************************************************************
*
* ftpLs - list directory contents via FTP
*
* This routine lists the contents of a directory.  The content list
* is obtained via an NLST FTP transaction.
*
* The local device name must be the same as the remote host name
* with a colon ":" as a suffix.  (For example "wrs:" is the device
* name for the "wrs" host.)
*
* RETURNS : OK, or ERROR if could not open directory.
*/

STATUS ftpLs
    (
    char * dirName /* name of directory to list */
    )
    {
    DEV_HDR * pDevHdr;
    char    fullFileName [MAX_FILENAME_LENGTH];
    char    hostName [MAXHOSTNAMELEN];
    int     hostLength;
    char    usr [MAX_IDENTITY_LEN];
    char    passwd [MAX_IDENTITY_LEN];
    int     dataSock;
    int     cntrlSock;
    char    buffer [BUFSIZ];
    int     nChars;


    /* Get device header and complete filename */

    ioFullFileNameGet (dirName, &pDevHdr, fullFileName);

    /* Get the host name, and remove the trailing ":" */

    strcpy (hostName, pDevHdr->name);
    hostLength = strlen (hostName);
    hostName [hostLength - 1] = EOS;

    /* Get user ID information */

    remCurIdGet (usr, passwd);

    if (ftpXfer (hostName, usr, passwd, "", "NLST", fullFileName, "",
        &cntrlSock, &dataSock) != OK)
        {
            FTPLDEBUG ("Can't open directory \"%s\"\n", 
               FTPL_DEBUG_ERRORS,dirName,1,2,3,4,5);
            return (ERROR);
        }

    /* Write out the listing */

    while ((nChars = read (dataSock, (char *) buffer, BUFSIZ)) > 0)
        write (STD_OUT, (char *) buffer, nChars);
    
    /* Close the sockets opened by ftpXfer */

    close (cntrlSock);
    close (dataSock);

    return (OK);
    }

/*******************************************************************************
*
* ftpLibDebugOptionSet - set the debug level of the ftp library routines
*
* This routine enables the debugging of ftp transactions using the ftp library.
*
* \ts
* Debugging Level     | Meaning
* ----------------------------------------------------------------
* FTPL_DEBUG_OFF      | No debugging messages. 
* FTPL_DEBUG_INCOMING | Display all incoming responses. 
* FTPL_DEBUG_OUTGOING | Display all outgoing commands. 
* FTPL_DEBUG_ERRORS   | Display warnings and errors
* \te
* EXAMPLE
* .CS
* ftpLibDebugOptionsSet (FTPL_DEBUG_ERRORS);    /@ Display any runtime errors @/
* ftpLibDebugOptionsSet (FTPL_DEBUG_OUTGOING);  /@ Display outgoing commands @/
* ftpLibDebugOptionsSet (FTPL_DEBUG_INCOMING);  /@ Display incoming replies @/
* ftpLibDebugOptionsSet (FTPL_DEBUG_INCOMING |  /@ Display both commands and @/ 
*                        FTPL_DEBUG_OUTGOING);  /@         replies @/
* .CE
*
*/

void ftpLibDebugOptionsSet
    (
    UINT32 debugLevel 
    )
    {
        ftplDebug = debugLevel;
    }

/*******************************************************************************
*
* ftpTransientConfigSet - set parameters for host FTP_TRANSIENT responses 
*
* This routine adjusts the delay between retries in response to receiving 
* FTP_PRELIM and the maximum retry count permitted before failing.
*
* RETURNS : OK
*/

STATUS ftpTransientConfigSet
    (
    UINT32 maxRetryCount, /* The maximum number of attempts to retry */
    UINT32 retryInterval  /* time (in system clock ticks) between retries */
    )
    {

    /* Set the values */

    ftplTransientMaxRetryCount = maxRetryCount;
    ftplTransientRetryInterval = retryInterval;

    return (OK);

    }

/*******************************************************************************
*
* ftpTransientConfigGet - get parameters for host FTP_TRANSIENT responses 
*
* This routine retrieves the delay between retries in response to receiving 
* FTP_TRANSIENT and the maximum retry count permitted before failing.
*
* RETURNS : OK
*
* SEE ALSO : ftpTransientConfigSet, tickLib
*/

STATUS ftpTransientConfigGet
    (
    UINT32 *maxRetryCount, /* The maximum number of attempts to retry */
    UINT32 *retryInterval  /* time (in system clock ticks) between retries */
    )
    {

    /* return the values */

    if (maxRetryCount != NULL)
        *maxRetryCount = ftplTransientMaxRetryCount;
    if (retryInterval != NULL)
       *retryInterval = ftplTransientRetryInterval;

    return (OK);

    }

/*******************************************************************************
*
* ftpTransientFatal - applette to terminate FTP transient host responses
*
* ftpXfer will normally retry a command if the host responds with a 4xx
* reply.   If this applette is installed,  it can immediately terminate
* the retry sequence.
*
*
* RETURNS 
*
* TRUE, terminate retry attempts; FALSE, continue retry attempts.
*
* INTERNAL 
* 
* This is the default routine if the customer does not install
* an applette.
*
* SEE ALSO : ftpTransientFatalInstall(), ftpTransientConfigSet()
*
*/

LOCAL BOOL ftpTransientFatal 
    (
    UINT32 reply /* Three digit code defined in RFC #959 */
    )
    {
    switch (reply)
        {
        case (421): /* Service not available */
        case (450): /* File unavailable */
        case (451): /* error in processing */
        case (452): /* insufficient storage */
            { 
            /* yes, these are actually non-recoverable replies */
            return (TRUE); 
            break;
            }
            /* attempt to retry the last command */
        default:
        return (FALSE); 
        }
    }

/*******************************************************************************
*
* ftpTransientFatalInstall - set applette to stop FTP transient host responses
*
* The routine installs a function which will determine if a transient response 
* should be fatal.
* Some ftp servers incorrectly use 'transient' responses instead of 
* 'error' to describe conditions such as 'disk full'.
*
* RETURNS 
* 
* OK if the installation is successful, or ERROR if the installation fails.
*
* SEE ALSO 
*
* ftpTransientConfigSet(), ftpTransientFatal() in 
* target/config/comps/src/net/usrFtp.c
*
*/

STATUS ftpTransientFatalInstall 
    (
    FUNCPTR pApplette  /* function that returns TRUE or FALSE */
    )
    {

    /* At least prevent this from being a disaster */
    if (pApplette == NULL)
        return (ERROR); 

    _func_ftpTransientFatal = pApplette;

    return (OK); /* attempt to retry the last command */
    }

/*******************************************************************************
*
* ftpPasvReplyParse - Parse the reply of a PASV command
*
* ftpPasvReplyParse expects to receive a string generated as a response from
* a PASV command.   The string will begin with '227' and end with a series
* of six integer values encapsulated in parentheses and separated by commas.
* EXAMPLE STRING:
*
* 227 Entering passive mode (147,11,1,23,1027,1028)
*              
* RETURNS : OK - Sucessfull parse
*           ERROR - Error in parse
*
* SEE ALSO : ftpCommandEnhanced(), ftpReplyGetEnhanced()
*
* NOMANUAL
*/
LOCAL STATUS ftpPasvReplyParse 
    (
    char *responseString, /* NULL terminated string */
    UINT32 *argument1,    /* First argument */
    UINT32 *argument2,    /* Second argument */
    UINT32 *argument3,    /* Third argument */
    UINT32 *argument4,    /* Fourth argument */
    UINT32 *argument5,    /* Fifth argument */
    UINT32 *argument6     /* Six argument */
    )
    {
    char *index;
    UINT32 tmpArg1;
    UINT32 tmpArg2;
    UINT32 tmpArg3;
    UINT32 tmpArg4;
    UINT32 tmpArg5;
    UINT32 tmpArg6;

    if (responseString == NULL)
        return (ERROR);

    /* Sanity check: Check for '227' at the beginning of the reply */
    if (strstr (responseString, "227") == NULL)
        {
        FTPLDEBUG ("ftpPasvReplyParse: error - '227' not part of PASV response\n", \
            FTPL_DEBUG_ERRORS, 0,1,2,3,4,5);
        return (ERROR);
        }
 
    index = strstr(responseString, "(");

    if (index == NULL)
        {
        FTPLDEBUG ("ftpPasvReplyParse: error - '(' not part of PASV response\n", \
            FTPL_DEBUG_ERRORS, 0,1,2,3,4,5);
        return (ERROR);
        }

    /* scan in the arguments */

    sscanf (index+1, "%d,%d,%d,%d,%d,%d", 
            &tmpArg1,
            &tmpArg2,
            &tmpArg3,
            &tmpArg4,
            &tmpArg5,
            &tmpArg6);

    /* Store arguments as neccesary */

    if (argument1)
        *argument1 = tmpArg1;

    if (argument2)
        *argument2 = tmpArg2;

    if (argument3)
        *argument3 = tmpArg3;

    if (argument4)
        *argument4 = tmpArg4;

    if (argument5)
        *argument5 = tmpArg5;

    if (argument6)
        *argument6 = tmpArg6;

    return OK;
    }

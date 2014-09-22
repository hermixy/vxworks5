/* shellLib.c - shell execution routines */

/* Copyright 1987-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
04a,30apr02,elr  Isolate logRtn from shell's stdin, stdout for security
           +fmk  SPR 74345 - fix exit to work properly during telnet/rlogin
                 session. Added shellIsRemoteConnectedSet() and 
                 shellIsRemoteConnectedGet() so that shell can be notified of
                 remote connection and give connection status.
03z,16oct01,jn   use symFindSymbol for symbol lookup (SPR #7453)
03y,08oct01,fmk  increase size of errnoString in get1Redir() SPR 27859 and get
                 rid of compiler warnings
03x,07jul99,cno  Correct "syntax error" message (SPR24924)
03w,14mar99,jdi  doc: removed refs to config.h and/or configAll.h (SPR 25663).
03v,16jun98,fle  Remove trailing white space from command (^M), merged from
                 jimmyb's fix : SPR # 21507
03u,10oct95,jdi  doc: added .tG Shell to shellHistory() and shellPromptSet().
03t,05may93,caf  tweaked for ansi.
03s,05sep93,jcf  made calls to remCurId[SG]et indirect so network is decoupled.
03r,12jul93,jmm  removed VX_STDIO option from shellTaskOptions (spr 2041)
03q,20jan93,jdi  documentation cleanup for 5.1.
03p,02aug92,jcf  changed shellSigHandler to use sc_pregs for trcStack basis.
03o,29jul92,jwt  restored SPARC stack trace - added code to trcStack().
03n,28jul92,jwt  restored 02y modification removed in 03i; 
                 fixed version numbers.
03m,20jul92,smb  fixed shell hanging due to errno macro.
03l,09jul92,rrr  changed xsignal.h to private/sigLibP.h
03k,26may92,rrr  the tree shuffle
03j,30apr92,rrr  some preparation for posix signals.
03i,30mar92,yao  removed if condtionals to calls to trcStack() .
03h,12mar92,yao  correctly casted paramaters for excJobAdd().  changed
		 copyright notice.
03g,06jan92,yao  removed if conditional to call excInfoShow() for I960.
03f,17dec91,rrr  Removed decl for trcLib so it would compile.
03e,13dec91,gae  ANSI cleanup.
03d,14nov91,jpb  fixed problem with logout trashing user name.
		 save initial username and password and restore these
		 upon logout.  see spr 916 and 1100.
03c,15oct91,yao  fixed bug introduced in 03a for shell script.
03b,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -fixed #else and #endif
		  -changed READ, WRITE and UPDATE to O_RDONLY O_WRONLY and ...
		  -changed VOID to void
		  -changed copyright notice
03a,29aug91,yao  made the shell work with stdin directed to "/null"
		 or uninitialized.
02z,01aug91,yao  fixed to pass 6 args to excJobAdd() call.
02y,18feb91,jwt  removed SPARC signal handler attempt to trace own stack.
02x,29apr91,hdn  added defines and macros for TRON architecture.
02w,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 made shellLogout NOMANUAL; doc review by gae.
02v,25mar91,del  added I960 functionality to signal handler.
02u,13mar91,jaa	 documentation cleanup.
02t,05oct90,dnw  made shellRestart() and shellLoginInstall() be NOMANUAL.
02s,10aug90,dnw  added include of shellLib.h.
02r,01aug90,jcf  added include of sigLib.h.
02q,13jul90,dnw  added print of errno when redirection open/creat fails
		 added errnoStringGet()
02p,24may90,jcf  made execute externally visible.
		 fixed ledId sign comparison in execShell.
02o,18apr90,shl  added shell security.
02n,14apr90,jcf  remove tcb extension dependencies.
	  	 changed shell name to tShell.
02m,10nov89,dab  made ledId global.
02l,01sep88,ecs  mangled for SPARC; bumped shellTaskStackSize from 5k.
02k,23may89,dnw  added VX_FP_TASK to shell task options.
02j,16nov88,gae  made shell be locked when not interactive, i.e. doing scripts.
		 made shell history size be a global variable.
02i,27oct88,jcf  added taskSuspend (0) to shellSigHandler so sigHandlerCleanup
		   is never run on a deleted task.
02h,23sep88,gae  documentation.
02g,06jun88,dnw  changed taskSpawn/taskCreate args.
02f,30may88,dnw  changed to v4 names.
02e,29may88,dnw  added VX_UNBREAKABLE to shell task options.
02d,28may88,dnw  renamed h to shellHistory and logout to shellLogout
		   (h and logout moved to usrLib).
		 removed call to dbgSetTaskBreakable() since shell is spawned
		   unbreakable.
		 replace shellSetOrig...() with shellOrigStdSet().
		 changed call to create() to creat().
02c,26may88,gae  added signal handling to shell task.
02b,02apr88,gae  made it work with I/O system changes.
		 added VX_STDIO option to shell.
		 made shell complain when output can't be closed properly.
02a,27jan88,jcf  made kernel independent.
		   removed unnecessary include of a_out.h
01e,21nov87,gae  fixed double prompt bug with scripts.
01d,02nov87,dnw  moved logout() and shellLogoutInstall() here from usrLib.
	   +gae  changed "korn" references to "led".
01c,24oct87,gae  changed name from shellExec.c.
		 changed setOrig{In,Out,Err}Fd() to shellSetOrig{In,Out,Err}(),
		   setShellPrompt() to shellSetPrompt(), and abortScript() to
		   shellAbortScript().  Added h() and shellLock().
01b,21oct87,gae  added shellInit().
01a,06sep87,gae  split from shell.yacc.
		 made shell restart itself on end of file.
*/

/*
DESCRIPTION
This library contains the execution support routines for the VxWorks shell.
It provides the basic programmer's interface to VxWorks.
It is a C-expression interpreter, containing no built-in commands.

The nature, use, and syntax of the shell are more fully described in
the "Target Shell" chapter of the
.I "VxWorks Programmer's Guide."

INCLUDE FILES: shellLib.h

SEE ALSO: ledLib,
.pG "Target Shell"
*/

#include "vxWorks.h"
#include "shellLib.h"
#include "ctype.h"
#include "ioLib.h"
#include "memLib.h"
#include "string.h"
#include "symLib.h"
#include "sysSymTbl.h"
#include "private/sigLibP.h"
#include "taskLib.h"
#include "remLib.h"
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "ledLib.h"
#include "fioLib.h"
#include "trcLib.h"
#include "errno.h"
#include "private/funcBindP.h"

IMPORT void yystart ();
IMPORT int yyparse ();

#define MAX_PROMPT_LEN	80
#define MAX_SHELL_LINE	128	/* max chars on line typed to shell */

/* global variables */

int ledId;		/* Line EDitor descriptor (-1 = don't use) */

/* shell task parameters */

int shellTaskId;
char *shellTaskName	= "tShell";
int shellTaskPriority	= 1;
int shellTaskOptions	= VX_FP_TASK | VX_UNBREAKABLE;
int shellTaskStackSize	= 20000;	/* default/previous stack size */
int shellHistSize	= 20;		/* default shell history size */

int redirInFd;		/* fd of input redirection stream */
int redirOutFd;		/* fd of output redirection stream */

/* local variables */

LOCAL char promptString [MAX_PROMPT_LEN] = "-> ";
LOCAL BOOL shellAbort;		   /* TRUE = someone requested shell to abort */
LOCAL BOOL shellLocked;		   /* TRUE = shell is in exclusive use */
LOCAL BOOL shellExecuting;	   /* TRUE = shell was already executing */
LOCAL int origFd [3];		   /* fd of original interactive streams */
LOCAL FUNCPTR loginRtn;            /* network login routine */
LOCAL int loginRtnVar;             /* network login routine variable */
LOCAL FUNCPTR logoutRtn;	   /* network logout routine */
LOCAL int logoutVar;		   /* network logout variable */
LOCAL BOOL startRemoteSession;     /* TRUE = non-console, run security check */
LOCAL BOOL shellIsRemoteConnected; /* TRUE = connected to remote session */

LOCAL char *redirErrMsgWithString	= "  errno = %#x (%s)\n";
LOCAL char *redirErrMsg			= "  errno = %#x\n";

LOCAL char originalUser [MAX_IDENTITY_LEN];     /* original current user and */
LOCAL char originalPasswd [MAX_IDENTITY_LEN];   /* password before any rlogin */

/* forward declarations */

void shell ();


/* forward static functions */

static void shellSigHandler
    (
    int			signal,
    int			code,
    struct sigcontext *	pContext
    );

static STATUS execShell
    (
    BOOL	interactive
    );

static STATUS getRedir
    (
    char *	line,
    int *	pInFd,
    int *	pOutFd
    );

static STATUS get1Redir
    (
    char *	line,
    int *	pInFd,
    int *	pOutFd
    );

static STATUS errnoStringGet
    (
    int		errnum,
    char *	errnoString
    );

static void stringTrimRight		/* string a string right */
    (
    char *	strToTrim		/* string to trim right  */
    );


/*******************************************************************************
*
* shellInit - start the shell
*
* This routine starts the shell task.  If the configuration macro INCLUDE_SHELL
* is defined, shellInit() is called by the root task, usrRoot(), in usrConfig.c.
*
* RETURNS: OK or ERROR.
*
* SEE ALSO:
* .pG "Target Shell"
*/

STATUS shellInit
    (
    int stackSize,      /* shell stack (0 = previous/default value) */
    int arg             /* argument to shell task */
    )
    {
    if (taskNameToId (shellTaskName) != ERROR)
	return (ERROR);		/* shell task already active */

    if (stackSize != 0)
	shellTaskStackSize = stackSize;


    shellTaskId = taskSpawn (shellTaskName, shellTaskPriority,
			     shellTaskOptions, shellTaskStackSize,
			     (FUNCPTR) shell, arg, 0, 0, 0, 0 ,0 ,0, 0, 0, 0);

    startRemoteSession = FALSE; /* this is the console, not telnet or rlogin */

    return ((shellTaskId == ERROR) ? ERROR : OK);
    }
/*******************************************************************************
*
* shellRestart - restart the shell task
*
* Used by shellSigHandler; called by excTask().
*
* RETURNS: N/A.
*
* NOMANUAL
*/

void shellRestart
    (
    BOOL remoteSession       /* TRUE = non-console, keep shell secure */
    )
    {
    if (remoteSession)
	{
	startRemoteSession = TRUE;
        }

    if ((taskRestart (shellTaskId)) != ERROR)
        {
	printErr ("shell restarted.\n");
        }
    else
	{
	printErr ("spawning new shell.\n");
	if (shellInit (0, TRUE) == ERROR)
	    printErr ("can't restart shell.\n");
	}
    }

/*******************************************************************************
*
* shellSigHandler - general signal handler for shell task
*
* All signals to the shell task are caught by this handler.
* Any exception info is printed, then the shell's stack is traced.
* Finally the shell is restarted.
*
* RETURNS: N/A
*/

LOCAL void shellSigHandler
    (
    int signal,
    int code,
    struct sigcontext *pContext
    )
    {
    extern void dbgPrintCall ();

    /* print any valid exception info */

    if (_func_excInfoShow != NULL)			/* default show rtn? */
	(*_func_excInfoShow) (&taskIdCurrent->excInfo, FALSE);

    printErr ("\007\n");		/* ring bell */

    trcStack (pContext->sc_pregs, (FUNCPTR) dbgPrintCall, (int) taskIdCurrent);

    excJobAdd ((VOIDFUNCPTR) shellRestart, FALSE, 0, 0, 0, 0, 0);
    taskSuspend (0);			/* wait until excTask restarts us */
    }
/*******************************************************************************
*
* shell - the shell entry point
*
* This routine is the shell task.  It is started with a single parameter
* that indicates whether this is an interactive shell to be used from a
* terminal or a socket, or a shell that executes a script.
*
* Normally, the shell is spawned in interactive mode by the root task,
* usrRoot(), when VxWorks starts up.  After that, shell() is called only
* to execute scripts, or when the shell is restarted after an abort.
*
* The shell gets its input from standard input and sends output to standard
* output.  Both standard input and standard output are initially assigned
* to the console, but are redirected by telnetdTask() and rlogindTask().
*
* The shell is not reentrant, since \f3yacc\fP does not generate a
* reentrant parser.  Therefore, there can be only a single shell executing
* at one time.
*
* RETURNS: N/A
*
* SEE ALSO:
* .pG "Target Shell"
*/

void shell
    (
    BOOL interactive    /* should be TRUE, except for a script */
    )
    {
    int ix;
    struct sigaction sv;

    /* setup default signal handler */

    sv.sa_handler = (VOIDFUNCPTR) shellSigHandler;
    sv.sa_mask    = 0;
    sv.sa_flags   = 0;

    for (ix = 1; ix < 32; ix++)
	sigaction (ix, &sv, (struct sigaction *) NULL);

    /* test for new call of shell or restart of aborted shell */

    if (!shellExecuting)
	{
	/* legitimate call of shell; if interactive save in/out/err fd's */

	if (interactive)
	    {
	    origFd [STD_IN]  = ioGlobalStdGet (STD_IN);
	    origFd [STD_OUT] = ioGlobalStdGet (STD_OUT);
	    origFd [STD_ERR] = ioGlobalStdGet (STD_ERR); 
	    shellLocked = FALSE;

	    /*
	     * Install Line EDitor interface:
	     * This is done only after the first "interactive"
	     * invocation of the shell so that a startup
	     * script could change some parameters.
	     * Eg. ledId to -1 or shellHistSize to > 20.
	     */

	    if (ledId == (int)NULL &&
		(ledId = ledOpen (STD_IN, STD_OUT, shellHistSize)) == ERROR)
		{
		printErr ("Unable to install Line EDitor interface\n");
		}
	    }
	else
	    {
	    /* do not allow rlogins while doing a script */
	    shellLocked = TRUE;
	    }

	/* shell should not reference network directly for scalability */

	if (_func_remCurIdGet != NULL)		
            (* _func_remCurIdGet) (originalUser, originalPasswd);  

	shellExecuting = TRUE;
	}
    else
	{
	/* this must be a restart, i.e. via ABORT key;
	 * restore original interactive in/out/err fd's */

	if (interactive)
	    {
	    ioGlobalStdSet (STD_IN,  origFd [STD_IN]);
	    ioGlobalStdSet (STD_OUT, origFd [STD_OUT]);
	    ioGlobalStdSet (STD_ERR, origFd [STD_ERR]); 
	    printf ("\n");
	    }
	else
	    return;
	}

    ioctl (STD_IN, FIOOPTIONS, OPT_TERMINAL);

    (void)execShell (interactive);

    shellExecuting = FALSE;
    }
/*******************************************************************************
*
* execShell - execute stream of shell commands
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS execShell
    (
    FAST BOOL interactive
    )
    {
    char inLine [MAX_SHELL_LINE + 1];
    FAST int i;
    STATUS status = OK;

    if (startRemoteSession)		/* execute shell security if required */
        {

        /* 
         * Save user and password information in case it is changed 
         * during the remote session.  It is restored by shellLogout.  
         * Reference network indirectly for scalability.
         */

	if (_func_remCurIdGet != NULL)		
            (* _func_remCurIdGet) (originalUser, originalPasswd);  

        startRemoteSession = FALSE;		/* now that it is started */

        }	/* if (startRemoteSession) */

    shellAbort = FALSE;

    while (TRUE)
	{
	/* read next line */

	if (interactive)
	    {
	    int nchars;

	    if (ioGlobalStdGet (STD_IN) == -1)
		{
		taskDelay (1);		/* so that other task can run */
		continue;
		}
	    printf ("%s", promptString);

	    if ((ledId == (int)NULL) || (ledId == -1))
		nchars = fioRdString (STD_IN, inLine, MAX_SHELL_LINE);
	    else
		nchars = ledRead (ledId, inLine, MAX_SHELL_LINE);

	    if (nchars == EOF)
		{
		/* start shell over again */
		excJobAdd ((VOIDFUNCPTR)taskRestart, shellTaskId, 0, 0, 0, 0, 0);
		}
	    }
	else if (fioRdString (STD_IN, inLine, MAX_SHELL_LINE) != EOF)
	    printf ("%s\n", inLine);
	else
	    break;		/* end of script - bye */

	/* got a line of input:
	 *   ignore comments, blank lines, and null lines */

	inLine [MAX_SHELL_LINE] = EOS;	/* make sure inLine has EOS */

	for (i = 0; inLine [i] == ' '; i++)	/* skip leading blanks */
	    ;

	if (inLine [i] != '#' && inLine [i] != EOS)
	    {
	    /* Eliminate trailing space */

	    stringTrimRight (&inLine[i]);
	    if (inLine[i] == EOS)
		continue;

	    /* parse & execute command */

            /* The exit command is intercepted and if 
             * a user has telnetted or rlogged in to the shell,
             * logout is executed instead of exit. This allows 
             * proper cleanup of the rlogin or telnet 
             * session and allows control to return to the 
             * local shell console.*/
                
            if (strcmp (&inLine [i],"exit") == 0 && 
                shellIsRemoteConnected)
                status = execute ("logout");
            else
                status = execute (&inLine [i]);

	    if (status != OK && !interactive)
		{
		status = ERROR;
		break;
		}

	    /* check for script aborted;
	     *   note that "shellAbort" is a static variable and
	     *   is not reset until we get back up to an interactive
	     *   shell.  Thus all levels of nested scripts are aborted. */

	    if (shellAbort)
		{
		if (!interactive)
		    {
		    status = ERROR;
		    break;
		    }
		else
		    shellAbort = FALSE;
		}
	    }
	}

    return (status);
    }
/*******************************************************************************
*
* shellScriptAbort - signal the shell to stop processing a script
*
* This routine signals the shell to abort processing a script file.
* It can be called from within a script if an error is detected.
*
* RETURNS: N/A
*
* SEE ALSO:
* .pG "Target Shell"
*/

void shellScriptAbort (void)
    {
    shellAbort = TRUE;
    }
/*******************************************************************************
*
* shellHistory - display or set the size of shell history
*
* This routine displays shell history, or resets the default number of
* commands displayed by shell history to <size>.  By default, history size
* is 20 commands.  Shell history is actually maintained by ledLib.
*
* RETURNS: N/A
*
* SEE ALSO: ledLib, h(),
* .pG "Target Shell,"
* windsh,
* .tG "Shell"
*/

void shellHistory
    (
    int size    /* 0 = display, >0 = set history to new size */
    )
    {
    ledControl (ledId, NONE, NONE, size);
    }
/*******************************************************************************
*
* execute - interpret and execute a source line
*
* This routine parses and executes the specified source line.
* First any I/O redirection is cracked, then if any text remains,
* that text is parsed and executed via the yacc-based interpreter.
* If no text remains after the I/O redirection, then the shell itself
* is invoked (recursively) with the appropriate redirection.
* Note that blank lines, null lines, and comment lines should NOT
* be passed to this routine.  Initial blanks should be stripped too!
*
* RETURNS: OK or ERROR.
*
* NOMANUAL
*/

STATUS execute
    (
    FAST char *line
    )
    {
    int newInFd;
    int newOutFd;
    int oldInFd  = ioGlobalStdGet (STD_IN);
    int oldOutFd = ioGlobalStdGet (STD_OUT);
    STATUS status;

    /* get any redirection specs */

    if (getRedir (line, &newInFd, &newOutFd) != OK)
	return (ERROR);

    if (*line == EOS)
	{
        /* set any redirections specified, call shell, and restore streams */

	if (newInFd >= 0)
	    ioGlobalStdSet (STD_IN, newInFd);
	if (newOutFd >= 0)
	    ioGlobalStdSet (STD_OUT, newOutFd);

	status = execShell (FALSE);

	ioGlobalStdSet (STD_IN, oldInFd);
	ioGlobalStdSet (STD_OUT, oldOutFd);
	}
    else
	{
	/* set global stream fds for redirection of function calls;
	 * a -1 means no redirection
	 */

	redirInFd = newInFd;
	redirOutFd = newOutFd;

	/* initialize parse variables and parse and execute line */

	yystart (line);
	status = (yyparse () == 0) ? OK : ERROR;
	}

    /* close redirection files */

    if (newInFd >= 0)
	close (newInFd);

    if (newOutFd >= 0 && close (newOutFd) == ERROR)
	printf ("can't close output.\n");

    return (status);
    }
/*******************************************************************************
*
* getRedir - establish redirection specified on input line
*
* This routines picks the redirection specs off the end of the input line.
* The values pointed to by pInFd and pOutFd are set to -1 if the input and
* output respectively are not redirected, and to the file descriptor (fd) of
* the redirection stream if they are redirected.  Note that this routine
* also trucates from the end of the input line any successfully cracked
* redirection specs, i.e. an EOS is inserted in the input line at the point
* where the redirection specs began.
*
* RETURNS: ERROR if error in redirection spec or opening stream,
* OK if successful redirection found, or no redirection found.
*/

LOCAL STATUS getRedir
    (
    char *line,
    FAST int *pInFd,    /* -1, or fd of of input redirection */
    FAST int *pOutFd    /* -1, or fd of of output redirection */
    )
    {
    *pInFd = *pOutFd = -1;

    if (get1Redir (line, pInFd, pOutFd) != OK ||
        get1Redir (line, pInFd, pOutFd) != OK)
	{
	if (*pInFd >= 0)
	    close (*pInFd);

	if (*pOutFd >= 0)
	    close (*pOutFd);

	return (ERROR);
	}

    return (OK);
    }
/*******************************************************************************
*
* get1Redir - get a redirection from a line
*
* This routine picks a single redirection specification off the end
* of the specified line.
*
* RETURNS: ERROR if error in redirection spec or opening stream,
* OK if successful redirection found, or no redirection found.
*/

LOCAL STATUS get1Redir
    (
    char *line,         /* line to scan */
    int *pInFd,         /* if '<' found then fd is assigned here */
    int *pOutFd         /* if '>' found then fd is assigned here */
    )
    {
    FAST char *p;	       /* position in line */
    char *name;		       /* name of redirection file if found  */
    char *errnoString = NULL;  /* pointer to symTbl's copy of string */ 
    int	 ourErrno; 

    if (strlen (line) == 0)
	return (OK);

    /* Set p == end of line */

    p = line + strlen (line) - 1;

    /* pick off last word and back up to previous non-blank before that */

    while (p > line && *p == ' ')
	{
	p--;		/* skip trailing blanks */
	}

    *(p + 1) = EOS;

    /* stop searching if:
     *   reached beginning of line,
     *   reached a blank,
     *   reached a redirection token,
     *   hit a semicolon or quote.
     */

    while (p > line  && *p != ' ' &&
	   *p != '<' && *p != '>' &&
	   *p != ')' && *p != ';' &&
	   *p != '"')
	{
	p--;		/* skip back to start of word */
	}

    name = p + 1;	/* name must begin here */

    while (p > line && *p == ' ')
	p--;		/* skip back to previous non-blank */


    /* is this a redirection? */

    if (*p == '>' && *(p -1) != '>')
	{
	if (*pOutFd >= 0)
	    {
	    printf ("ambiguous output redirect.\n");
	    return (ERROR);
	    }

	if ((*pOutFd = creat (name, O_WRONLY)) < 0)
	    {
	    printf ("can't create output '%s'\n", name);
	    ourErrno = errno;
	    if ((errnoStringGet (ourErrno, errnoString) == OK) &&
		(errnoString != NULL))
		printf (redirErrMsgWithString, ourErrno, errnoString);
	    else
		printf (redirErrMsg, ourErrno);
	    return (ERROR);
	    }

	*p = EOS; /* truncate to exclude the redirection stuff just used */
	}
    else if (*p == '<' && (p == line || *(p -1) != '<'))
	{
	if (*pInFd >= 0)
	    {
	    printf ("ambiguous input redirect.\n");
	    return (ERROR);
	    }

	if ((*pInFd = open (name, O_RDONLY, 0)) < 0)
	    {
	    printf ("can't open input '%s'\n", name);
	    ourErrno = errno;
	    if ((errnoStringGet (ourErrno, errnoString) == OK) &&
		(errnoString != NULL))
		printf (redirErrMsgWithString, ourErrno, errnoString);
	    else
		printf (redirErrMsg, ourErrno);
	    return (ERROR);
	    }

	*p = EOS; /* truncate to exclude the redirection stuff just used */
	}

    return (OK);
    }
/*******************************************************************************
*
* shellPromptSet - change the shell prompt
*
* This routine changes the shell prompt string to <newPrompt>.
*
* RETURNS: N/A
*
* SEE ALSO:
* .pG "Target Shell,"
* windsh,
* .tG "Shell"
*/

void shellPromptSet
    (
    char *newPrompt     /* string to become new shell prompt */
    )
    {
    strncpy (promptString, newPrompt, MAX_PROMPT_LEN);
    }
/*******************************************************************************
*
* shellOrigStdSet - set the shell's default input/output/error file descriptors
*
* This routine is called to change the shell's default standard
* input/output/error file descriptor.  Normally, it is used only by the
* shell, rlogindTask(), and telnetdTask().  Values for <which> can be
* STD_IN, STD_OUT, or STD_ERR, as defined in vxWorks.h.  Values for <fd> can
* be the file descriptor for any file or device.
*
* RETURNS: N/A
*/

void shellOrigStdSet
    (
    int which,  /* STD_IN, STD_OUT, STD_ERR */
    int fd      /* fd to be default */
    )
    {
    origFd [which] = fd;
    }
/*******************************************************************************
*
* shellLock - lock access to the shell
*
* This routine locks or unlocks access to the shell.  When locked, cooperating
* tasks, such as telnetdTask() and rlogindTask(), will not take the shell.
*
* RETURNS:
* TRUE if <request> is "lock" and the routine successfully locks the shell,
* otherwise FALSE.  TRUE if <request> is "unlock" and the routine
* successfully unlocks the shell, otherwise FALSE.
*
* SEE ALSO:
* .pG "Target Shell"
*/

BOOL shellLock
    (
    BOOL request        /* TRUE = lock, FALSE = unlock */
    )
    {
    if (request == shellLocked)
	return (FALSE);

    shellLocked = request;
    return (TRUE);
    }
/********************************************************************************
*
* shellLogin - login using the user-supplied login routine
*
* RETURNS: OK or ERROR
*
* NOMANUAL
*/

STATUS shellLogin
    (
    int fd   /* i/o file descriptor passed from telnetd */
    )
    {
    if (loginRtn != NULL)
        {

        /* 
         * The standard i/o of this task context will now be that of the 
         * specified descriptor.
         */

        ioTaskStdSet (0, STD_IN, fd);
        ioTaskStdSet (0, STD_OUT, fd);

        /* Call the user-installed login routine */

        if ((*loginRtn)(loginRtnVar) == ERROR)
            {
	    return (ERROR);
            }

        printf("\n\n"); /* user already logged in at this point */
	return (OK);
        }	/* if (loginRtn != NULL) */

	return (OK); /* No login routine provided */
    }	
/********************************************************************************
* shellLoginInstall - login hook for network login routine
*
* RETURNS: N/A.
*
* NOMANUAL
*/

void shellLoginInstall
    (
    FUNCPTR logRtn,
    int logRtnVar
    )
    {
    loginRtn    = logRtn;
    loginRtnVar = logRtnVar;
    }
/*******************************************************************************
*
* shellLogoutInstall - logout hook for telnetdTask and rlogindTask
*
* RETURNS: N/A.
*
* NOMANUAL
*/

void shellLogoutInstall
    (
    FUNCPTR logRtn,
    int logVar
    )
    {
    logoutRtn = logRtn;
    logoutVar = logVar;
    }
/*******************************************************************************
*
* shellLogout - log out of the shell
*
* This routine logs out of the VxWorks shell.  If a remote login is active
* (via `rlogin' or `telnet'), it is stopped, and standard I/O is restored
* to the console.
*
* SEE ALSO: rlogindTask(), telnetdTask(), logout()
*
* RETURNS: N/A
*
* NOMANUAL
*/

void shellLogout (void)
    {
    /* Restore original user and password information, saved by shell().
     * Reference network indirectly for scalability.
     */

    shellLock (FALSE);

    if (_func_remCurIdSet != NULL)		
	(* _func_remCurIdSet) (originalUser, originalPasswd);  

    if (logoutRtn != NULL)
	(*logoutRtn) (logoutVar);

    }

/*******************************************************************************
*
* shellIsRemoteConnectedSet - notify shell of remote connection/disconnection
*
* This routine allows a remote session like rlogin or telnet to notify
* the shell of a successful remote connection/disconnection.
*
* TRUE = connected to remote session and FALSE = disconnected from remote
* session.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void shellIsRemoteConnectedSet
    (
    BOOL remoteConnection    /* TRUE = connected, FALSE = disconnected */
    )
    {
    shellIsRemoteConnected = remoteConnection; 
    }

/*******************************************************************************
*
* shellIsRemoteConnectedGet - get remote connection status of shell
*
* This routine allows a user to get the remote connection status of the shell.
*
* RETURNS: TRUE if shell is remotely connected or FALSE if the shell is not
* remotely connected.
*
* NOMANUAL
*/

BOOL shellIsRemoteConnectedGet (void)
    {
    return shellIsRemoteConnected;
    }

/*******************************************************************************
*
* errnoStringGet - get string associated with errno
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS errnoStringGet
    (
    FAST int errnum,     /* status code whose name is to be printed */
    char * errnoString
    )
    {
    void *    val;
    SYMBOL_ID symId;

    /* new symLib api - symbol name lengths are no longer limited */

    if ((statSymTbl != NULL) &&
	(symFindSymbol (statSymTbl, NULL, (void *)errnum, 
			SYM_MASK_NONE, SYM_MASK_NONE, &symId) == OK) &&
	(symNameGet (symId, &errnoString) == OK) &&
	(symValueGet (symId, &val) == OK) &&
	(val == (void *)errnum))
	{
	return (OK);
	}

    return (ERROR);
    }

/*******************************************************************************
*
* stringTrimRight - remove trailing white space from a string
*
* RETURNS: void.
*/

LOCAL void stringTrimRight
    (
    char *	strToTrim			/* string to trim right */
    )
    {
    register char *	strCursor = NULL;	/* string cursor */

    strCursor = strToTrim + strlen(strToTrim) - 1;

    while (strCursor > strToTrim)
	{
	if (isspace ((int)(*strCursor)))
	    strCursor--;
	else
	    break;
	}

    if (strCursor == strToTrim)
	{

	if (isspace ((int)(*strCursor)))   /* whole string is white space */
	    {
	    *strCursor = EOS;
	    return;
	    }
	}

    /* Normal return, non-empty string */

    *(strCursor+1) = EOS;
    return;
    }

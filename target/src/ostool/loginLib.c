/* loginLib.c - user login/password subroutine library */

/* Copyright 1984-1992 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02a,26may99,pfl  fixed login password encryption (SPR 9584)
01z,14mar99,jdi  doc: removed refs to config.h and/or configAll.h (SPR 25663).
01y,14jul97,dgp  doc: change ^D to CTRL-D in loginPrompt
01x,10jul97,dgp  doc: fix SPR 8303, loginUserAdd() saves address of password
01w,01aug96,sgv  fix for spr #4971, passwd set in loginPrompt.
01v,13oct95,jdi  fixed vxencrypt pathnames; changed refs to UNIX to "host".
01u,14mar93,jdi  fixed typo.
01t,20jan93,jdi  documentation cleanup for 5.1.
01s,20jul92,jmm  added group parameter to symAdd call
01r,18jul92,smb  Changed errno.h to errnoLib.h.
01q,26may92,rrr  the tree shuffle
01p,13dec91,gae  ANSI cleanup.
01o,19nov91,rrr  shut up some ansi warnings.
01n,14nov91,jpb  fixed problem with logout trashing user name.
		 moved all references to originalUser and originalPasswd
		 from loginLib.c to shellLib.c (local). see spr 916 and 1100.
01m,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -changed VOID to void
		  -changed copyright notice
01l,13may91,shl  undo'ed 01j.
01k,01may91,jdi  documentation tweaks.
01j,29apr91,shl  added call to save machine name, user and group ids (spr 916).
01i,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by shl.
01h,11feb91,jaa	 documentation cleanup.
01g,08oct90,shl  fixed to set NULL password correctly in remCurIdSet().
01f,04oct90,shl  fixed loginPrompt() to save original and install
		 new user name and password after successful rlogin.
01e,10aug90,dnw  made loginDefaultEncrypt be default implicitly by setting
		   encryptRtn to loginDefaultEncrypt;
		 cleaned-up documentation.
01d,15jul90,gae  made loginPrompt check userName against NULL -- bug fix.
01c,09may90,shl  fixed loginStringSet to copy MAX_LOGIN_NAME_LEN bytes.
01b,19apr90,shl  de-linted.
01a,03feb90,shl  written.
*/

/*
DESCRIPTION
This library provides a login/password facility for network access to the
VxWorks shell.  When installed, it requires a user name and password match
to gain access to the VxWorks shell from rlogin or telnet.  Therefore VxWorks
can be used in secure environments where access must be restricted.

Routines are provided to prompt for the user name and password, and 
verify the response by looking up the name/password pair in a login user
table.  This table contains a list of user names and encrypted passwords
that will be allowed to log in to the VxWorks shell remotely.  Routines are
provided to add, delete, and access the login user table.  The list of
user names can be displayed with loginUserShow().

INSTALLATION
The login security feature is initialized by the root task, usrRoot(), in
usrConfig.c, if the configuration macro INCLUDE_SECURITY is defined.  
Defining this macro also adds a single default user to the login table.
The default user and password are defined as LOGIN_USER_NAME
and LOGIN_PASSWORD.  These can be set to any desired name and password.
More users can be added by making additional calls to loginUserAdd().  If
INCLUDE_SECURITY is not defined, access to VxWorks will not be restricted
and secure.

The name/password pairs are added to the table by calling loginUserAdd(),
which takes the name and an encrypted password as arguments.  The VxWorks
host tool vxencrypt is used to generate the encrypted form of a password.
For example, to add a user name of "fred" and password of "flintstone",
first run vxencrypt on the host to find the encryption of "flintstone" as
follows:
.CS
    % vxencrypt
    please enter password: flintstone
    encrypted password is ScebRezb9c
.CE

Then invoke the routine loginUserAdd() in VxWorks:
.CS
	loginUserAdd ("fred", "ScebRezb9c");
.CE

This can be done from the shell, a start-up script, or application code.

LOGGING IN
When the login security facility is installed, every attempt to rlogin
or telnet to the VxWorks shell will first prompt for a user name and password.
.CS
    % rlogin target

    VxWorks login: fred
    Password: flintstone

    ->
.CE

The delay in prompting between unsuccessful logins is increased linearly with
the number of attempts, in order to slow down password-guessing programs.

ENCRYPTION ALGORITHM
This library provides a simple default encryption routine,
loginDefaultEncrypt().  This algorithm requires that
passwords be at least 8 characters and no more than 40 characters.

The routine loginEncryptInstall() allows a user-specified encryption
function to be used instead of the default.

INCLUDE FILES: loginLib.h

SEE ALSO: shellLib, vxencrypt,
.pG "Shell"
*/

#include "vxWorks.h"
#include "stdlib.h"
#include "semLib.h"
#include "string.h"
#include "lstLib.h"
#include "loginLib.h"
#include "ioLib.h"
#include "symLib.h"
#include "symbol.h"
#include "remLib.h"
#include "errnoLib.h"
#include "stdio.h"
#include "unistd.h"
#include "sysLib.h"
#include "tickLib.h"


/* global variables */

int     loginTimeOutInSecond = 60;    /* number of seconds before timing out */

/* local variables */

LOCAL char    	loginString [MAX_LOGIN_NAME_LEN + 1] = "VxWorks login: ";
LOCAL SYMTAB_ID loginSymTbl;
LOCAL FUNCPTR 	encryptRtn = loginDefaultEncrypt; /* encryption function */
LOCAL int     	encryptVar;			  /* and its argument */


/* forward static functions */

static BOOL loginPrintName (char *name, int val, SYM_TYPE type, char *string);
static STATUS loginNameGet (char *name);
static STATUS loginPasswdGet (char *passwd);
static STATUS loginEncrypt (char *in, char *out);


/*******************************************************************************
*
* loginInit - initialize the login table
*
* This routine must be called to initialize the login data structure used by
* routines throughout this module.  If the configuration macro INCLUDE_SECURITY
* is defined, it is called by usrRoot() in usrConfig.c, before any other
* routines in this module.
*
* RETURNS: N/A
*/

void loginInit (void)
    {
    static BOOL loginInitialized = FALSE;

    if (!loginInitialized)
        {
        loginInitialized = TRUE;
	loginSymTbl 	 = symTblCreate (6, FALSE, memSysPartId);
					/* make 64 entry hash table */
	}
    }
/*******************************************************************************
*
* loginUserAdd - add a user to the login table
*
* This routine adds a user name and password entry to the login table.
* Note that what is saved in the login table is the user name and the
* address of <passwd>, not the actual password.
*
* The length of user names should not exceed MAX_LOGIN_NAME_LEN, while
* the length of passwords depends on the encryption routine used.  For the
* default encryption routine, passwords should be at least 8 characters long
* and no more than 40 characters.
*
* The procedure for adding a new user to login table is as follows:
* .IP (1) 4
* Generate the encrypted password by invoking vxencrypt
* in \f3host/<hostOs>/bin\fP.
* .IP (2)
* Add a user by invoking loginUserAdd() in the VxWorks shell
* with the user name and the encrypted password.
* .LP
* The password of a user can be changed by first deleting the user entry,
* then adding the user entry again with the new encrypted password.
*
* EXAMPLE
*.CS
*    -> loginUserAdd "peter", "RRdRd9Qbyz"
*    value = 0 = 0x0
*    -> loginUserAdd "robin", "bSzyydqbSb"
*    value = 0 = 0x0
*    -> loginUserShow
*
*      User Name
*      =========
*      peter
*      robin
*    value = 0 = 0x0
*    ->
*.CE
*
* RETURNS: OK, or ERROR if the user name has already been entered.
*
* SEE ALSO: vxencrypt
*/

STATUS loginUserAdd
    (
    char  name[MAX_LOGIN_NAME_LEN+1],   /* user name */
    char  passwd[80]                    /* user password */
    )
    {
    char     *value;
    SYM_TYPE  type;			/* login type */

    if (symFindByName (loginSymTbl, name, &value, &type) == OK)
	{
	errnoSet (S_loginLib_USER_ALREADY_EXISTS);
	return (ERROR);
	}
    else
        if (symAdd (loginSymTbl, name, passwd, type, symGroupDefault) != OK)
	    return (ERROR);

    return (OK);
    }
/*******************************************************************************
*
* loginUserDelete - delete a user entry from the login table
*
* This routine deletes an entry in the login table.
* Both the user name and password must be specified to remove an entry
* from the login table.
*
* RETURNS: OK, or ERROR if the specified user or password is incorrect.
*/

STATUS loginUserDelete
    (
    char  *name,                /* user name */
    char  *passwd               /* user password */
    )
    {
    char      encryptBuf [80+1];
    char     *value;
    SYM_TYPE  type;

    if (loginEncrypt (passwd, encryptBuf) == ERROR)
        return (ERROR);

    if (symFindByName (loginSymTbl, name, &value, &type) == OK)
	{
	if (symRemove (loginSymTbl, name, type) == OK)
	    return (OK);
   	}

    errnoSet (S_loginLib_UNKNOWN_USER);
    return (ERROR);
    }
/*******************************************************************************
*
* loginUserVerify - verify a user name and password in the login table
*
* This routine verifies a user entry in the login table.
*
* RETURNS: OK, or ERROR if the user name or password is not found.
*/

STATUS loginUserVerify
    (
    char  *name,                /* name of user */
    char  *passwd               /* password of user */
    )
    {
    char      encryptBuf[80+1];
    char     *value;
    SYM_TYPE  type;

    if (loginEncrypt (passwd, encryptBuf) == ERROR)
	return (ERROR);

    if (symFindByName (loginSymTbl, name, &value, &type) == ERROR)
        {
        errnoSet(S_loginLib_UNKNOWN_USER);
        return (ERROR);
        }

    if (strcmp (value, encryptBuf) == 0)	/* verify password */
	return (OK);
    else
	{
	errnoSet (S_loginLib_INVALID_PASSWORD);
	return (ERROR);
	}
    }
/*******************************************************************************
*
* loginPrintName - display a single user entry
*
* ARGSUSED1
*/

LOCAL BOOL loginPrintName
    (
    char     *name,
    int       val,
    SYM_TYPE  type,
    char     *string
    )
    {
    printf ("  %-15s\n", name);
    return (TRUE);
    }
/*******************************************************************************
*
* loginUserShow - display the user login table
*
* This routine displays valid user names.
*
* EXAMPLE
*.CS
*     -> loginUserShow ()
*
*       User Name
*       =========
*       peter
*       robin
*     value = 0 = 0x0
*.CE
*
* RETURNS: N/A
*/

void loginUserShow (void)
    {
    char  *string;

    string = "";

    printf ("\n%s\n", "  User Name");
    printf (  "%s\n", "  =========");

    (void)symEach (loginSymTbl, (FUNCPTR)loginPrintName, (int)string);
    }
/*****************************************************************************
*
* loginNameGet - prompt user for login name
*
* RETURNS: OK if <name> is at least 1 byte long, or ERROR if less.
*/

LOCAL STATUS loginNameGet
    (
    char *name          /* buffer for user name */
    )
    {
    int   nbytes 	  = 0;		/* bytes read */
    int   timeOutInSecond = 15;
    long  bytesToRead;
    ULONG   secondPassed;
    ULONG  startTick;

    while (name == NULL || name[0] == EOS)
	{
	printf ("\n%s",loginString);

	secondPassed = 0;
	bytesToRead  = 0;
	startTick    = tickGet ();

	while (secondPassed < timeOutInSecond && bytesToRead == 0)
	    {
	    if (ioctl (STD_IN, FIONREAD, (int)&bytesToRead) == ERROR)
		return (ERROR);

	    taskDelay (sysClkRateGet () /4);
	    secondPassed = (tickGet () - startTick) / sysClkRateGet ();
	    }

	if (secondPassed >= timeOutInSecond)
	    {
	    printf ("\n");
	    name[0] = EOS;
	    return (OK);
	    }

	if (bytesToRead > 0)
	   nbytes = read (STD_IN, name, MAX_LOGIN_NAME_LEN);

	if (nbytes > 0)
	    {
	    name [nbytes - 1] = EOS;

	    if (nbytes > 1)
		return (OK);
	    }
	else
	    {
	    return (ERROR);
	    }
	}

    return (ERROR);	/* just in case, should never reach here */
    }
/*******************************************************************************
*
* loginPasswdGet - get password from user
*
* RETURNS: OK if the user types in the password within the time limit,
* or ERROR otherwise.
*/

LOCAL STATUS loginPasswdGet
    (
    char *passwd        /* buffer for passwd field up  */
                        /* to MAX_LOGIN_NAME_LEN bytes */
    )
    {
    STATUS status;
    int    nbytes = 0;
    int    timeOutInSecond = 15;
    long   bytesToRead;
    int    secondPassed;
    ULONG   startTick;
    int    oldoptions;		/* original OPT_TERM options */
    int    newoptions;		/* new OPT_TERM options during login */

    oldoptions = ioctl (STD_IN, FIOGETOPTIONS, 0);
    newoptions = oldoptions & ~OPT_ECHO;

    printf ("Password: ");	/* ask for password */

    /* don't want to echo the password */
    ioctl (STD_IN, FIOSETOPTIONS, newoptions);

    bytesToRead  = 0;
    secondPassed = 0;
    startTick    = tickGet();

    while (secondPassed < timeOutInSecond &&
	   bytesToRead == 0)
	{
	if (ioctl (STD_IN, FIONREAD, (int)&bytesToRead) == ERROR)
	    return (ERROR);

	taskDelay (sysClkRateGet() /4);
	secondPassed = (tickGet() - startTick) /sysClkRateGet();
	}

    if (bytesToRead > 0)
	nbytes = read (STD_IN, passwd, MAX_LOGIN_NAME_LEN);

    if (nbytes > 0)
	{
	passwd [nbytes - 1] = EOS;
	status = OK;
	}
    else
	status = ERROR;

    ioctl (STD_IN, FIOSETOPTIONS, oldoptions); 		/* restore options */
    return (status);
    }
/*****************************************************************************
*
* loginPrompt - display a login prompt and validate a user entry
*
* This routine displays a login prompt and validates a user entry.  If both
* user name and password match with an entry in the login table, the user is
* then given access to the VxWorks system.  Otherwise, it prompts the user
* again.
*
* All control characters are disabled during authentication except CTRL-D,
* which will terminate the remote login session.
*
* INTERNAL
* This routine should be called when starting (not a restart) the shell,
* during rlogin, or login via telnet.
*
* RETURNS: OK if the name and password are valid, or ERROR if there is an
* EOF or the routine times out.
*/

STATUS loginPrompt
    (
    char  *userName     /* user name, ask if NULL or not provided */
    )
    {
    char    name [MAX_LOGIN_NAME_LEN];	    /* buffer to hold user name */
    char    passwd [MAX_LOGIN_NAME_LEN];    /* buffer to hold user password */
    int     minTimeOutInSecond 	= 60;
    long    maxTimeOutInSecond 	= 60 * 60;  /* 1 hour */
    STATUS  status           	= OK;
    int     ix                 	= 0;
    int	    secondPassed 	= 0;
    long    totalTicks;		/* ticks equivalent to loginTimeOutInSecond */
    ULONG   startTick;
    ULONG   tickUsed;
    int     oldoptions = ioctl (STD_IN, FIOGETOPTIONS, 0);
    int     newoptions = (oldoptions & ~(OPT_ABORT) & ~(OPT_MON_TRAP));

    /* disable interrupt or reboot from the terminal during network login */

    (void)ioctl (STD_IN, FIOSETOPTIONS, newoptions);

    /* each loginPrompt session cannot be more than 60 minutes
     * or less than 60 seconds */

    if (loginTimeOutInSecond < minTimeOutInSecond)
	loginTimeOutInSecond = minTimeOutInSecond;
    else if (loginTimeOutInSecond > maxTimeOutInSecond)
	loginTimeOutInSecond = maxTimeOutInSecond;

    startTick  = tickGet();
    totalTicks = loginTimeOutInSecond * sysClkRateGet();

    if (userName == NULL)
	name[0] = EOS;
    else
	strcpy (name, userName);	/* user name provided */

    while (secondPassed < loginTimeOutInSecond)
	{
	(void)ioctl (STD_IN, FIOFLUSH, 0);

	if (strlen (name) == 0)
	    {
	    if (loginNameGet (name) == ERROR)	/* get user name */
		{
		/* the remote site will disconnect from the VxWorks shell */

		status = ERROR;
		break;
		}
  	    }

	if (loginUserVerify (name, "") == OK) 	/* no password needed */
	    break;

	if (strlen (name) > 0)
	    {
	    if (loginPasswdGet (passwd) == ERROR)
		{
		status = ERROR;
		break;
		}
	    else if (loginUserVerify (name, passwd) == OK)   /* correct login */
		{
		status = OK;
		break;
		}
	    else
		{
		printf ("\nLogin incorrect\n");
		ix++;		/* keep count of failed login try */
		}
	    }

	name[0] = EOS; 			/* reset user name */

	tickUsed = tickGet() - startTick;
        secondPassed = tickUsed / sysClkRateGet ();

	/* delay increases linearly as to the number of failed login attempts */
	if ((totalTicks - tickUsed) > (sysClkRateGet () * ix))
	    taskDelay (sysClkRateGet() * ix);
	}

    if (secondPassed >= loginTimeOutInSecond)
	{
	printf ("\nLogin timed out after %d seconds!\n", loginTimeOutInSecond);
	status = ERROR;
	}

    taskDelay (sysClkRateGet ()/2);

    (void) ioctl (STD_IN, FIOSETOPTIONS, oldoptions);	/* restore terminal */

    /* set user name to new user if correctly rlogin'd */

    if (status == OK)
	{
	/* passwd also set */

	remCurIdSet (name, passwd); /* restored by shellLogout */
	}

    return (status);
    }
/*****************************************************************************
*
* loginStringSet - change the login string
*
* This routine changes the login prompt string to <newString>.
* The maximum string length is 80 characters.
*
* RETURNS: N/A
*/

void loginStringSet
    (
    char  *newString            /* string to become new login prompt */
    )
    {
    strncpy (loginString, newString, MAX_LOGIN_NAME_LEN);
    }

/*****************************************************************************
*
* loginEncryptInstall - install an encryption routine
*
* This routine allows the user to install a custom encryption routine.
* The custom routine <rtn> must be of the following form: 
* .CS
* STATUS encryptRoutine
*        (
*        char *password,               /@ string to encrypt    @/
*        char *encryptedPassword       /@ resulting encryption @/
*        )
* .CE
* When a custom encryption routine is installed, a host version of
* this routine must be written to replace the tool vxencrypt
* in \f3host/<hostOs>/bin\fP.
*
* EXAMPLE
* The custom example above could be installed as follows:
* .CS
* #ifdef INCLUDE_SECURITY
*     loginInit ();                               /@ initialize login table   @/
*     shellLoginInstall (loginPrompt, NULL);      /@ install shell security   @/
*     loginEncryptInstall (encryptRoutine, NULL); /@ install encrypt. routine @/
* #endif
* .CE
*
* RETURNS: N/A
*
* SEE ALSO: loginDefaultEncrypt(), vxencrypt
*/

void loginEncryptInstall
    (
    FUNCPTR rtn,        /* function pointer to encryption routine */
    int     var         /* argument to the encryption routine (unused) */
    )
    {
    encryptRtn = rtn;
    encryptVar = var;
    }
/*****************************************************************************
*
* loginEncrypt - invoke the encryption routine installed by loginEncryptInstall
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS loginEncrypt
    (
    char *in,           /* argument to be passed to encryption routine */
    char *out           /* argument to be passed to encryption routine */
    )
    {
    if ((encryptRtn != NULL) && ((*encryptRtn) (in, out) != OK))
	return (ERROR);

    return (OK);
    }
/******************************************************************************
*
* loginDefaultEncrypt - default password encryption routine
*
* This routine provides default encryption for login passwords.  It employs
* a simple encryption algorithm.  It takes as arguments a string <in> and a
* pointer to a buffer <out>.  The encrypted string is then stored in the
* buffer.
*
* The input strings must be at least 8 characters and no more than 40
* characters.
*
* If a more sophisticated encryption algorithm is needed, this routine can
* be replaced, as long as the new encryption routine retains the same
* declarations as the default routine.  The routine vxencrypt
* in \f3host/<hostOs>/bin\fP
* should also be replaced by a host version of <encryptionRoutine>.  For more
* information, see the manual entry for loginEncryptInstall().
*
* RETURNS: OK, or ERROR if the password is invalid.
*
* SEE ALSO: loginEncryptInstall(), vxencrypt
*
* INTERNAL
* The encryption is done by summing the password and multiplying it by
* a magic number.
*/

STATUS loginDefaultEncrypt
    (
    char *in,                           /* input string */
    char *out                           /* encrypted string */
    )
    {
    int            ix;
    unsigned long  magic     = 31695317;
    unsigned long  passwdInt = 0;

   if (strlen (in) < 8 || strlen (in) > 40)
        {
	errnoSet (S_loginLib_INVALID_PASSWORD);
        return (ERROR);
        }

    for (ix = 0; ix < strlen(in); ix++)         /* sum the string */
        passwdInt += (in[ix]) * (ix+1) ^ (ix+1);

    sprintf (out, "%u", (long) (passwdInt * magic)); /* convert interger
							to string */
    /* make encrypted passwd printable */

    for (ix = 0; ix < strlen (out); ix++)
        {
        if (out[ix] < '3')
            out[ix] = out[ix] + '!';    /* arbitrary */

        if (out[ix] < '7')
            out[ix] = out[ix] + '/';    /* arbitrary */

        if (out[ix] < '9')
            out[ix] = out[ix] + 'B';    /* arbitrary */
        }

    return (OK);
    }

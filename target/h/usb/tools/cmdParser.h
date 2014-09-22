/* cmdParser.h - Command line parsing utility functions */

/* Copyright 2000 Wind River Systems, Inc. */

/*
Modification history
--------------------
01b,23nov99,rcb  Make KeywordMatch() function public.
01a,01jun99,rcb  First.
*/

/*
DESCRIPTION

This file includes a collection of command-line parsing functions which are
useful in the creation of command line utilities, such as bus exercisers.

There are three groups of functions defined by this library.  The first is
a collection of general string parser functions, such as functions to eliminate
white space, functions to strip tokens out of a string, and so forth.  

The second set of functions drive the actual parsing process.  In order to 
use this second set of functions, clients must construct a table of CMD_DESCR
structures which define the commands to be recognized by the parser.  A 
brief example of such a table is shown below.

.CS
CMD_DESCR commands [] =
    {
    {"Help", 4, "Help/?", "Displays list of commands.", CmdParserHelpFunc},
    {"?",    1, NULL,	  NULL, 			CmdParserHelpFunc},
    {"Exit", 4, "Exit",   "Exits program.",		CmdParserExitFunc},
    {NULL,   0, NULL,	  NULL, 			NULL}
    };
.CE

The first field is the keyword for the command.  The second field specifies
the number of characters of the command which must match - allowing the user
to enter only a portion of the keyword as a shortcut.  The third and fourth
fields are strings giving the command usage and a brief help string.  A NULL
in the Help field indicates that the corresponding keyword is a synonym for 
another command its usage/help should not be shown.  The final field is a 
pointer to a function of type CMD_EXEC_FUNC which will be invoked if the 
parser encounters the corresponding command.

The third group of functions provide standard CMD_EXEC_FUNCs for certain
commonly used commands, such as CmdParserHelpFunc and CmdParserExitFunc as
shown in the preceding example.

The caller may pass a generic (pVOID) parameter to the command line parsing
functions in the second group.	This function is in turn passed to the
CMD_EXEC_FUNCs.  In this way, the caller can specify context information
for the command execution functions.

Commands are executed after the user presses [enter].  Multiple commands may 
be entered on the same command line separated by semicolons (';').  Each 
command as if it had been entered on a separate line (unless a command 
terminates with an error, in which case all remaining commands entered on the 
same line will be ignored).
*/


#ifndef __INCcmdParserh
#define __INCcmdParserh

#ifdef	__cplusplus
extern "C" {
#endif


/* Constants */

/* Command parser return codes */

#define RET_OK		    0		/* Normal termination */
#define RET_ERROR	    1		/* Program failure */
#define RET_CONTINUE	    2		/* Program continues looping */


/* Prompt & input definitions */

#define CMD_SEPARATOR	    ';' 	/* Command separator character */

#define MAX_CMD_LEN	    256 	/* Maximum command length */
#define MAX_KEYWORD_LEN     32		/* Maximum cmd keyword length */


/* Command descriptors */

/*
 * CMD_EXEC_FUNC
 *
 * Note: The CMD_EXEC_FUNC is expected to update the <Cmd> parameter so
 * that it points to the next character in the command line following 
 * the parameters consumed by the CMD_EXEC_FUNC itself.
 */


typedef UINT16 (*CMD_EXEC_FUNC)
    (
    pVOID param,			/* Generic parameter passed down */
    char **ppCmd,			/* Ptr to remainder of cmd line */
    FILE *fin,				/* stream for input (if any) */
    FILE *fout				/* stream for output (if any) */
    );

/*
 * CMD_DESCR - defines a command/keyword to be recognized by parser
 */

typedef struct cmd_descr
    {
    char *keyword;			/* Command keyword */
    int minMatch;			/* Minimum # of chars to match */
    char *usage;			/* Syntax */
    char *help; 			/* Description string */
    CMD_EXEC_FUNC execFunc;		/* Command execution function */
    } CMD_DESCR, *pCMD_DESCR;


/* function prototypes */

UINT16 PromptAndExecCmd
    (
    pVOID param,		/* Generic parameter for exec funcs */
    char *pPrompt,		/* Prompt to display */
    FILE *fin,			/* Input stream */
    FILE *fout, 		/* Output stream */
    CMD_DESCR *pCmdTable	/* CMD_DESCR table */
    );


UINT16 ExecCmd
    (
    pVOID param,		/* Generic parameter for exec funcs */
    char *pCmd, 		/* Cmd buffer to be parsed/executed */
    FILE *fin,			/* Stream for input */
    FILE *fout, 		/* Stream for output */
    CMD_DESCR *pCmdTable	/* CMD_DESCR table */
    );


int KeywordMatch
    (
    char *s1,			    /* string 1 */
    char *s2,			    /* string 2 */
    int len			    /* max length to compare */
    );


char *SkipSpace 
    (
    char *pStr			/* Input string */
    );


UINT16 TruncSpace 
    (
    char *pStr			/* Input string */
    );


char *GetNextToken 
    (
    char *pStr, 		/* Input string */
    char *pToken,		/* Bfr to receive token */
    UINT16 tokenLen		/* Max length of Token bfr */
    );


char *GetHexToken
    (
    char *pStr, 		/* input string */
    long *pToken,		/* buffer to receive token value */
    long defVal 		/* default value */
    );


UINT16 CmdParserHelpFunc
    (
    pVOID param,		/* Generic parameter passed down */
    char **ppCmd,		/* Ptr to remainder of cmd line */
    FILE *fin,			/* stream for input (if any) */
    FILE *fout			/* stream for output (if any) */
    );


UINT16 CmdParserExitFunc
    (
    pVOID param,		/* Generic parameter passed down */
    char **ppCmd,		/* Ptr to remainder of cmd line */
    FILE *fin,			/* stream for input (if any) */
    FILE *fout			/* stream for output (if any) */
    );


#ifdef	__cplusplus
}
#endif

#endif	/* __INCcmdParserh */


/* End of file. */


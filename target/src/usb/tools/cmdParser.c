/* cmdParser.c - Command line parser routines. */

/* Copyright 2000 Wind River Systems, Inc. */

/*
Modification history
--------------------
01f,18sep01,wef  merge from wrs.tor2_0.usb1_1-f for veloce, fixed formatting
01e,08aug01,dat  Removing warnings
01d,31jul01,wef  fix man page generation errors
01c,23nov99,rcb  Make KeywordMatch() function public.
01b,16aug99,rcb  Add option in CmdParserHelpFunc to display help only for
		 a specific command.
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
CMD_DESCR Commands [] =
    {
    {"Help", 4, "Help/?", "Displays list of commands.", CmdParserHelpFunc},
    {"?",    1, NULL,	  NULL, 	    CmdParserHelpFunc},
    {"Exit", 4, "Exit",   "Exits program.",	CmdParserExitFunc},
    {NULL,   0, NULL,	  NULL, 	    NULL}
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


/* Include files */

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "ctype.h"

#include "usb/usbPlatform.h"	    /* Basic definitions */
#include "usb/tools/cmdParser.h"    /* Our API */


/* Constants */

#define PAUSE_DISPLAY	    22	    /* Help display pauses every n lines */


/*************************************************************************
*
* PromptAndExecCmd - Prompt for a command and execute it.
*
* Displays <pPrompt> to <fout> and prompts for input from <fin>. Then, 
* parses/executes the command.	<pCmdTable> points to an array of
* CMD_DESCR structures defining the command to be recognized by the
* parser, and <Param> is a generic parameter passed down to individual 
* command execution functions.
*
* RETURNS:  RET_OK for normal termination
*	RET_ERROR for program failure.
*	RET_CONTINUE if execution should continue.
*/

UINT16 PromptAndExecCmd
    (
    pVOID param,	/* Generic parameter for exec funcs */
    char *pPrompt,	/* Prompt to display */
    FILE *fin,		/* Input stream */
    FILE *fout, 	/* Output stream */
    CMD_DESCR *pCmdTable    /* CMD_DESCR table */
    )

    {
    char cmd [MAX_CMD_LEN]; /* Buffer for input command */


    /* Display prompt */

    fprintf (fout, pPrompt);


    /* Retrieve input */

    if (fgets (cmd, sizeof (cmd), fin) != cmd)
        return RET_ERROR;


    /* Execute input */

    return ExecCmd (param, cmd, fin, fout, pCmdTable);
    }


/*************************************************************************
*
* KeywordMatch - Compare keywords
*
* Compares <s1> and <s2> up to <len> characters, case insensitive.  
* Returns 0 if strings are equal.  
*
* NOTE: This function is equivalent to strnicmp(), but that function is
* not available in all libraries.
*
* RETURNS:  0 if s1 and s2 are the same
*	-n if s1 < s2
*	+n if s1 > s2
*/

int KeywordMatch
    (
    char *s1,		    /* string 1 */
    char *s2,		    /* string 2 */
    int len		/* max length to compare */
    )

    {
    int n = 0;

    for (; len > 0; len--)
	{
	if ((n = toupper (*s1) - toupper (*s2)) != 0 || *s1 == 0 || *s2 == 0) 
	    break;
	s1++;
	s2++;
	}

    return n;
    }


/*************************************************************************
*
* ExecCmd - Execute the command line
*
* Parses and executes the commands in the <pCmd> buffer.  I/O - if any -
* will go to <fin>/<fout>.  The <pCmd> may contain any number of commands
* separated by CMD_SEPARATOR. <pCmdTable> points to an array of
* CMD_DESCR structures defining the command to be recognized by the
* parser, and <param> is a generic parameter passed down to individual 
* command execution functions.
*
* RETURNS:  RET_OK for normal termination.
*	RET_ERROR for program failure.
*	RET_CONTINUE if execution should continue.
*/

UINT16 ExecCmd
    (
    pVOID param,	/* Generic parameter for exec funcs */
    char *pCmd, 	/* Cmd buffer to be parsed/executed */
    FILE *fin,		/* Stream for input */
    FILE *fout, 	/* Stream for output */
    CMD_DESCR *pCmdTable    /* CMD_DESCR table */
    )

    {
    UINT16 s = RET_CONTINUE;	    /* Execution status */

    char keyword [MAX_KEYWORD_LEN]; /* Bfr for command keyword */
 
    CMD_DESCR *pEntry;		/* Pointer to entry in CmdTable */

    /* Parse each command on the command line. */

    TruncSpace (pCmd);

    do
	{
	
	/* Retrieve keyword and match against known keywords. */

	pCmd = GetNextToken (pCmd, keyword, sizeof (keyword));

	if (strlen (keyword) > 0)
	    {
	    for (pEntry = pCmdTable; 
		 pEntry != NULL && pEntry->keyword != NULL;
		 pEntry++)
		{
		if (KeywordMatch (pEntry->keyword, 
				  keyword, 
				  max (strlen (keyword), 
				       (unsigned)pEntry->minMatch))
			       == 0)
		    {

		    /* 
		     * We found a matching keyword.  Execute the CMD_EXEC_FUNC
		     * for this command. 
		     */

		    if (pEntry->execFunc == CmdParserHelpFunc)

			s = (*pEntry->execFunc) (pCmdTable, &pCmd, fin, fout);

		    else

			s = (*pEntry->execFunc) (param, &pCmd, fin, fout);

		    break;
		    }
		}

	    if (pEntry == NULL || pEntry->keyword == NULL)
		{
		fprintf (fout, "Unrecognized command: %s\n", keyword);
		break;
		}
	    }

	/* Skip over trailing CMD_SEPARATOR characters. */

	if (*pCmd != CMD_SEPARATOR)
	    while (*pCmd != CMD_SEPARATOR && *pCmd != 0)
		pCmd++;

	while (*pCmd == CMD_SEPARATOR)
	    pCmd++;
	}
	while (strlen (pCmd) > 0 && s == RET_CONTINUE);

    return s;
    }



/*************************************************************************
*
* SkipSpace - Skips leading white space in a string
*
* Returns a pointer to the first non-white-space character in <pStr>.
*
* RETURNS: Ptr to first non-white-space character in <pStr>
*/

#define IS_SPACE(c) (isspace ((int) (c)) || ((c) > 0 && (c) < 32))

char *SkipSpace 
    (
    char *pStr		/* Input string */
    )
    {
    while (IS_SPACE (*pStr))
	pStr++;

    return pStr;
    }


/*************************************************************************
*
* TruncSpace - Truncates string to eliminate trailing whitespace
*
* Trucates <pStr> to eliminate trailing white space.  Returns count
* of characters left in <pStr> upon return.
*
* RETURNS: Number of characters in <pStr> after truncation.
*/

UINT16 TruncSpace 
    (
    char *pStr		/* Input string */
    )
    {
    UINT16 len;

    while ((len = strlen (pStr)) > 0 && IS_SPACE (pStr [len - 1]))
	pStr [len - 1] = 0;

    return len;
    }


/*************************************************************************
*
* GetNextToken - Retrieves the next token from an input string
*
* Copies the next token from <pStr> to <pToken>.  White space before the
* next token is discarded.  Tokens are delimited by white space and by
* the command separator, CMD_SEPARATOR.  No more than <tokenLen> - 1
* characters from <pStr> will be copied into <pToken>.	<tokenLen> must be
* at least one and <pToken> will be NULL terminated upon return.
*
* RETURNS: Pointer into <pStr> following end of copied <pToken>.
*/

char *GetNextToken 
    (
    char *pStr, 	/* Input string */
    char *pToken,	/* Bfr to receive token */
    UINT16 tokenLen	/* Max length of Token bfr */
    )
    {
    UINT16 len; 	/* Temporary length counter */

    /* Skip leading white space */

    pStr = SkipSpace (pStr);


    /* Copy next token into Token bfr */

    pToken [0] = 0;

    while ((len = strlen (pToken)) < tokenLen && 
	   *pStr != 0 && 
	   !IS_SPACE (*pStr) && 
	   *pStr != CMD_SEPARATOR)
	{
	pToken [len] = *pStr;
	pToken [len+1] = 0;
	pStr++;
	}

    return pStr;
    }


/*************************************************************************
*
* GetHexToken - Retrieves value of hex token
*
* Retrieves the next token from <pCmd> line, interprets it as a hex 
* value, and stores the result in <pToken>.  If there are no remaining 
* tokens, stores <defVal> in <pToken> instead.
*
* RETURNS: Pointer into <pStr> following end of copied <pToken>
*/

char *GetHexToken
    (
    char *pStr, 	/* input string */
    long *pToken,	/* buffer to receive token value */
    long defVal 	/* default value */
    )

    {
    char temp [9];	/* Temp storage for token */

    /* Retrieve next token */

    pStr = GetNextToken (pStr, temp, sizeof (temp));

    /* Evaluate Token value */

    if (strlen (temp) == 0 || sscanf (temp, "%lx", pToken) == 0)
	*pToken = defVal;

    return pStr;
    }


/*************************************************************************
*
* CmdParserHelpFunc - Displays list of supported commands
*
* Displays the list of commands in the parser command table to <fout>.	
* When the parser recognizes that this function is about to be executed, 
* it substitutes a pointer to the current CMD_DESCR table in <param>. 
* If this function is called directly, <param> should point to a table
* of CMD_DESCR structures.
*
* RETURNS: RET_CONTINUE
*/

UINT16 CmdParserHelpFunc
    (
    pVOID param,	    /* Generic parameter passed down */
    char **ppCmd,	    /* Ptr to remainder of cmd line */
    FILE *fin,		    /* stream for input (if any) */
    FILE *fout		    /* stream for output (if any) */
    )

    {
    CMD_DESCR *pCmdTable = (CMD_DESCR *) param;
    char keyword [MAX_KEYWORD_LEN];
    int lines = 0;


    *ppCmd = GetNextToken (*ppCmd, keyword, sizeof (keyword));


    if (strlen (keyword) == 0)
	fprintf (fout, "\n");

    for (; pCmdTable != NULL && pCmdTable->keyword != NULL; pCmdTable++) 
	{
	if (pCmdTable->help != NULL) 
	    {
	    /* If a parameter has been entered, display help only for 
	     * matching cmds */

	    if (strlen (keyword) > 0)
		if (KeywordMatch (pCmdTable->keyword, 
				  keyword, 
				  max (strlen (keyword), 
				       (unsigned)pCmdTable->minMatch)) 
				 != 0)
		    continue;

	    /* Pause the display every PAUSE_DISPLAY lines */

	    if (lines == PAUSE_DISPLAY) 
		{
		fprintf (fout,	 "[Press a key to continue]");
		fgetc (fin);
		fprintf (fout, "\r		\r");
		lines = 0;
		}

	    fprintf (fout, "%*s  %s\n", 
		     (strlen (keyword) == 0) ? 22 : 0,
		     (pCmdTable->usage == NULL) ? 
				pCmdTable->keyword : pCmdTable->usage, 
		     pCmdTable->help);

	    ++lines;
	    }
	}

    if (strlen (keyword) == 0)
	fprintf (fout, "\n\n");

    return RET_CONTINUE;
    }


/*************************************************************************
*
* CmdParserExitFunc - Terminates parser execution
*
* Returns RET_OK, causing the parser to return RET_OK to the caller
* signally normal termination of the parser.
*
* RETURNS: RET_OK
*/

UINT16 CmdParserExitFunc
    (
    pVOID param,	    /* Generic parameter passed down */
    char **ppCmd,	    /* Ptr to remainder of cmd line */
    FILE *fin,		    /* stream for input (if any) */
    FILE *fout		    /* stream for output (if any) */
    )

    {
    return RET_OK;
    }


/* End of file. */


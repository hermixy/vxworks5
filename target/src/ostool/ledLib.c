/* ledLib.c - line-editing library */

/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02w,18oct01,fmk  add routine description to fix doc build errors
02v,27sep01,dcb  Fix SPR 21697.  Free the ledId that was allocated in ledOpen.
                   Compiler warning clean up.
02u,14jul97,dgp  doc: change ^ to CTRL- in cursor movement descriptions
02t,31oct96,elp  Replaced symAdd() call by symSAdd() call (symtbls synchro).
02s,23nov93,jmm  ledRead now ignores NULL characters (spr 2666)
02r,20jan93,jdi  documentation cleanup for 5.1.
02q,19oct92,jcf  fixed esc-space garbage.
02p,23aug92,jcf  fixed esc-k garbage when first commaned.
02o,20jul92,jmm  added group parameter to symAdd call
02n,18jul92,smb  Changed errno.h to errnoLib.h.
02m,26may92,rrr  the tree shuffle
02l,19mar92,jmm  fixed problem in histAdd and histAll w/long lines in history
                 SPR #1381
02k,20dec91,gae  stopped blow ups when using history for first time.
		 removed nulls from output stream in histAll().
02j,10dec91,gae  added includes for ANSI.
02i,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -fixed #else and #endif
		  -changed VOID to void
		  -changed copyright notice
02h,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by gae.
02g,08feb91,jaa	 documentation cleanup.
02f,25oct90,gae  fixed ignoring of previously set ty options - per Hdei Nunoe.
02e,10aug90,dnw  changed declaration of ledClose from void to STATUS.
02d,10aug90,kdl  added forward declarations for functions returning void.
02c,11may90,yao  added missing modification history (02b) for the last checkin.
02b,09may90,yao  typecasted malloc to (char *).
02a,15apr90,jcf  changed symbol type to SYM_TYPE.
01z,14mar90,jdi  documentation cleanup.
01y,22feb90,dab  fixed glitch in ledRead()'s 's' cmd. documentation.
01x,10nov89,dab  fixed 'x' cmd bug in ledRead(). fixed bfill() bug in ledRead().
		   fixed EOF bug in ledRead(). added 's' cmd to ledRead()'s vi
		   interface.
01w,13jun89,gae  fixed glitch in 01v caused by histSize not being init'd.
01v,01may89,dab  histInit() now keeps past history when size is changed.
01u,24sep88,gae  documentation touchup.
01t,30aug88,gae  documentation.
01s,10aug88,gae  fixed "static" bug in histInit()
01r,22jun88,dnw  changed bmove() to bcopy() since bcopy() now handles
		   overlapping buffers.
01q,30may88,dnw  changed to v4 names.
01p,09nov87,gae  changed name (again) from kornLib.c.
01o,03nov87,ecs  documenation.
01n,02nov87,gae  documenation.
01m,24oct87,gae  changed name from shellLib.c.  Fixed history mechanism:
		   added histInit() and removed built-in "h".
01l,17aug87,gae  Made re-entrant, viz. useable by programs other than the shell.
		 Fixed history bug.  Added symbol name completion.
01k,24jul87,gae  imported ty{Backspace,DeleteLine,Eof}Char's from tyLib.c
		   and used instead of constants.
01j,07apr87,jlf  deleted shHistDelete, which was unused, to appease lint.
01i,01apr87,ecs  delinted.
01h,19mar87,jlf  added copyright.
	   +gae	 minor fixes for mangen.
		 documentation.
01g,12mar87,gae  little fix in shHistFind() for bug with 'n'.
01f,02feb87,gae  added writex() so control chars will be printed as '?';
		   also done in shHistAll().  Improved motion (maybe);
		   added redraw (^L); made undo nicer; allowed replace to
		   be cancelled with ESC; fixed shHistFind() bug.
		 Added UNIX_DEBUG ability.
01e,19jan87,gae  included memLib.h to appease lint.
01d,14jan87,gae  made history use a free list, history search works
		   within lines not just at beginning, command mode default
		   after search, made history number increment properly;
		   saved space by making curLn a pointer to the user's string,
		   and more...
01c,20dec86,dnw  changed to not get include files from default directories.
		 replaced spaces[] and bksps[] with writen() (vax compiler
		   complained about line too long).
01b,18dec86,gae  various bug fixes & improvements.
01a,23oct86,gae  written
*/

/*
DESCRIPTION
This library provides a line-editing layer on top of a `tty' device.
The shell uses this interface for its history-editing features.

The shell history mechanism is similar to the UNIX Korn shell history
facility, with a built-in line-editor similar to UNIX \f3vi\fP that allows
previously typed commands to be edited.  The command h() displays the 20
most recent commands typed into the shell; old commands fall off the top
as new ones are entered.

To edit a command, type ESC to enter edit mode, and use the commands
listed below.  The ESC key switches the shell to edit mode.  The RETURN
key always gives the line to the shell from either editing or input mode.

The following list is a summary of the commands available in edit mode.

.TS
tab(|);
l s
lf9 l.
Movement and search commands:
.sp .25
<n>G | - Go to command number <n>.
/<s> | - Search for string <s> backward in history.
?<s> | - Search for string <s> forward in history.
n | - Repeat last search.
N | - Repeat last search in opposite direction.
<n>k | - Get <n>th previous shell command in history.
<n>- | - Same as "k".
<n>j | - Get <n>th next shell command in history.
<n>+ | - Same as "j".
<n>h | - Move left <n> characters.
CTRL-H | - Same as "h".
<n>l | - Move right <n> characters.
\f1SPACE\fP | - Same as "l".
<n>w | - Move <n> words forward.
<n>W | - Move <n> blank-separated words forward.
<n>e | - Move to end of the <n>th next word.
<n>E | - Move to end of the <n>th next blank-separated word.
<n>b | - Move back <n> words.
<n>B | - Move back <n> blank-separated words.
f<c> | - Find character <c>, searching forward.
F<c> | - Find character <c>, searching backward.
^ | - Move cursor to first non-blank character in line.
$ | - Go to end of line.
0 | - Go to beginning of line.

.T&
l s
lf9 l.
Insert commands (input is expected until an ESC is typed):
.sp .25
a | - Append.
A | - Append at end of line.
c \f1SPACE\fP | - Change character.
cl | - Change character.
cw | - Change word.
cc | - Change entire line.
c$ | - Change everything from cursor to end of line.
C | - Same as "c$".
S | - Same as "cc".
i | - Insert.
I | - Insert at beginning of line.
R | - Type over characters.

.T&
l s
lf9 l.
Editing commands:
.sp .25
<n>r<c> | - Replace the following <n> characters with <c>.
<n>x | - Delete <n> characters starting at cursor.
<n>X | - Delete <n> characters to the left of the cursor.
d \f1SPACE\fP | - Delete character.
dl | - Delete character.
dw | - Delete word.
dd | - Delete entire line.
d$ | - Delete everything from cursor to end of line.
D | - Same as "d$".
p | - Put last deletion after the cursor.
P | - Put last deletion before the cursor.
u | - Undo last command.
~ | - Toggle case, lower to upper or vice versa.

.T&
l s
lf9 l.
Special commands:
.sp .25
CTRL-U | - Delete line and leave edit mode.
CTRL-L | - Redraw line.
CTRL-D | - Complete symbol name.
\f1RETURN\fP | - Give line to shell and leave edit mode.
.TE

The default value for <n> is 1.

DEFICIENCIES
Since the shell toggles between raw mode and line mode, type-ahead can be
lost.  The ESC, redraw, and non-printable characters are built-in.  The
EOF, backspace, and line-delete are not imported well from tyLib.
Instead, tyLib should supply and/or support these characters
via ioctl().

Some commands do not take counts as users might expect.  For example,
"<n>i" will not insert whatever was entered <n> times.

INCLUDE FILES: ledLib.h

SEE ALSO:
.pG "Shell"

INTERNAL
LINE_LEN should be specifed elsewhere.
In an attempt to economize shell stack space, the "string" parameter is
written in up to LINE_LEN!  An EOS is used at the beginning of the string
as well, thus only LINE_LEN-1 places are returned.
*/

#include "vxWorks.h"
#include "ctype.h"
#include "errnoLib.h"
#include "ioLib.h"
#include "lstLib.h"
#include "memLib.h"
#include "stdlib.h"
#include "stdio.h"
#include "unistd.h"
#include "string.h"
#include "iosLib.h"
#include "sysSymTbl.h"
#include "symLib.h"
#include "symbol.h"


/* to debug under UNIX:
cc -O -I$vh -DPORTABLE -c bLib.c
cc -O -I$vh -DUNIX_DEBUG -o ledLib ledLib.c $v/lib/host/vxWorks.a symLib.o bLib.o
*/

#ifdef	UNIX_DEBUG

#undef	EOF
#undef	NULL

#include "stdio.h"

char tyBackspaceChar	= 0x08;	/* default is control-H */
char tyDeleteLineChar	= 0x15;	/* default is control-U */
char tyEofChar		= 0x04;	/* default is control-D */

SYMTAB_ID sysSymTbl;

#endif	/* UNIX_DEBUG */

IMPORT char tyBackspaceChar;	/* default is control-H */
IMPORT char tyDeleteLineChar;	/* default is control-U */
IMPORT char tyEofChar;		/* default is control-D */

#define	CMP_CHAR	0x04	/* symbol name completion		*/
#define	BEL_CHAR	0x07	/* bell					*/
#define RET_CHAR	'\n'	/* return				*/
#define	ESC_CHAR	0x1b	/* escape				*/
#define NON_CHAR	'?'	/* non-printable char. representation	*/
#define RDW_CHAR	0x0c	/* redraw				*/

#define	LINE_LEN	128	/* >= MAX_SHELL_LINE!			*/

#define	BACKWARD	(-1)
#define	FORWARD		1

#define save()	    if (strlen (saveLn) == 0) strcpy (saveLn, curLn)

typedef	struct		/* CMP_ARG */
    {
    int count;
    int nameLen;
    char *name;
    char *match;
    } CMP_ARG;

typedef struct	/* HIST */
    {
    NODE node;
    char line [LINE_LEN+1];
    } HIST;


typedef struct	/* LED */
    {
    int inFd;
    int outFd;
    int histSize;
    LIST histFreeList;
    LIST histList;
    HIST *pHist;

    /* XXX the following are not needed between ledRead's
     *     but are used in support routines.
     */

    char *buffer;	/* hold deletions from curLn */
    int histNum;	/* current history number */
    BOOL cmdMode;	/* insert or command mode */
    } LED;

typedef LED *LED_ID;


/* forward static functions */

static void setPreempt (FUNCPTR *catchFunc, FUNCPTR func);
static int preempt (FUNCPTR *catchFunc, char ch, char *curLn, int *curPs, int
		*number, LED_ID ledId);
static void redraw (int outFd, char *oldLn, int lastPs, char *curLn, int
		*curPs);
static BOOL completeName (char *curLn, int *curPs);
static BOOL findName (char *name, char *value, SYM_TYPE type, CMP_ARG *arg);
static void search (BOOL ignorePunct, BOOL endOfWord, char *curLn, int
		*curPs, int dir);
static BOOL find (char ch, char *curLn, int *curPs, int dir);
static STATUS findFwd (char ch, char *curLn, int *curPs, int *number);
static STATUS findBwd (char ch, char *curLn, int *curPs, int *number);
static STATUS change (char ch, char *curLn, int *curPs, int *number, LED_ID
		ledId);
static STATUS deletec (char ch, char *curLn, int *curPs, int *number, LED_ID
		ledId);
static STATUS replace (char ch, char *curLn, int *curPs, int *number);
static void beep (int outFd);
static int writex (int fd, char buffer [ ], int nbytes);
static int writen (int fd, char ch, int nbytes);
static void histInit (LED_ID ledId, int histSize);
static void histAdd (LED_ID ledId, char *line);
static BOOL histNum (LED_ID ledId, char *line, int n);
static STATUS histNext (LED_ID ledId, char *line);
static STATUS histPrev (LED_ID ledId, char *line);
static BOOL histFind (LED_ID ledId, char *match, char *line);
static void histAll (LED_ID ledId);


/*******************************************************************************
*
* ledOpen - create a new line-editor ID
*
* This routine creates the ID that is used by ledRead(), ledClose(), and
* ledControl().  Storage is allocated for up to <histSize> previously read
* lines.
*
* RETURNS: The line-editor ID, or ERROR if the routine runs out of memory.
*
* SEE ALSO: ledRead(), ledClose(), ledControl()
*/

int ledOpen
    (
    int inFd,           /* low-level device input fd */
    int outFd,          /* low-level device output fd */
    int histSize        /* size of history list */
    )
    {
    LED_ID ledId = (LED_ID)calloc (1, sizeof (LED));

    if (ledId == NULL)
	return (ERROR);

    if ((ledId->buffer = (char *) malloc (LINE_LEN + 1)) == NULL)
	return (ERROR);

    ledId->inFd  = inFd;
    ledId->outFd = outFd;

    lstInit (&ledId->histList);
    lstInit (&ledId->histFreeList);

    histInit (ledId, histSize);

    return ((int) ledId);
    }
/*******************************************************************************
*
* ledClose - discard the line-editor ID
*
* This routine frees resources allocated by ledOpen().  The low-level
* input/output file descriptors are not closed.
*
* RETURNS: OK.
*
* SEE ALSO: ledOpen()
*/

STATUS ledClose
    (
    int led_id          /* ID returned by ledOpen */
    )
    {
    FAST LED_ID ledId = (LED_ID)led_id;

    lstFree (&ledId->histList);
    lstFree (&ledId->histFreeList);

    free (ledId->buffer);
    free (ledId);
    
    return (OK);
    }
/*******************************************************************************
*
* ledRead - read a line with line-editing
*
* This routine handles line-editing and history substitutions.
* If the low-level input file descriptor is not in OPT_LINE mode,
* only an ordinary read() routine will be performed.
*
* RETURNS: The number of characters read, or EOF.
*/

int ledRead
    (
    int led_id,         /* ID returned by ledOpen */
    char *string,       /* where to return line */
    int maxBytes        /* maximum number of chars to read */
    )
    {
    FAST LED_ID ledId = (LED_ID)led_id;
    FAST int ix;		/* the ubiquitous			*/
    char ch;			/* last character entered		*/
    BOOL overStrike = FALSE;	/* in over-strike mode			*/
    BOOL srchMode = FALSE;	/* entering search characters		*/
    BOOL done = FALSE;		/* return has been entered		*/
    int returnVal = EOF;	/* number of characters or EOF		*/
    int result;			/* return value from preempt		*/
    char oldLn  [LINE_LEN+1];	/* current line before most recent input*/
    char saveLn [LINE_LEN+1];	/* saved version of line before changes */
    char histLn [LINE_LEN+1];	/* last version of historical line	*/
    char *curLn;		/* current line, points to user's string*/
    int curPs = 0;		/* current cursor position		*/
    int lastPs = 0;		/* last position cursor was at		*/
    int number = 0;		/* repeat factor			*/
    FUNCPTR catchFunc;		/* preempt routine			*/
#ifndef	UNIX_DEBUG
    int oldoptions = ioctl (ledId->inFd, FIOGETOPTIONS, 0 /*XXX*/);

    /* XXX should be check for ERROR except an option might be 0xff,
     * perhaps inspect errno for valid 'fd', or something.
     */

    /* if not in line mode, just do plain read */

    if (!(oldoptions & OPT_LINE))
	return (read (ledId->inFd, string, maxBytes));

    /* turn off echo & line mode */
    ioctl (ledId->inFd, FIOSETOPTIONS, oldoptions & ~(OPT_LINE | OPT_ECHO));

#endif	/* UNIX_DEBUG */

    bzero (string, maxBytes);
    string [0] = EOS;		/* Beginning Of String */
    curLn = &string [1];	/* reserve position 0 for EOS (BOS) */
    saveLn [0] = EOS;
    histLn [0] = EOS;
    setPreempt (&catchFunc, (FUNCPTR) NULL);

    ledId->buffer [0] = EOS;
    ledId->cmdMode = FALSE;

    bfill (oldLn, LINE_LEN+1, EOS);
    bfill (curLn, maxBytes - 1, EOS);		/* -1 cause of BOS */

    /* operation:
     *    redraw line,
     *    read next character,
     *    based on command/input mode perform appropriate action,
     *    loop.
     */

    while (! done)
	{
	redraw (ledId->outFd, oldLn, lastPs, curLn, &curPs);

	lastPs = curPs;
	strcpy (oldLn, curLn);

	if (read (ledId->inFd, &ch, 1) < 1)
	    {
	    done = TRUE;
	    returnVal = EOF;
	    break;
	    }

	/* Ignore NULL in the input stream */

	if (ch == (char)NULL)
	    continue;

	/* some function wants to preempt? */

	result = preempt (&catchFunc, ch, curLn, &curPs, &number, ledId);

	if (result == ERROR)
	    {
	    beep (ledId->outFd);
	    continue;	/* short-cut loop */
	    }
	else if (result == OK)
	    continue;	/* short-cut loop */

	if (ledId->cmdMode)
	    {
	    /* parse command */

	    if (ch == tyBackspaceChar)
		ch = 'h';

	    if (ch == tyDeleteLineChar)
		{
		ledId->cmdMode = FALSE;
		curLn [0] = EOS;
		curPs = 0;
		}
	    else switch (ch)
		{
		case 'G':
		    save();

		    if (number < 1)
			number = 1;

		    if (histNum (ledId, curLn, number))
			curPs = 0;
		    else
			beep (ledId->outFd);
		    number = 0;
		    break;

		case '/':
		case '?':
		    strcpy (saveLn, curLn);
		    srchMode = TRUE;
		    ledId->cmdMode = FALSE;
		    curLn [0] = ch;
		    curLn [1] = EOS;
		    curPs = 1;
		    break;

		case 'n':
		case 'N':
		    if (histLn [0] == EOS)
			{
			beep (ledId->outFd);
			break;
			}

		    save ();

		    if (ch == 'N')
			histLn [0] = histLn [0] == '/' ? '?' : '/';

		    if (! histFind (ledId, histLn, curLn))
			{
			beep (ledId->outFd);
			curLn [0] = EOS;
			}

		    curPs = 0;
		    break;

		case 'k':
		case '-':
		    save ();

		    do
			{
			if (histPrev (ledId, curLn) == ERROR)
			    {
			    beep (ledId->outFd);
			    break;
			    }
			}
		    while (--number > 0);
		    number = 0;

		    curPs = 0;
		    break;

		case 'j':
		case '+':
		    save ();

		    do
			{
			if (histNext (ledId, curLn) == ERROR)
			    {
			    beep (ledId->outFd);
			    break;
			    }
			}
		    while (--number > 0);
		    number = 0;

		    curPs = 0;
		    break;

		case 'h':
		    do
			{
			if (curPs > 0)
			    curPs--;
			}
		    while (--number > 0);
		    number = 0;
		    break;

		case 'l':
		case ' ':
		    do
			{
			if ((strlen (curLn) != 0)&&(curPs < strlen (curLn) - 1))
			    curPs++;
			}
		    while (--number > 0);
		    number = 0;
		    break;

		case 'w':
		case 'W':	/* ignore punctuation */
		case 'b':
		case 'B':	/* ignore punctuation */
		case 'e':
		case 'E':	/* ignore punctuation */
		    {
		    int origPs = curPs;
		    int dir = (ch == 'b' || ch == 'B') ? BACKWARD :FORWARD;
		    BOOL word = ch != 'w' && ch != 'W';

		    do
			{
			search (isupper ((int)ch), word, curLn, &curPs, dir);
			}
		    while (curPs != origPs && --number > 0);
		    number = 0;
		    break;
		    }

		case 'f':
		case 't':	/* doesn't backward step
				 * XXX - not implemented */
		    setPreempt (&catchFunc, (FUNCPTR)findFwd);
		    break;

		case 'F':
		case 'T':	/* doesn't forward step
				 * XXX - not implemented */
		    setPreempt (&catchFunc, (FUNCPTR)findBwd);
		    break;

		case 'r':
		    strcpy (saveLn, curLn);
		    setPreempt (&catchFunc, (FUNCPTR)replace);
		    break;

		case 'R':
		    strcpy (saveLn, curLn);
		    overStrike = TRUE;
		    ledId->cmdMode = FALSE;
		    break;

		case 'A':
		    curPs = strlen (curLn);
		    /* drop thru */

		case 'a':
		    ledId->cmdMode = FALSE;
		    strcpy (saveLn, curLn);
		    if (curPs < strlen (curLn))
			curPs++;
		    break;

		case 'x':
		    strcpy (saveLn, curLn);
		    number = min (number, strlen (curLn) - curPs);
		    ix = 0;
		    do
			{
			ledId->buffer [ix++] = curLn [curPs];
			if (curPs < strlen (curLn) - 1)
			    strcpy (&curLn [curPs], &curLn [curPs + 1]);
			else
			    {
			    curLn [curPs] = EOS;
			    if (curPs > 0)
				curPs--;
			    else
				break;
			    }
			}
		    while (--number > 0);
		    number = 0;
		    ledId->buffer [ix] = EOS;
		    break;

		case 'X':
		    strcpy (saveLn, curLn);
		    ix = 0;
		    do
			{
			if (curPs > 0)
			    {
			    curPs--;
			    ledId->buffer [ix++] = curLn [curPs];
			    strcpy (&curLn [curPs], &curLn [curPs + 1]);
			    }
			else if (strlen (curLn) == 1)
			    {
			    ledId->buffer [ix++] = curLn [curPs];
			    curLn [curPs] = EOS;
			    }
			else
			    break;
			}
		    while (--number > 0);
		    number = 0;
		    ledId->buffer [ix] = EOS;
		    break;

		case 'c':
		    strcpy (saveLn, curLn);
		    setPreempt (&catchFunc, (FUNCPTR)change);
		    break;

		case 'C':
		    strcpy (saveLn, curLn);
		    ledId->cmdMode = FALSE;
		    curLn [curPs] = EOS;
		    break;

                case 's':
                    strcpy (saveLn, curLn);
		    number = number ? number : 1;
		    ix = 0;
                    if (number >= strlen (curLn) - curPs)
                        curLn [curPs] = EOS;
                    else
                        {
                        do
                            {
                            ledId->buffer [ix++] = curLn [curPs];
                            if (curPs < strlen (curLn) - 1)
                                strcpy (&curLn [curPs], &curLn [curPs + 1]);
                            else
                                {
                                curLn [curPs] = EOS;
                                if (curPs > 0)
                                    curPs--;
                                else
                                    break;
                                }
                            }
                        while (--number > 0);
                        }
                    number = 0;
                    ledId->buffer [ix] = EOS;
                    ledId->cmdMode = FALSE;
                    break;

		case 'S':
		    strcpy (saveLn, curLn);
		    if (change ('c', curLn, &curPs, &number, ledId) == ERROR)
			beep (ledId->outFd);
		    break;

		case 'd':
		    strcpy (saveLn, curLn);
		    setPreempt (&catchFunc, (FUNCPTR)deletec);
		    break;

		case 'D':
		    strcpy (saveLn, curLn);
		    strncpy (ledId->buffer, &curLn [curPs],
			     strlen (&curLn [curPs]) + 1); /* +EOS */
		    curLn [curPs] = EOS;
		    if (curPs > 0)
			curPs--;
		    break;

		case 'I':
		    curPs = 0;
		    /* drop thru */

		case 'i':
		    strcpy (saveLn, curLn);
		    number = 0;		/* can't handle "count" */
		    ledId->cmdMode = FALSE;
		    break;

		case 'p':
		    if (strlen (ledId->buffer) == 0)
			break;

		    if (strlen (curLn) == 0)
			goto caseP;		/* treat like 'P' */

		    number = 0;		/* can't handle "count" */

		    if (strlen (curLn) + strlen (ledId->buffer) < maxBytes)
			{
			strcpy (saveLn, curLn);

			if (++curPs < strlen (curLn))
			    {
			    /* insert buffer into curLn */

			    bcopy (&curLn [curPs],
				   &curLn [curPs + strlen (ledId->buffer)],
				   strlen (&curLn [curPs]) + 1); /* +EOS */

			    bcopy (ledId->buffer, &curLn [curPs],
				   strlen (ledId->buffer));
			    }
			else
			    /* buffer goes after curLn */
			    strcat (curLn, ledId->buffer);

			curPs += strlen (ledId->buffer) - 1;
			}
		    else
			beep (ledId->outFd);
		    break;

		case 'P':
		caseP:
		    number = 0;		/* can't handle "count" */

		    if (strlen (ledId->buffer) == 0)
			break;

		    if (strlen (curLn) + strlen (ledId->buffer) < maxBytes)
			{
			strcpy (saveLn, curLn);

			/* insert buffer into curLn */

			bcopy (&curLn [curPs],
			       &curLn [curPs + strlen (ledId->buffer)],
			       strlen (&curLn [curPs]) + 1); /* +EOS */

			bcopy (ledId->buffer, &curLn [curPs],
			       strlen (ledId->buffer));

			if (curPs == 0)
			    curPs--;
			curPs += strlen (ledId->buffer);
			}
		    else
			beep (ledId->outFd);
		    break;

		case 'u':
		case 'U':	/* restore original line
				 * XXX - not implemented */
		    if (strlen (saveLn) != 0)
			{
			bswap (curLn, saveLn,
			       max (strlen (curLn), strlen (saveLn)) + 1);
								  /* +EOS */
			if (curPs == strlen (saveLn) - 1 ||
			    curPs > strlen (curLn))
			    {
			    curPs = strlen (curLn) - 1;
			    }
			}
		    else
			beep (ledId->outFd);
		    break;

		case RET_CHAR:
		    done = TRUE;
		    returnVal = strlen (curLn);
		    break;

		case '$':
		    curPs = strlen (curLn) - 1;
		    break;

		case '0':
		    if (number == 0)
			{
			curPs = 0;
			break;
			}
		    /* drop thru */

		case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
		    if (number < 0)
			number = 0;
		    number = number * 10 + (ch - '0');
		    break;

		case '~':
		    strcpy (saveLn, curLn);
		    ch = curLn [curPs];
		    if (isalpha ((int)ch))
			curLn [curPs] = islower ((int)ch) ? toupper ((int)ch)
			    : tolower (ch);
		    if (curPs < strlen (curLn) - 1)
			curPs++;
		    break;

		case '^':
		    for (curPs = 0;
			 curLn [curPs] != EOS && isspace ((int)curLn[curPs]);
			 curPs++)
			 ;
		    break;

		case RDW_CHAR:
		    write (ledId->outFd, "\n", 1);
		    (void)writen (ledId->outFd, ' ', curPs);
		    strcpy (oldLn, "");
		    break;

		case CMP_CHAR:
		    strcpy (saveLn, curLn);

		    if (! completeName (curLn, &curPs))
			beep (ledId->outFd);
		    break;

		default:
		    beep (ledId->outFd);
		    break;
		}
	    }
	else
	    {
	    /* input mode */

	    if (ch == tyEofChar && curPs == 0)
		{
		/* only return EOF when used at the beginning of line */

		done = TRUE;
		returnVal = EOF;
		}
	    else if (ch == tyBackspaceChar)
		{
		if (curPs > 0)
		    {
		    strcpy (&curLn [curPs - 1], &curLn [curPs]);
		    curPs--;
		    }
		}
	    else if (ch == tyDeleteLineChar)
		{
		curLn [0] = EOS;
		curPs = 0;
		}
	    else switch (ch)
		{
		case ESC_CHAR:
		    overStrike = FALSE;
		    if (! srchMode)
			{
			ledId->cmdMode = TRUE;
			if (curPs > 0)
			    curPs--;
			break;
			}

		    /* drop thru */

		case RET_CHAR:
		    if (srchMode)
			{
			if (strcmp (curLn, "/") == 0 || strcmp (curLn, "?") ==0)
			    /* just use previous history search */
			    strcpy (curLn, histLn);
			else
			    /* new history search */
			    strcpy (histLn, curLn);

			if (! histFind (ledId, histLn, curLn))
			    {
			    curLn [0] = EOS;
			    beep (ledId->outFd);
			    }

			curPs = 0;
			srchMode = FALSE;
			ledId->cmdMode = TRUE;
			}
		    else
			{
			done = TRUE;
			returnVal = strlen (curLn);
			}
		    break;

		case CMP_CHAR:
		    strcpy (saveLn, curLn);

		    if (! completeName (curLn, &curPs))
			beep (ledId->outFd);
		    else
			/*
			if (isalpha(curLn [curPs]) || isdigit(curLn[curPs]) ||
				curLn [curPs] == '_')
			*/
			while (isalpha((int)curLn [curPs]) ||
				isdigit((int)curLn[curPs]) ||
				curLn [curPs] == '_')
			    {
			    curPs++;
			    }
		    break;

		default:
		    if (strlen (curLn) < maxBytes)
			{
			if (overStrike && curPs < strlen (curLn))
			    curLn [curPs++] = ch;
			else
			    {
			    bcopy (&curLn [curPs], &curLn [curPs + 1],
				   strlen (&curLn [curPs]) + 1); /* + EOS */
			    curLn [curPs++] = ch;
			    }
			}
		    else
			beep (ledId->outFd);
		    break;
		}
	    }
	}

    if (returnVal == EOF)
	{
#ifndef	UNIX_DEBUG
	/* turn echo & line mode back on */
	ioctl(ledId->inFd, FIOSETOPTIONS, oldoptions);
#endif	/* UNIX_DEBUG */

	return (EOF);
	}

    histAdd (ledId, curLn);
    write (ledId->outFd, "\n", 1);

    /* must fix up BOS */
    bcopy (&string [1], &string [0], strlen (curLn) + 1);	/* +EOS */

#ifndef	UNIX_DEBUG
    /* turn echo & line mode back on */
    ioctl (ledId->inFd, FIOSETOPTIONS, oldoptions);
#endif	/* UNIX_DEBUG */

    return (returnVal);
    }
/*******************************************************************************
*
* ledControl - change the line-editor ID parameters
*
* This routine changes the input/output file
* descriptor and the size of the history list.
*
* RETURNS: N/A
*/

void ledControl
    (
    int led_id,    /* ID returned by ledOpen */
    int inFd,      /* new input fd (NONE = no change) */
    int outFd,     /* new output fd (NONE = no change) */
    int histSize   /* new history list size (NONE = no change), (0 = display) */
    )
    {
    FAST LED_ID ledId = (LED_ID)led_id;

    if (inFd >= 0)
	ledId->inFd = inFd;
    if (outFd >= 0)
	ledId->outFd = outFd;
    if (histSize == 0)
	histAll (ledId);
    if (histSize > 0)
	histInit (ledId, histSize);
    }

/*******************************************************************************
*
* setPreempt - set function to use next
*/

LOCAL void setPreempt
    (
    FAST FUNCPTR *catchFunc,
    FAST FUNCPTR func
    )
    {
    *catchFunc = func;
    }
/*******************************************************************************
*
* preempt - perform preempt function if set
*
* If <catchFunc> is non-NULL then it is called using the
* remaining arguments.
*
* RETURNS:
*    result of preempt function, or
*    1 if no function called
*/

LOCAL int preempt
    (
    FAST FUNCPTR *catchFunc,    /* preempting function */
    char ch,                    /* arguments to be */
    char *curLn,                /* passed to th */
    int *curPs,                 /* routine ... */
    int *number,
    LED_ID ledId
    )
    {
    FUNCPTR tmpFunc = *catchFunc;

    if (*catchFunc == (FUNCPTR)NULL)
	return (1);	/* no function called */

    /* clear the catch */

    *catchFunc = (FUNCPTR) NULL;

    return ((*tmpFunc) (ch, curLn, curPs, number, ledId));
    }
/*******************************************************************************
*
* redraw - redraw line
*
* From the previous line and position (oldLn & lastPs) redraw line to
* reflect reality (curLn & curPs).
*
* Operation: compare new line with previous line,
* optimize the number of characters redrawn and cursor motion.
*/

LOCAL void redraw
    (
    int outFd,
    char *oldLn,
    int lastPs,
    char *curLn,
    int *curPs
    )
    {
    FAST int ok1Ps;	/* line is the same up to this position */
    FAST int ok2Ps;	/* line is the same from the far end to here */

    /* find first difference */
    for (ok1Ps = 0;
	 curLn [ok1Ps] == oldLn [ok1Ps] && curLn [ok1Ps] != EOS;
	 ok1Ps++)
	;

    /* find first difference from the far end if same length */
    if (strlen (curLn) == strlen (oldLn))
	{
	for (ok2Ps = strlen (curLn);
	     curLn [ok2Ps] == oldLn [ok2Ps] && ok2Ps > 0;
	     ok2Ps--)
	    ;
	if (ok2Ps < strlen (curLn))
	    ok2Ps++;
	}
    else
	ok2Ps = strlen (curLn);

    /* line is bad before cursor position */
    if (ok1Ps < lastPs)
	{
	/* back up */
	(void)writen (outFd, '\b', lastPs - ok1Ps);

	/* redraw line from ok1Ps to the good portion */
	(void)writex (outFd, &curLn [ok1Ps], ok2Ps - ok1Ps);

	lastPs = ok2Ps;
	}

    /* line is bad after cursor position */
    else if (ok2Ps > lastPs)
	{
	/* draw line from lastPs to the good portion */
	(void)writex (outFd, &curLn [lastPs], ok2Ps - lastPs);
	lastPs = ok2Ps;
	}

    /* line is bad at cursor position */
    else if (ok2Ps == lastPs && lastPs < strlen (curLn))
	{
	/* draw single character */
	(void)writex (outFd, &curLn [lastPs], 1);
	lastPs++;
	}

    /* blank out remaining junk if we're at the end of the line */
    if (lastPs == strlen (curLn))
	{
	int i = strlen (oldLn) - strlen (curLn);

	if (i > 0)
	    {
	    (void)writen (outFd, ' ', i);
	    (void)writen (outFd, '\b', i);
	    }
	}

    /* now get cursor right */

    if (lastPs > *curPs)
	(void)writen (outFd, '\b', lastPs - *curPs);
    else
	(void)writex (outFd, &curLn [lastPs], *curPs - lastPs);
    }

/*
 * Definitions used through remaining routines.
 */
#define CUR			(curLn [*curPs])
#define PREV			(curLn [*curPs-dir])
#define NEXT			(curLn [*curPs+dir])
#define INC			(*curPs+=dir)
#define ispunct_nospc(c)	(ispunct(c) && !isspace(c))
#define igpunct(c)		(ignorePunct ? ispunct_nospc(c) : FALSE)
#define isword(c)	(isalpha(c) || isdigit(c) || index("*?_-.[]~=",c))

/*******************************************************************************
*
* completeName - complete partial symbol name at current position
*
* RETURNS: TRUE if symbol name is completed in any way, otherwise FALSE.
*/

LOCAL BOOL completeName
    (
    FAST char *curLn,
    int *curPs
    )
    {
    CMP_ARG arg;
    char saveLn [LINE_LEN+1];
    char string [100];
    int count;
    int dir	= FORWARD;
    int startPs = *curPs;
    int endPs	= *curPs;

    /* operation:
     *   isolate begin and end of string at current position,
     *   if begin and end are equal then no string - return error,
     *   call support routine to see if string is in symbol table,
     *     if not then - return error
     *	   otherwise determine symbol up to the last unambiguous character,
     *     then insert extra characters - return ok.
     */

    while (isalpha((int)curLn [startPs-1]) ||
	   isdigit((int)curLn[startPs-1])  ||
	   curLn [startPs-1] == '_')
	{
	startPs--;
	}

    while (isalpha ((int)curLn [endPs]) ||
	   isdigit ((int)curLn [endPs]) ||
	   curLn [endPs] == '_')
	{
	endPs++;
	}

    if (endPs == startPs)
	return (FALSE);

    if (curLn [startPs] == '_')
	startPs++;		/* ignore leading underscore */

    strncpy (string, &curLn [startPs], endPs - startPs);
    string [endPs - startPs] = EOS;

    arg.name    = string;		/* search string */
    arg.nameLen = strlen (string);	/* optimization */
    arg.count   = 0;			/* reset to zero */

    symEach (sysSymTbl, (FUNCPTR)findName, (int)&arg);
    count = arg.count;

    /* keep looking while match is ambiguous */

    while (arg.count > 1 && count == arg.count)
	{
	if (strcmp (arg.name, arg.match) == 0)
	    {
	    /* just in case more than one entry with same name,
	     * but hope this first match is different, oh well.
	     */
	    count = 1;
	    break;
	    }
	arg.count = 0;		/* reset to zero */
	arg.nameLen++;		/* optimization */

	strncpy (arg.name, arg.match, arg.nameLen);

	symEach (sysSymTbl, (FUNCPTR)findName, (int)&arg);
	}

    /* have to admit we didn't find anything */

    if (count == 0)
	return (FALSE);

    if (count > 1)
	{
	/* ambiguous match up 'til second last position */
	arg.name [arg.nameLen - 1] = EOS;
	arg.match = arg.name;
	}

    /* insert new string, don't forget to save original line */

    if ((endPs - startPs) + strlen (curLn) + strlen (arg.match) < LINE_LEN)
	{
	strcpy (saveLn, curLn);

	strcpy (&curLn [startPs], arg.match);
	strcat (curLn, &saveLn [endPs]);

	if (isalpha((int)CUR) || isdigit((int)CUR) || CUR == '_')
	    {
	    while (isalpha((int)NEXT) || isdigit((int)NEXT) || NEXT == '_')
		{
		INC;
		}
	    }
	}

    return (TRUE);
    }
/*******************************************************************************
*
* findName - support routine for completeName
*
* This routine is called for each string in the symbol table.
*
* RETURNS: TRUE always.
*
* ARGSUSED
*/

LOCAL BOOL findName
    (
    char *name,         /* name of this entry */
    char *value,        /* symbol table junk */
    SYM_TYPE type,      /* symbol table junk */
    CMP_ARG *arg        /* what to match, etc. */
    )
    {
    if (name[0] == '_')
	name++;		/* ignore leading underscore */

    if (strncmp (name, arg->name, arg->nameLen) == 0)
	{
	if (++arg->count == 1 || strlen (name) > strlen (arg->match))
	    arg->match = name;
	}

    return (TRUE);
    }
/*******************************************************************************
*
* search - move from current position to matched character position
*
* RETURNS: new ptr to current position.
*/

LOCAL void search
    (
    FAST BOOL ignorePunct,              /* ignore punctuation or not */
    FAST BOOL endOfWord,                /* stop at end of word, or
                                           when BACKWARD stop at beginning */
    char *curLn,
    int *curPs,
    int dir                     /* direction to search */
    )
    {
    /* make sure we're not already at limits */

    if (CUR == EOS || NEXT == EOS)
	return;

    if (isspace ((int)CUR))
	{
	/* just get to a non-space */
	while (NEXT != EOS && isspace ((int)CUR))
	    INC;
	}
    else if (ispunct ((int)CUR))
	{
	/* just get to the next/previous word */
	while (NEXT != EOS && !isalnum ((int)CUR))
	    INC;
	}
    else if (endOfWord || dir == BACKWARD)
	{
	/* while space coming */
	while (isspace((int)NEXT))
	    INC;

	/* maybe we've got to punctuation */
	if (ispunct_nospc ((int)NEXT))
	    {
	    do
		{
		INC;
		}
	    while (NEXT != EOS && ispunct_nospc ((int)NEXT));

	    /* while we're on a space advance/retreat */
	    while (NEXT != EOS && isspace ((int)CUR))
		INC;

	    return;
	    }

	/* get to end/beginning of word but not past */
	while (NEXT != EOS && (isalnum ((int)NEXT) || igpunct(((int)NEXT))))
	    INC;
	}
    else
	{
	/* get past end/beginning of word */

	do
	    {
	    INC;
	    }
	while (NEXT != EOS && (isalnum ((int)CUR) || igpunct((int)CUR)));

	/* while we're on a space advance/retreat */
	while (NEXT != EOS && isspace ((int)CUR))
	    INC;
	}
    }
/*******************************************************************************
*
* find - a particualar character
*
* RETURNS:
* TRUE if `ch' found, or
* FALSE if end of line reached first.
*/

LOCAL BOOL find
    (
    char ch,                    /* what to find */
    char *curLn,
    int *curPs,
    int dir                     /* direction to search */
    )
    {
    if (CUR == EOS || NEXT == EOS)
	return (FALSE);

    do
	{
	INC;
	}
    while (CUR != ch && NEXT != EOS);

    return (CUR == ch);
    }
/*******************************************************************************
*
* findFwd - find <ch> some <number> of times looking forwards
*/

LOCAL STATUS findFwd
    (
    FAST char ch,
    char *curLn,
    int *curPs,
    int *number
    )
    {
    while (find (ch, curLn, curPs, FORWARD) && --*number > 0)
	;

    *number = 0;

    return (OK);
    }
/*******************************************************************************
*
* findBwd - find <ch> some <number> of times looking backwards
*/

LOCAL STATUS findBwd
    (
    char ch,
    char *curLn,
    int *curPs,
    int *number
    )
    {
    while (find (ch, curLn, curPs, BACKWARD) && --*number > 0)
	;
    *number = 0;

    return (OK);
    }
/*******************************************************************************
*
* change - change as specified by (w/W/e/E/sp/l/h/c/$/0)
*/

LOCAL STATUS change
    (
    FAST char ch,
    char *curLn,
    int *curPs,
    int *number,
    LED_ID ledId
    )
    {
    STATUS status = OK;

    switch (ch)
	{
	case '0':
	case ' ':
	case 'l':
	case 'h':
	case 'w':
	case 'W':
	case 'e':
	case 'E':
	case 'b':
	case 'B':
	    ledId->cmdMode = FALSE;
	    status = deletec (ch, curLn, curPs, number, ledId);
	    break;

	case 'c':
	    *curPs = 0;
	    /* drop through */

	case '$':
	    ledId->cmdMode = FALSE;
	    CUR = EOS;
	    break;

	default:
	    status = ERROR;
	    break;
	}

    return (status);
    }
/*******************************************************************************
*
* deletec - delete as specified by (w/W/e/E/b/B/sp/l/h/d/$/0) <number> of times
*
* RETURNS: OK or ERROR
*/

LOCAL STATUS deletec
    (
    FAST char ch,
    char *curLn,
    int *curPs,
    int *number,
    LED_ID ledId
    )
    {
    STATUS status = OK;
    FAST int tmp = *curPs;
    FAST int i = 0;
    int dir = (ch == 'b' || ch == 'B' || ch == 'h') ? BACKWARD : FORWARD;
    BOOL endOfWord = ch != 'w' && ch != 'W';
    int fudge;				/* fudge factor? */

    do
	{
	switch (ch)
	    {
	    case 'w':
	    case 'W':		/* ignore punctuation */
	    case 'e':
	    case 'E':		/* ignore punctuation */
	    case 'b':
	    case 'B':		/* ignore punctuation */
		{
		int origPs = *curPs;

		search (isupper ((int)ch), endOfWord, curLn, curPs, dir);

		if (*curPs == origPs)
		    *number = 1;			/* get out! */

		fudge = (NEXT == EOS) ? 1 : 0;

		if (dir == FORWARD)
		    {
		    i = *curPs - tmp + fudge;
		    strncpy (ledId->buffer, &curLn [tmp], i);
		    strcpy (&curLn [tmp], &curLn [*curPs+fudge]);
		    *curPs = tmp;
		    }
		else
		    {
		    i = tmp - *curPs + fudge;
		    strncpy (ledId->buffer, &CUR, i);
		    strcpy (&CUR, &curLn [tmp+fudge]);
		    tmp = *curPs - fudge;
		    }
		break;
		}

	    case ' ':
	    case 'l':
		ledId->buffer [i++] = CUR;
		if (*curPs < strlen (curLn) - 1)
		    {
		    strcpy (&CUR, &NEXT);
		    break;
		    }
		else if (*curPs == strlen (curLn) - 1)
		    *number = 1;		/* get out! */

		CUR = EOS;
		if (*curPs > 0)
		    (*curPs)--;
		break;

	    case 'd':
		strcpy (ledId->buffer, curLn);
		i = strlen (ledId->buffer);
			    /* don't want 'i' to cause any grief */
		*curPs = 0;
		CUR = EOS;
		*number = 1;		/* get out! */
		break;

	    case '$':
		i = strlen (curLn) - *curPs + 1;
		strncpy (ledId->buffer, &CUR, i);
		CUR = EOS;
		*number = 1;		/* get out! */
		break;

	    case '0':
		i = *curPs + 1;
		strncpy (ledId->buffer, &curLn [0], i);
		bcopy (&CUR, &curLn [0], strlen (&CUR) + 1); /* +EOS */
		*curPs = 0;
		*number = 1;		/* get out! */
		break;

	    case 'h':	/* XXX - not implemented */
	    default:
		i = strlen (ledId->buffer);
		*number = 1;		/* get out! */
		status = ERROR;
		break;
	    }
	}
    while (--*number > 0);

    *number = 0;

    ledId->buffer [i] = EOS;

    return (status);
    }
/*******************************************************************************
*
* replace - replace current position thru count with specified character
*/

LOCAL STATUS replace
    (
    char ch,
    char *curLn,
    int *curPs,
    int *number
    )
    {
    if (*number < 1)
	*number = 1;

    /* don't do replacement if escape */
    if (ch != ESC_CHAR)
	bfill (&CUR, min (*number, strlen (&CUR)), ch);

    *number = 0;

    return (OK);
    }
/*******************************************************************************
*
* beep - make a noise
*/

LOCAL void beep
    (
    int outFd
    )
    {
    static char bell = BEL_CHAR;

    write (outFd, &bell, 1);
    }
/*******************************************************************************
*
* writex - write bytes to a file but change control chars to '?'
*
* RETURNS: number of characters actually written, pretty much like write.
*/

LOCAL int writex
    (
    FAST int fd,
    char buffer [],
    FAST int nbytes
    )
    {
    FAST int i;
    FAST char *pBuf = buffer;
    char non_ch = NON_CHAR;

    for (i = 0; i < nbytes; i++, pBuf++)
	{
	if (write (fd, (isprint ((int)(*pBuf)) ? pBuf : &non_ch), 1) != 1)
	    return (ERROR);
	}

    return (nbytes);
    }
/*******************************************************************************
*
* writen - write a character n times
*
* NOTE: requires n <= (LINE_LEN + 1)
*
* RETURNS: number of characters actually written, just like write.
*/

LOCAL int writen
    (
    int fd,
    char ch,
    int nbytes
    )
    {
    char buf [LINE_LEN + 1];

    bfill (buf, nbytes, ch);
    return (write (fd, buf, nbytes));
    }

/*******************************************************************************
*
* histInit - initialize history list
*
* On successive calls, resets history to new size.
*/

LOCAL void histInit
    (
    LED_ID ledId,
    int histSize        /* amount of history required */
    )
    {
    int histDiff = histSize - ledId->histSize;
    HIST *pH;

    if (ledId->histSize == 0)
	{
	lstFree (&ledId->histList);
	lstFree (&ledId->histFreeList);

	ledId->histNum  = 0;

	lstInit (&ledId->histList);
	lstInit (&ledId->histFreeList);
	}

    if (histDiff >= 0)
	{
	/* add to history free list */

	while (histDiff-- > 0)
	    {
	    /* allocate history space */

	    ledId->pHist = (HIST *) malloc (sizeof (HIST));

	    if (ledId->pHist == NULL)
		return;		/* oh, oh! */

	    ledId->pHist->line[0] = EOS;

	    lstAdd (&ledId->histFreeList, &ledId->pHist->node);
	    }
	}

    else
	{
	/* free excess histories from history free list */
	while (lstCount (&ledId->histFreeList) > 0 && histDiff++ < 0)
	    {
	    pH = (HIST *) lstGet (&ledId->histFreeList);
	    free ((char *) pH);
	    }

	/* free oldest histories from history list */
	while (lstCount (&ledId->histList) > 0 && histDiff++ < 0)
	    {
	    pH = (HIST *) lstGet (&ledId->histList);
	    free ((char *) pH);
	    }
	}

    ledId->histSize = histSize;
    }
/*******************************************************************************
*
* histAdd - add line to history
*/

LOCAL void histAdd
    (
    FAST LED_ID ledId,
    FAST char *line
    )
    {
    FAST HIST *pH = (HIST *) lstLast (&ledId->histList);
    FAST int i;

    ledId->pHist = NULL;

    /* ignore blank lines */
    for (i = 0; isspace ((int)line[i]); i++)
	;

    /* ignore blank lines & consecutive identical commands */

    if (i >= strlen (line) || (pH != NULL && strcmp (line, pH->line) == 0))
	return;

    /* try free list or get from top of history list */

    if ((pH = (HIST *) lstGet (&ledId->histFreeList)) == NULL &&
	(pH = (HIST *) lstGet (&ledId->histList)) == NULL)
	{
	/* history foul-up! */
	return;
	}

    /* append to end of history list */

    lstAdd (&ledId->histList, &pH->node);

    ledId->histNum++;

    strncpy (pH->line, line, LINE_LEN);
    pH->line [LINE_LEN - 1] = EOS;
    }
/*******************************************************************************
*
* histNum - get historical line number
*
* RETURNS: FALSE if no history by that number
*/

LOCAL BOOL histNum
    (
    FAST LED_ID ledId,
    FAST char *line,            /* where to return historical line */
    FAST int n                  /* history number */
    )
    {
    int histFirst = ledId->histNum - lstCount (&ledId->histList) + 1;
    FAST HIST *pH;
    FAST int i;

    if (n < histFirst || n > ledId->histNum)
	return (FALSE);

    for (pH = (HIST *) lstFirst (&ledId->histList), i = histFirst;
	 pH != NULL && i != n;
	 pH = (HIST *) lstNext (&pH->node), i++)
	;

    if (pH == NULL)
	return (FALSE);

    strcpy (line, pH->line);

    ledId->pHist = pH;

    return (TRUE);
    }
/*******************************************************************************
*
* histNext - get next entry in history
*
* If line == NULL then just forward history.
*
* RETURNS: ERROR if no history
*/

LOCAL STATUS histNext
    (
    FAST LED_ID ledId,
    FAST char *line             /* where to return historical line */
    )
    {
    if (ledId->pHist == NULL)
	ledId->pHist = (HIST *) lstFirst (&ledId->histList);
    else if ((ledId->pHist = (HIST *) lstNext (&ledId->pHist->node)) == NULL)
	ledId->pHist = (HIST *) lstFirst (&ledId->histList);

    if (ledId->pHist == NULL)
	return (ERROR);

    if (line != NULL)
	strcpy (line, ledId->pHist->line);

    return (OK);
    }
/*******************************************************************************
*
* histPrev - get previous entry in history
*
* If line == NULL then just forward history.
*
* RETURNS: ERROR if no history
*/

LOCAL STATUS histPrev
    (
    FAST LED_ID ledId,
    FAST char *line             /* where to return historical line */
    )
    {
    if (ledId->pHist == NULL)
	ledId->pHist = (HIST *) lstLast (&ledId->histList);
    else if ((ledId->pHist = (HIST *) lstPrevious (&ledId->pHist->node))
								    == NULL)
	{
	ledId->pHist = (HIST *) lstLast (&ledId->histList);
	}

    if (ledId->pHist == NULL)
	return (ERROR);

    if (line != NULL)
	strcpy (line, ledId->pHist->line);

    return (OK);
    }
/*******************************************************************************
*
* histFind - find string in history list
*
* Match should be of form: "/test", or "?test" for forward or backward searches.
*
* RETURNS: TRUE if history found, line is replaced with historical line
*/

LOCAL BOOL histFind
    (
    FAST LED_ID ledId,
    FAST char *match,           /* what to match */
    FAST char *line             /* where to return historical line */
    )
    {
    char *pLine;
    FAST HIST *pH = ledId->pHist;
    FAST BOOL forward = match [0] == '?';
    int i = lstCount (&ledId->histList) + 1;
    BOOL found = FALSE;

    if (pH == NULL && (pH = (HIST *) lstLast (&ledId->histList)) == NULL)
	return (FALSE);

    do
	{
	if (forward)
	    {
	    if ((pH = (HIST *) lstNext (&pH->node)) == NULL &&
		(pH = (HIST *) lstFirst (&ledId->histList)) == NULL)
		return (FALSE);
	    }

	pLine = pH->line;

	while ((pLine = index (pLine, match [1])) != 0)
	    {
	    if (strncmp (pLine, &match [1], strlen (match) - 1) == 0)
		{
		found = TRUE;
		break;	/* found it! */
		}
	    else if (*(++pLine) == EOS)
		break;
	    }

	if (!found && !forward)
	    {
	    if ((pH = (HIST *) lstPrevious (&pH->node)) == NULL &&
		(pH = (HIST *) lstLast (&ledId->histList)) == NULL)
		return (FALSE);
	    }
	}
    while (--i > 0 && !found);

    /* position 'pHist' correctly for next search */

    if (!forward)
	{
	if ((ledId->pHist = (HIST *) lstPrevious (&pH->node)) == NULL)
	    ledId->pHist = (HIST *) lstLast (&ledId->histList);
	}
    else
	ledId->pHist = pH;

    if (i < 1)
	return (FALSE);

    strcpy (line, pH->line);

    return (TRUE);
    }
/*******************************************************************************
*
* histAll - print all historical lines
*/

LOCAL void histAll
    (
    FAST LED_ID ledId
    )
    {
    FAST int histFirst = ledId->histNum - lstCount (&ledId->histList) + 1;
    FAST HIST *pH;
    char buffer [LINE_LEN + 10]; /* Need to add padding for the
				  * line number
				  */

    for (pH = (HIST *) lstFirst (&ledId->histList);
	 pH != NULL;
	 pH = (HIST *) lstNext (&pH->node))
	{
	sprintf (buffer, "%3d  %s", histFirst++, pH->line);
	(void)writex (ledId->outFd, buffer, strlen (buffer));
	write (ledId->outFd, "\n", 1);
	}
    }

#ifdef	UNIX_DEBUG

/*******************************************************************************
*
* main - UNIX debug module
*
* NOMANUAL
*/

void main ()

    {
    static char *junk[] =
	    { "one", "two", "three", "four", "five", "six", "seven",
	      "eight", "nine", "ten", "force", "fits", "fight", "several" };
    char line [LINE_LEN+1];
    int i;
    int ledId;


    setbuf (stdout, NULL);

    sysSymTbl = symTblCreate (6);

    for (i = 0; i < NELEMENTS (junk); i++)
	symSAdd (sysSymTbl, junk[i], 0, 0, symGroupDefault);

    ledId = ledOpen (0, 1, 20);

    system ("stty cbreak -echo");

    printf ("ledLib: UNIX Debug\n");

    printf ("-> ");

    while (ledRead (ledId, line, LINE_LEN) != EOF)
	{
	printf ("-> ");
	}

    system ("stty -cbreak echo");
    }
/*******************************************************************************
*
* errnoSet - bogus edition
*
* NOMANUAL
*/

void errnoSet (status)
    int status;

    {
    printf ("errnoSet: new status 0x%x.\n", status);
    }
#endif	/* UNIX_DEBUG */

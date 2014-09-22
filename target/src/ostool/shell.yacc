%{
/* shell.yacc - grammar for VxWorks shell */

/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
07m,16oct01,jn   use symFindSymbol for symbol lookups
07l,27nov01,pch  Provide floating-point exclusion based on _WRS_NO_TGT_SHELL_FP
		 definition instead of testing a specific CPU type.
07k,23oct01,fmk  Do not call symFindByValue and print symbol name if symbol
                 value = -1 SPR 22254
07j,04sep98,cdp  apply 07i for all ARM CPUs with ARM_THUMB==TRUE.
07i,30jul97,cdp  for ARM7TDMI_T, force calls to be in Thumb state.
07h,31may96,ms   added in patch for SPR 4439.
07g,19mar95,dvs  removed tron references.
07f,02mar95,yao  removed floating point temporarily for PPC403.
07e,19mar95,dvs  removed tron references.
07d,13feb93,kdl  changed cplusLib.h to private/cplusLibP.h (SPR #1917).
07c,03sep92,wmd  modified addArg() to pass floats correcty for the i960.
07b,03sep92,rrr  reduced max function arguments from 16 to 12 (for i960).
07a,31aug92,kdl  fixed passing of more than 10 parameters during funcCall();
		 changed MAX_ARGS to MAX_SHELL_ARGS.
06z,19aug92,jmm  fixed problem with not recognizing <= (SPR 1517)
06y,01aug92,srh  added C++ demangling idiom to printSym.
                 added include of cplusLib.h.
06x,20jul92,jmm  added group parameter to symAdd call
06w,23jun92,kdl  increased max function arguments from 10 to 16.
06v,22jun92,jmm  backed out 6u change, now identical to gae's 21dec revision
06u,22jun92,jmm  added group parameter to symAdd
06t,21dec91,gae  more ANSI cleanups.
06s,19nov91,rrr  shut up some warnings.
06r,05oct91,rrr  changed strLib.h to string.h
06q,02jun91,del  added I960 parameter alignment fixes.
06p,10aug90,kdl  added forward declarations for functions returning VOID.
06o,10jul90,dnw  spr 738: removed checking of access (checkAccess, chkLvAccess)
		   Access checking did vxMemProbe with 4 byte read, which
		   caused problems with memory or devices that couldn't do
		   4 byte reads but could do other types of access.
		   Access checking was actually a throw-back to a time when
		   the shell couldn't recover from bus errors, but it can now.
		   So I just deleted the access checking.
		 lint clean-up, esp to allow VOID to be void one day.
06n,10dec89,jcf  symbol table type now a SYM_TYPE.
06m,09aug89,gae  fixed copyright notice.
06l,30jul89,gae  changed obsolete sysMemProbe to vxMemProbe. 
06k,07jul88,jcf  changed malloc to match new declaration.
06j,30may88,dnw  changed to v4 names.
06i,01apr88,gae  made it work with I/O system changes -- io{G,S}etGlobalStd().
06h,20feb88,dnw  lint
06g,14dec87,dnw  removed checking for odd byte address access.
		 changed printing format of doubles from "%f" to "%g". 
06f,18nov87,gae  made assignment to be of type specified by rhs.
06e,07nov87,gae  fixed undefined symbol bug.
		 fixed history bug by redirecting LED I/O.
06d,03nov87,ecs  documentation.
06c,28oct87,gae  got rid of string type.
06b,06oct87,gae  split off "execution" portion to shellExec.c.
		 changed to use conventional C type casting.
		 provided more info for invalid yacc operations.
		 allowed expressions to be function addresses.
06a,01jun87,gae  added interpretation of bytes, words, floats, doubles;
		   expressions can now be "typed" a la assembler, .[bwfdls].
		 fixed redirection bug with ">>" and "<<".
05i,16jul87,ecs  fixed newSym so that new symbols will be global.
05h,01apr87,gae  made assign() not print "new value" message (duplicated
		   normal "value" message for expressions.
05g,25apr87,gae  fixed bug in assign() that allowed memory corruption.
		 checked h() parameter for greater than or equal to zero.
		 improved redirection detection.
		 now parse assignments correctly as expressions.
05f,01apr87,ecs  added include of strLib.h.
05e,20jan87,jlf  documentation.
05d,14jan87,gae  got rid of unused curLineNum.  h() now has parameter, if
		   non-zero then resets history to that size.
05c,20dec86,dnw  changed to not get include files from default directories.
05b,18dec86,gae  made history initialization only happen on first start of
		   shell.  Added neStmt to fix empty stmt assignment bug.
05a,17dec86,gae	 use new shCmd() in execShell() to do Korn shell-like input.
04q,08dec86,dnw  changed shell.slex.c to shell_slex.c for VAX/VMS compatiblity.
	    jlf  fixed a couple bugs causing problems mainly on Heurikon port.
04p,24nov86,llk  deleted SYSTEM conditional compiles.
04o,08oct86,gae  added C assignment operators; allowed multiple assignment.
		 STRINGs are no longer temporary.  Added setShellPrompt().
04n,27jul86,llk  added standard error fd, setOrigErrFd.
04m,17jun86,rdc  changed memAllocates to mallocs.
04l,08apr86,dnw  added call to vxSetTaskBreakable to make shell unbreakable.
		 changed sstLib calls to symLib.
04k,02apr86,rdc  added routines setOrigInFd and setOrigOutFd.
04j,18jan86,dnw  removed resetting (flushing) for standard in/out upon restarts;
		   this is now done more appropriately by the shell restart
		   routine in dbgLib.
...deleted pre 86 history - see RCS
*/

/*
DESCRIPTION
This is the parser for the VxWorks shell, written in yacc.
It provides the basic programmer's interface to VxWorks.
It is a C expression interpreter, containing no built-in commands.  

SEE ALSO: "Shell"
*/

#include "vxWorks.h"
#include "sysSymTbl.h"
#include "errno.h"
#include "errnoLib.h"
#include "ioLib.h"
#include "taskLib.h"
#include "stdio.h"
#include "private/cplusLibP.h"

#define YYSTYPE VALUE		/* type of parse stack */

#define	MAX_SHELL_LINE	128	/* max chars on line typed to shell */

#define MAX_SHELL_ARGS	30	/* max number of args on stack */
#define MAX_FUNC_ARGS	12	/* max number of args to any one function */
				/*  NOTE: The array indices in funcCall()
				 *        must agree with MAX_FUNC_ARGS!!
				 */

#define BIN_OP(op)	rvOp((getRv(&yypvt[-2], &tmpVal1)), op, \
			      getRv(&yypvt[-0], &tmpVal2))

#define	RV(value)	(getRv (&(value), &tmpVal2))
#define NULLVAL		(VALUE *) NULL

#define CHECK		if (semError) YYERROR
#define SET_ERROR	semError = TRUE


typedef enum		/* TYPE */
    {
    T_UNKNOWN,
    T_BYTE,
    T_WORD,
#ifndef	_WRS_NO_TGT_SHELL_FP
    T_INT,
    T_FLOAT,
    T_DOUBLE
#else	/* _WRS_NO_TGT_SHELL_FP */
    T_INT
#endif	/* _WRS_NO_TGT_SHELL_FP */
    } TYPE;

typedef enum		/* SIDE */
    {
    LHS,
    RHS,
    FHS			/* function: rhs -> lhs */
    } SIDE;

typedef struct		/* VALUE */
    {
    SIDE side;
    TYPE type;
    union
	{
	int *lv;	/* pointer to any of the below */

	char byte;
	short word;
	int rv;
	char *string;
#ifndef	_WRS_NO_TGT_SHELL_FP
	float fp;
	double dp;
#endif	/* _WRS_NO_TGT_SHELL_FP */
	} value;
    } VALUE;

IMPORT int redirInFd;
IMPORT int redirOutFd;

LOCAL BOOL semError;	/* TRUE = semantic error found */
LOCAL VALUE tmpVal1;	/* used by BIN_OP above for expression evaluation */
LOCAL VALUE tmpVal2;	/* used by BIN_OP above for expression evaluation */
LOCAL int argStack [MAX_SHELL_ARGS];	/* arguments to functions */
LOCAL int nArgs;	/* number of args currently on argStack */
LOCAL BOOL usymFlag;	/* TRUE = U_SYMBOL has been seen */
LOCAL VALUE usymVal;	/* value of U_SYMBOL which has been seen */
LOCAL BOOL spawnFlag;	/* TRUE if spawn is first parameter in argStack[] */

%}

%start line

%token NL 0
%token T_SYMBOL D_SYMBOL U_SYMBOL NUMBER CHAR STRING FLOAT
%token OR AND EQ NE GE LE INCR DECR ROT_LEFT ROT_RIGHT UMINUS PTR TYPECAST
%token ENDFILE LEX_ERROR

%left '=' MULA DIVA MODA ADDA SUBA SHLA SHRA ANDA ORA XORA
%right '?' ':'
%left OR
%left AND
%left '|'
%left '^'
%left '&'
%left EQ NE
%left GE LE '>' '<'
%left ROT_LEFT ROT_RIGHT
%left '+' '-'
%left '*' '/' '%'
%left INCR DECR
%left UNARY	/* supplies precedence for unary operators */
%left PTR '[' '('
%left TYPECAST

%%

line	:	stmt
	|	stmt ';' line
	;
stmt	:	/* empty */
	|	expr			{ printValue (&$1); CHECK; }
	;

expr	:	D_SYMBOL
	|	T_SYMBOL { $1.side = RHS; setRv (&$$, &$1); }
	|	STRING	 { $$ = $1; $$.value.rv = newString((char*)$1.value.rv);
			   CHECK; }
	|	CHAR
	|	NUMBER
	|	FLOAT
	|	'(' expr ')'	 	{ $$ = $2; }
	|	expr '(' arglist ')'
			{ $$ = funcCall (&$1, &$3); CHECK; }
	|	typecast expr %prec TYPECAST
			{ 
			typeConvert (&$2, $1.type, $1.side); $$ = $2;
			CHECK;
			}
	|	'*' expr  %prec UNARY	{ VALUE tmp;
					  (void)getRv (&$2, &tmp);
					  setLv (&$$, &tmp);
					  CHECK;
					}
	|	'&' expr  %prec UNARY	{ $$.value.rv = (int)getLv (&$2);
					  $$.type = T_INT; $$.side = RHS; }
	|	'-' expr  %prec UNARY	{ rvOp (RV($2), UMINUS, NULLVAL); }
	|	'!' expr  %prec UNARY	{ rvOp (RV($2), '!', NULLVAL); }
	|	'~' expr  %prec UNARY	{ rvOp (RV($2), '~', NULLVAL); }
	|	expr '?' expr ':' expr	{ setRv (&$$, RV($1)->value.rv ? &$3
								       : &$5); }
	|	expr '[' expr ']'	{ BIN_OP ('+');
					  typeConvert (&$$, T_INT, RHS);
					  setLv (&$$, &$$); }
	|	expr PTR expr		{ BIN_OP ('+');
					  typeConvert (&$$, T_INT, RHS);
					  setLv (&$$, &$$); }
	|	expr '+' expr		{ BIN_OP ('+'); }
	|	expr '-' expr		{ BIN_OP ('-'); }
	|	expr '*' expr		{ BIN_OP ('*'); }
	|	expr '/' expr 		{ BIN_OP ('/'); }
	|	expr '%' expr 		{ BIN_OP ('%'); }
	|	expr ROT_RIGHT expr 	{ BIN_OP (ROT_RIGHT); }
	|	expr ROT_LEFT expr 	{ BIN_OP (ROT_LEFT); }
	|	expr '&' expr 		{ BIN_OP ('&'); }
	|	expr '^' expr 		{ BIN_OP ('^'); }
	|	expr '|' expr 		{ BIN_OP ('|'); }
	|	expr AND expr 		{ BIN_OP (AND); }
	|	expr OR expr 		{ BIN_OP (OR); }
	|	expr EQ expr 		{ BIN_OP (EQ); }
	|	expr NE expr 		{ BIN_OP (NE); }
	|	expr GE expr 		{ BIN_OP (GE); }
	|	expr LE expr 		{ BIN_OP (LE); }
	|	expr '>' expr 		{ BIN_OP ('>'); }
	|	expr '<' expr 		{ BIN_OP ('<'); }
	|	INCR expr	%prec UNARY	{ rvOp (RV($2), INCR, NULLVAL);
						  assign (&$2, &$$); CHECK; }
	|	DECR expr	%prec UNARY	{ rvOp (RV($2), DECR, NULLVAL);
						  assign (&$2, &$$); CHECK; }
	|	expr INCR	%prec UNARY	{ VALUE tmp;
						  tmp = $1;
						  rvOp (RV($1), INCR, NULLVAL);
						  assign (&$1, &$$); CHECK;
						  $$ = tmp; }
	|	expr DECR	%prec UNARY	{ VALUE tmp;
						  tmp = $1;
						  rvOp (RV($1), DECR, NULLVAL);
						  assign (&$1, &$$); CHECK;
						  $$ = tmp; }
	|	expr ADDA expr	{ BIN_OP (ADDA); assign (&$1, &$$); CHECK;}
	|	expr SUBA expr	{ BIN_OP (SUBA); assign (&$1, &$$); CHECK;}
	|	expr ANDA expr	{ BIN_OP (ANDA); assign (&$1, &$$); CHECK;}
	|	expr ORA  expr	{ BIN_OP (ORA);  assign (&$1, &$$); CHECK;}
	|	expr MODA expr	{ BIN_OP (MODA); assign (&$1, &$$); CHECK;}
	|	expr XORA expr	{ BIN_OP (XORA); assign (&$1, &$$); CHECK;}
	|	expr MULA expr	{ BIN_OP (MULA); assign (&$1, &$$); CHECK;}
	|	expr DIVA expr	{ BIN_OP (DIVA); assign (&$1, &$$); CHECK;}
	|	expr SHLA expr	{ BIN_OP (SHLA); assign (&$1, &$$); CHECK;}
	|	expr SHRA expr	{ BIN_OP (SHRA); assign (&$1, &$$); CHECK;}
	|	expr '=' expr	{ assign (&$1, &$3); $$ = $1; }
	|	U_SYMBOL
			/* the following is to allow "undef sym" error msg,
			 * instead of "syntax err" when U_SYM is not followed
			 * by proper assignment (see yyerror() below) */
			{ usymFlag = TRUE; usymVal = $1; }
		'=' expr 
			{
			if ($1.type != T_UNKNOWN)
			    {
			    printf ("typecast of lhs not allowed.\n");
			    YYERROR;
			    }
			else
			    {
			    $$ = newSym ((char *)$1.value.rv, $4.type); CHECK;
			    assign (&$$, &$4); CHECK;
			    }
			usymFlag = FALSE;
			}
	;

arglist	:	/* empty */
			{ $$ = newArgList (); }
	|	neArglist
	;

neArglist:	expr			/* non-empty arglist */
			{ $$ = newArgList (); addArg (&$$, &$1); CHECK; }
	|	neArglist ',' expr	/* ',' is required */
			{ addArg (&$1, &$3); CHECK; }
	;

typecast:	'(' TYPECAST ')'		{ $2.side = RHS; $$ = $2; }
	|	'(' TYPECAST '(' ')' ')'	{ $2.side = FHS; $$ = $2; }
	;

%%

#include "a_out.h"
#include "ctype.h"
#include "stdlib.h"
#include "string.h"
#include "symLib.h"

#include "shell_slex_c"

/* forward declarations */

LOCAL int newString ();
LOCAL VALUE *getRv ();
LOCAL int *getLv ();
LOCAL VALUE evalExp ();
#ifndef	_WRS_NO_TGT_SHELL_FP
LOCAL void doubleToInts ();
#endif	/* _WRS_NO_TGT_SHELL_FP */
LOCAL void setRv ();
LOCAL void typeConvert ();
LOCAL BOOL checkLv ();
LOCAL BOOL checkRv ();

/*******************************************************************************
*
* yystart - initialize local variables
*
* NOMANUAL
*/

void yystart (line)
    char *line;

    {
    lexNewLine (line);
    semError = FALSE;
    usymFlag = FALSE;
    nArgs = 0;
    spawnFlag = FALSE;
    }
/*******************************************************************************
*
* yyerror - report error
*
* This routine is called by yacc when an error is detected.
*/

LOCAL void yyerror (string)
    char *string;

    {
    if (semError)	/* semantic errors have already been reported */
	return;

    /* print error depending on what look-ahead token is */

    switch (yychar)
	{
	case U_SYMBOL:	/* U_SYM not at beginning of line */
	    printf ("undefined symbol: %s\n", (char *) yylval.value.rv);
	    break;

	case LEX_ERROR:	     /* lex should have already reported the problem */
	    break;

	default:
	    if (usymFlag)    /* leading U_SYM was followed by invalid assign */
		printf ("undefined symbol: %s\n", (char *)usymVal.value.rv);
	    else
		printf ("%s\n", string);
	    break;
	}
    }
/*******************************************************************************
*
* rvOp - sets rhs of yyval to evaluated expression
*/

LOCAL void rvOp (pY1, op, pY2)
    VALUE *pY1;
    int op;
    VALUE *pY2;

    {
    VALUE yy;

    yy = evalExp (pY1, op, pY2);

    setRv (&yyval, &yy);
    }
/*******************************************************************************
*
* assign - make assignment of new value to a cell
*/

LOCAL void assign (pLv, pRv)
    FAST VALUE *pLv;	/* lhs to be assigned into */
    FAST VALUE *pRv;	/* rhs value */

    {
    VALUE val;

    /* verify that lv can be assigned to, then make the assignment */

    if (checkLv (pLv) && checkRv (pRv))
	{
	(void)getRv (pRv, &val);

	/* make value agree in type */

	pLv->type = pRv->type;

	typeConvert (&val, pLv->type, RHS);

	switch (pLv->type)
	    {
	    case T_BYTE:
		* (char *)getLv (pLv) = val.value.byte;
		break;

	    case T_WORD:
		* (short *)getLv (pLv) = val.value.word;
		break;

	    case T_INT:
		*getLv (pLv) = val.value.rv;
		break;

#ifndef	_WRS_NO_TGT_SHELL_FP
	    case T_FLOAT:
		* (float *)getLv (pLv) = val.value.fp;
		break;

	    case T_DOUBLE:
		* (double *)getLv (pLv) = val.value.dp;
		break;
#endif	/* _WRS_NO_TGT_SHELL_FP */

	    default:
		printf ("bad assignment.\n");
		SET_ERROR;
	    }
	}
    else
	{
	printf ("bad assignment.\n");
	SET_ERROR;
	}
    }
/*******************************************************************************
*
* newString - allocate and copy a string
*/

LOCAL int newString (string)
    char *string;

    {
    int length    = strlen (string) + 1;
    char *address = (char *) malloc ((unsigned) length);

    if (address == NULL)
	{
	printf ("not enough memory for new string.\n");
	SET_ERROR;
	}
    else
	bcopy (string, address, length);

    return ((int)address);
    }
/*******************************************************************************
*
* newSym - allocate a new symbol and add to symbol table
*/

LOCAL VALUE newSym (name, type)
    char *name;
    TYPE type;

    {
    VALUE value;
    void *address = (void *) malloc (sizeof (double));

    if (address == NULL)
	{
	printf ("not enough memory for new variable.\n");
	SET_ERROR;
	}

    else if (symAdd (sysSymTbl, name, (char *) address, (N_BSS | N_EXT),
                     symGroupDefault) != OK)
	{
	free ((char *) address);
	printf ("can't add '%s' to system symbol table - error = 0x%x.\n",
		name, errnoGet());
	SET_ERROR;
	}
    else
	{
	printf ("new symbol \"%s\" added to symbol table.\n", name);

	value.side	= LHS;
	value.type	= type;
	value.value.lv	= (int *) address;
	}

    return (value);
    }
/*******************************************************************************
*
* printSym - print symbolic value
*/

LOCAL void printSym (val, prefix, suffix)
    FAST int val;
    char *prefix;
    char *suffix;

    {
    void *    symVal;  /* symbol value      */
    SYMBOL_ID symId;   /* symbol identifier */
    char *    name;    /* symbol name       */
    char      demangled [MAX_SYS_SYM_LEN + 1];
    char *    nameToPrint;

    /* Only search for symbol value and print symbol name if value is not -1 */
    
        if ((val != -1) && 
	    (symFindSymbol (sysSymTbl, NULL, (void *)val, 
		   	    SYM_MASK_NONE, SYM_MASK_NONE, &symId) == OK) &&
	    (symNameGet (symId, &name) == OK) &&
	    (symValueGet (symId, &symVal) == OK) &&
	    (symVal != 0) && ((val - (int)symVal) < 0x1000))
	    {
	    printf (prefix);

	    nameToPrint = cplusDemangle(name, demangled, sizeof (demangled));

	    if (val == (int) symVal)
	        printf ("%s", nameToPrint);
	    else
	        printf ("%s + 0x%x", nameToPrint, val - (int) symVal);

	    printf (suffix);
	    }
    
    }
/*******************************************************************************
*
* newArgList - start a new argument list
*/

LOCAL VALUE newArgList ()
    {
    VALUE value;

    value.side	   = RHS;
    value.type	   = T_INT;
    value.value.rv = nArgs;

    return (value);
    }
/*******************************************************************************
*
* addArg - add an argument to an argument list
*/

LOCAL void addArg (pArgList, pNewArg)
    VALUE *pArgList;
    FAST VALUE *pNewArg;

    {
    VALUE val;
    int partA;
    int partB;
#if CPU_FAMILY==I960
    int nArgsSave;
#endif
#ifndef	_WRS_NO_TGT_SHELL_FP
    BOOL isfloat = pNewArg->type == T_FLOAT || pNewArg->type == T_DOUBLE;
#endif	/* _WRS_NO_TGT_SHELL_FP */
    SYMBOL_ID   symId;  /* symbol identifier           */
    SYM_TYPE    sType;  /* place to return symbol type */

#ifndef	_WRS_NO_TGT_SHELL_FP
    if (isfloat)
# if CPU_FAMILY!=I960
	nArgs++;	/* will need an extra arg slot */
# else /* CPU_FAMILY!=I960 */
	{
	nArgsSave = nArgs;
	if (spawnFlag)
	    {
	    if ((nArgs %2) == 0)
	  	nArgs++;
	    }
	else
	    {
	    nArgs += nArgs % 2;	/* conditionally borrow slot to double align */
	    nArgs++;		/* borrow second slot for double-word value  */
	    }
	}
# endif /* CPU_FAMILY!=I960 */
#endif	/* _WRS_NO_TGT_SHELL_FP */

    if (nArgs == MAX_SHELL_ARGS || 
        (nArgs - pArgList->value.rv) == MAX_FUNC_ARGS)
	{
#ifndef	_WRS_NO_TGT_SHELL_FP
	if (isfloat)
# if CPU_FAMILY!=I960
	    nArgs--;		/* return borrowed slot */
# else  /* CPU_FAMILY!=I960 */
	    nArgs = nArgsSave;	/* return borrowed slot(s) */
# endif /* CPU_FAMILY!=I960 */
#endif	/* _WRS_NO_TGT_SHELL_FP */
	printf ("too many arguments to functions.\n");
	SET_ERROR;
	}
    else
	{
	/* push arg value on top of arg stack */

	(void)getRv (pNewArg, &val);

#ifndef	_WRS_NO_TGT_SHELL_FP
	if (isfloat)
	    {
# if CPU_FAMILY==I960
	    if (spawnFlag == FALSE)
# endif /* CPU_FAMILY==I960 */
		nArgs--;	/* return borrowed slot */
	    
	    /* put float as integers on argStack */

	    doubleToInts (pNewArg->type == T_FLOAT ?
			  val.value.fp : val.value.dp,
			  &partA, &partB);

	    argStack[nArgs++] = partA;
	    argStack[nArgs++] = partB;
	    }
	else if (checkRv (&val))
#else	/* _WRS_NO_TGT_SHELL_FP */
	if (checkRv (&val))
#endif	/* _WRS_NO_TGT_SHELL_FP */
	    {
	    int rv;

	    switch (val.type)
		{
		case T_BYTE:
		    rv = val.value.byte;
		    break;

		case T_WORD:
		    rv = val.value.word;
		    break;

		case T_INT:
		    rv = val.value.rv;
		
		    /* 
		     * new symLib api - symbol name lengths are no
		     * longer limited 
		     */

		    if (symFindSymbol (sysSymTbl, NULL, (void *)rv, 
			   	       SYM_MASK_NONE, SYM_MASK_NONE, 
				       &symId) == OK)
		 	symTypeGet (symId, &sType);

		    if ((nArgs == 0) && (sType == (N_TEXT + N_EXT)))
			spawnFlag = TRUE;
		    break;

		default:
		    rv = 0;
		    printf ("addArg: bad type.\n");
		    SET_ERROR;
		}

	    argStack[nArgs++] = rv;
	    }
	}
    }
#ifndef	_WRS_NO_TGT_SHELL_FP
/*******************************************************************************
*
* doubleToInts - separate double into two integer parts
*/

LOCAL void doubleToInts (d, partA, partB)
    double d;
    int *partA;
    int *partB;

    {
    union 
	{
	struct
	    {
	    int a;
	    int b;
	    } part;
	double d;
	} val;

    val.d = d;

    *partA = val.part.a;
    *partB = val.part.b;
    }
#endif	/* _WRS_NO_TGT_SHELL_FP */
/*******************************************************************************
*
* funcCall - call a function
*/

LOCAL VALUE funcCall (pV, pArgList)
    VALUE *pV;
    VALUE *pArgList;

    {
    static int funcStatus;	/* status from function calls */
    int a [MAX_FUNC_ARGS];
    VALUE value;
    FAST int i;
    FAST int argNum;
    int oldInFd	 = ioGlobalStdGet (STD_IN);
    int oldOutFd = ioGlobalStdGet (STD_OUT);
    FUNCPTR pFunc = (pV->side == LHS) ? (FUNCPTR) (int)getLv (pV)
				      : (FUNCPTR) pV->value.rv;

#if ((CPU_FAMILY == ARM) && ARM_THUMB)
    pFunc = (FUNCPTR)((UINT32)pFunc | 1);	/* make it a Thumb call */
#endif

    /* get any specified args off stack, or pre-set all args to 0 */

    for (argNum = pArgList->value.rv, i = 0; i < MAX_FUNC_ARGS; argNum++, i++)
	{
	a [i] = (argNum < nArgs) ? argStack[argNum] : 0;
	}

    /* set standard in/out to redirection fds */

    if (redirInFd >= 0)
	ioGlobalStdSet (STD_IN, redirInFd);

    if (redirOutFd >= 0)
	ioGlobalStdSet (STD_OUT, redirOutFd);

    /* call function and save resulting status */

    errnoSet (funcStatus);

    value.side = RHS;
    value.type = pV->type;

    switch (pV->type)
	{
	case T_BYTE:
	case T_WORD:
	case T_INT:
	    {
	    /* NOTE: THE FOLLOWING ARRAY REFERENCES MUST AGREE WITH THE
	     *       MAX_FUNC_ARGS COUNT DEFINED ABOVE IN THIS FILE!
	     */
	    int rv = (* pFunc) (a[0], a[1], a[2], a[3], a[4], a[5], a[6],
				a[7], a[8], a[9], a[10], a[11]);

	    switch (pV->type)
		{
		case T_BYTE:
		    value.value.byte = (char) rv;
		    break;

		case T_WORD:
		    value.value.word = (short) rv;
		    break;

		case T_INT:
		    value.value.rv = rv;
		    break;
		default:
		    break;
		}

	    break;
	    }

#ifndef	_WRS_NO_TGT_SHELL_FP
	case T_FLOAT:
	    value.value.fp = (* (float (*)())pFunc) (a[0], a[1], a[2], a[3],
			a[4], a[5], a[6], a[7], a[8], a[9], a[10], a[11]);
	    break;

	case T_DOUBLE:
	    value.value.dp = (* (double (*)())pFunc) (a[0], a[1], a[2], a[3],
			a[4], a[5], a[6], a[7], a[8], a[9], a[10], a[11]);
	    break;
#endif	/* _WRS_NO_TGT_SHELL_FP */

	default:
	    printf ("funcCall: bad function type.\n");
	    SET_ERROR;
	}

    funcStatus = errnoGet ();

    /* restore original in/out fds */

    if (redirInFd >= 0)
	ioGlobalStdSet (STD_IN, oldInFd);

    if (redirOutFd >= 0)
	ioGlobalStdSet (STD_OUT, oldOutFd);

    /* arg stack back to previous level */

    nArgs = pArgList->value.rv;

    return (value);
    }
/*******************************************************************************
*
* checkLv - check that a value can be used as left value
*/

LOCAL BOOL checkLv (pValue)
    VALUE *pValue;

    {
    if (pValue->side != LHS)
	{
	printf ("invalid application of 'address of' operator.\n");
	SET_ERROR;
	return (FALSE);
	}

    return (TRUE);
    }
/*******************************************************************************
*
* checkRv - check that a value can be used as right value
*/

LOCAL BOOL checkRv (pValue)
    VALUE *pValue;

    {
    if (pValue->side == LHS)
	return (checkLv (pValue));

    return (TRUE);
    }
/*******************************************************************************
*
* getRv - get a value's right value 
*/

LOCAL VALUE *getRv (pValue, pRv)
    FAST VALUE *pValue;
    FAST VALUE *pRv;			/* where to put value */

    {
    if (pValue->side == RHS)
	*pRv = *pValue;
    else
	{
	pRv->side = RHS;
	pRv->type = pValue->type;

	switch (pValue->type)
	    {
	    case T_BYTE:
		pRv->value.byte = *(char *)pValue->value.lv;
		break;

	    case T_WORD:
		pRv->value.word = *(short *)pValue->value.lv;
		break;

	    case T_INT:
		pRv->value.rv = *pValue->value.lv;
		break;

#ifndef	_WRS_NO_TGT_SHELL_FP
	    case T_FLOAT:
		pRv->value.fp = *(float *)pValue->value.lv;
		break;

	    case T_DOUBLE:
		pRv->value.dp = *(double *)pValue->value.lv;
		break;
#endif	/* _WRS_NO_TGT_SHELL_FP */

	    default:
		printf ("getRv: invalid rhs.");
		SET_ERROR;
	    }
	}

    return (pRv);
    }
/*******************************************************************************
*
* getLv - get a value's left value (address)
*/

LOCAL int *getLv (pValue)
    VALUE *pValue;

    {
    return (checkLv (pValue) ? pValue->value.lv : 0);
    }
/*******************************************************************************
*
* setLv - set a lv
*/

LOCAL void setLv (pVal1, pVal2)
    FAST VALUE *pVal1;
    FAST VALUE *pVal2;

    {
    if (pVal2->side == LHS)
	{
	printf ("setLv: invalid lhs.\n");
	SET_ERROR;
	}

    if ((int)pVal2->type != (int)T_INT)
	{
	printf ("setLv: type conflict.\n");
	SET_ERROR;
	}

    pVal1->side     = LHS;
    pVal1->type     = pVal2->type;
    pVal1->value.lv = (int *)pVal2->value.rv;
    }
/*******************************************************************************
*
* setRv - set the rv
*/

LOCAL void setRv (pVal1, pVal2)
    FAST VALUE *pVal1;
    FAST VALUE *pVal2;

    {
    pVal1->side = RHS;
    pVal1->type = pVal2->type;

    switch (pVal2->type)
	{
	case T_BYTE:
	    pVal1->value.byte = (pVal2->side == LHS) ?
			    *(char *)pVal2->value.lv : pVal2->value.byte;
	case T_WORD:
	    pVal1->value.word = (pVal2->side == LHS) ?
			    *(short *)pVal2->value.lv : pVal2->value.word;

	case T_INT:
	    pVal1->value.rv = (pVal2->side == LHS) ?
			    *pVal2->value.lv : pVal2->value.rv;
	    break;

#ifndef	_WRS_NO_TGT_SHELL_FP
	case T_FLOAT:
	    pVal1->value.fp = (pVal2->side == LHS) ?
			    *(float *)pVal2->value.lv : pVal2->value.fp;
	    break;

	case T_DOUBLE:
	    pVal1->value.dp = (pVal2->side == LHS) ?
			    *(double *)pVal2->value.lv : pVal2->value.dp;
	    break;
#endif	/* _WRS_NO_TGT_SHELL_FP */

	default:
	    printf ("setRv: bad type.\n");
	    SET_ERROR;
	}
    }
/*******************************************************************************
*
* printLv - print left-hand side value
*
* "ssss + xxx = xxxx"
*/

LOCAL void printLv (pValue)
    VALUE *pValue;

    {
    FAST int *lv = getLv (pValue);

    printSym ((int) lv, "", " = ");

    printf ("0x%x", (UINT) lv);
    }
/*******************************************************************************
*
* printRv - print right-hand side value
*
* The format for integers is:
*
* "nnnn = xxxx = 'c' = ssss + nnn"
*                           ^ only if nn < LIMIT for some ssss
*                 ^ only if value is printable
*/

LOCAL void printRv (pValue)
    VALUE *pValue;

    {
    VALUE val;
    int rv;

    (void)getRv (pValue, &val);

    switch (pValue->type)
	{
	case T_BYTE:
	    rv = val.value.byte;
	    goto caseT_INT;

	case T_WORD:
	    rv = val.value.word;
	    goto caseT_INT;

	case T_INT:
	    rv = val.value.rv;
	    /* drop through */

	caseT_INT:
	    printf ("%d = 0x%x", rv, rv);
	    if (isascii (rv) && isprint (rv))
		printf (" = '%c'", rv);

	    printSym (rv, " = ", "");
	    break;

#ifndef	_WRS_NO_TGT_SHELL_FP
	case T_FLOAT:
	    printf ("%g", val.value.fp);
	    break;

	case T_DOUBLE:
	    printf ("%g", val.value.dp);
	    break;
#endif	/* _WRS_NO_TGT_SHELL_FP */

	default:
	    printf ("printRv: bad type.\n");
	    SET_ERROR;
	}
    }
/*******************************************************************************
*
* printValue - print out value
*/

LOCAL void printValue (pValue)
    FAST VALUE *pValue;

    {
    if (pValue->side == LHS)
	{
	if (checkLv (pValue) && checkRv (pValue))
	    {
	    printLv (pValue);
	    printf (": value = ");

	    printRv (pValue);
	    printf ("\n");
	    }
	else
	    {
	    printf ("invalid lhs.\n");
	    SET_ERROR;
	    }
	}
    else if (checkRv (pValue))
	{
	printf ("value = ");

	printRv (pValue);
	printf ("\n");
	}
    else
	{
	printf ("invalid rhs.\n");
	SET_ERROR;
	}
    }

/* TYPE SUPPORT */

LOCAL VALUE evalUnknown ();
LOCAL VALUE evalByte ();
LOCAL VALUE evalWord ();
LOCAL VALUE evalInt ();
LOCAL VALUE evalFloat ();
LOCAL VALUE evalDouble ();

typedef struct		/* EVAL_TYPE */
    {
    VALUE (*eval) ();
    } EVAL_TYPE;

LOCAL EVAL_TYPE evalType [] =
    {
    /*	eval		type		*/
    /*	---------------	--------------	*/
      { evalUnknown,	/* T_UNKNOWN*/	},
      { evalByte,	/* T_BYTE   */	},
      { evalWord,	/* T_WORD   */	},
      { evalInt,	/* T_INT    */	},
#ifndef	_WRS_NO_TGT_SHELL_FP
      { evalFloat,	/* T_FLOAT  */	},
      { evalDouble,	/* T_DOUBLE */	},
#endif	/* _WRS_NO_TGT_SHELL_FP */
    };

/*******************************************************************************
*
* evalExp - evaluate expression
*/

LOCAL VALUE evalExp (pValue1, op, pValue2)
    VALUE *pValue1;
    int op;
    VALUE *pValue2;

    {
    VALUE *p1 = pValue1;
    VALUE *p2 = pValue2;

    if (pValue2 == NULLVAL) /* unary expresions must set pValue2 to something */
	p2 = pValue2 = pValue1;

    /* make sure values have the same type */

    if ((int)p1->type > (int)p2->type)
	typeConvert (p2, p1->type, p1->side);
    else
	typeConvert (p1, p2->type, p2->side);

    return ((evalType[(int)pValue1->type].eval) (pValue1, op, pValue2));
    }
/*******************************************************************************
*
* evalUnknown - evaluate for unknown result
*
* ARGSUSED
*/

LOCAL VALUE evalUnknown (pValue1, op, pValue2)
    VALUE *pValue1;
    int op;
    VALUE *pValue2;

    {
    printf ("evalUnknown: bad evaluation.\n");

    SET_ERROR;

    return (*pValue1);	/* have to return something */
    }
/*******************************************************************************
*
* evalByte - evaluate for byte result
*/

LOCAL VALUE evalByte (pValue1, op, pValue2)
    VALUE *pValue1;
    int op;
    VALUE *pValue2;

    {
    VALUE *p1 = pValue1;
    VALUE *p2 = pValue2;
    VALUE result;

    /* evaluate as integers and then convert back */

    typeConvert (p1, T_INT, RHS);
    typeConvert (p2, T_INT, RHS);

    result = evalInt (p1, op, p2);

    typeConvert (&result, T_BYTE, RHS);

    return (result);
    }
/*******************************************************************************
*
* evalWord - evaluate for word result
*/

LOCAL VALUE evalWord (pValue1, op, pValue2)
    VALUE *pValue1;
    int op;
    VALUE *pValue2;

    {
    VALUE *p1 = pValue1;
    VALUE *p2 = pValue2;
    VALUE result;

    /* evaluate as integers and then convert back */

    typeConvert (p1, T_INT, RHS);
    typeConvert (p2, T_INT, RHS);

    result = evalInt (p1, op, p2);

    typeConvert (&result, T_WORD, RHS);

    return (result);
    }
/*******************************************************************************
*
* evalInt - evaluate for integer result
*/

LOCAL VALUE evalInt (pValue1, op, pValue2)
    VALUE *pValue1;
    int op;
    VALUE *pValue2;

    {
#define	OP_INT(op)	rv = e1 op e2; break
#define	OP_INT_U(op)	rv = op e1; break

    FAST int e1 = pValue1->value.rv;
    FAST int e2 = pValue2->value.rv;
    FAST int rv;
    VALUE result;

    switch (op)
	{
	case ADDA:
	case '+':
	    OP_INT(+);
	case SUBA:
	case '-':
	    OP_INT(-);
	case MULA:
	case '*':
	    OP_INT(*);
	case DIVA:
	case '/':
	    OP_INT(/);
	case '!':
	    OP_INT_U(!);
	case '~':
	    OP_INT_U(~);
	case MODA:
	case '%':
	    OP_INT(%);
	case ANDA:
	case '&':
	    OP_INT(&);
	case XORA:
	case '^':
	    OP_INT(^);
	case ORA:
	case '|':
	    OP_INT(|);
	case '<':
	    OP_INT(<);
	case '>':
	    OP_INT(>);
	case OR:
	    OP_INT(||);
	case AND:
	    OP_INT(&&);
	case EQ:
	    OP_INT(==);
	case NE:
	    OP_INT(!=);
	case GE:
	    OP_INT(>=);
	case LE:
	    OP_INT(<=);
	case INCR:
	    OP_INT_U(++);
	case DECR:
	    OP_INT_U(--);
	case SHLA:
	case ROT_LEFT:
	    OP_INT(<<);
	case SHRA:
	case ROT_RIGHT:
	    OP_INT(>>);
	case UMINUS:
	    OP_INT_U(-);
	default:
	    rv = 0;
	    printf ("operands have incompatible types.\n");
	    SET_ERROR;
	}

    result.side     = RHS;
    result.type     = pValue1->type;
    result.value.rv = rv;

    return (result);
    }
#ifndef	_WRS_NO_TGT_SHELL_FP
/*******************************************************************************
*
* evalFloat - evaluate for float result
*/

LOCAL VALUE evalFloat (pValue1, op, pValue2)
    VALUE *pValue1;
    int op;
    VALUE *pValue2;

    {
    VALUE *p1 = pValue1;
    VALUE *p2 = pValue2;
    VALUE result;

    /* evaluate as doubles and then convert back */

    typeConvert (p1, T_DOUBLE, RHS);
    typeConvert (p2, T_DOUBLE, RHS);

    result = evalDouble (p1, op, p2);

    typeConvert (&result, T_FLOAT, RHS);

    return (result);
    }
/*******************************************************************************
*
* evalDouble - evaluate for double result
*/

LOCAL VALUE evalDouble (pValue1, op, pValue2)
    VALUE *pValue1;
    int op;
    VALUE *pValue2;

    {
#define	OP_DOUBLE(op)	dp = e1 op e2; break
#define	OP_DOUBLE_U(op)	dp = op e1; break

    FAST double e1 = pValue1->value.dp;
    FAST double e2 = pValue2->value.dp;
    FAST double dp;
    VALUE result;

    switch (op)
	{
	case ADDA:
	case '+':
	    OP_DOUBLE(+);
	case SUBA:
	case '-':
	    OP_DOUBLE(-);
	case MULA:
	case '*':
	    OP_DOUBLE(*);
	case DIVA:
	case '/':
	    OP_DOUBLE(/);
	case '!':
	    OP_DOUBLE_U(!);

	case '<':
	    OP_DOUBLE(<);
	case '>':
	    OP_DOUBLE(>);
	case OR:
	    OP_DOUBLE(||);
	case AND:
	    OP_DOUBLE(&&);
	case EQ:
	    OP_DOUBLE(==);
	case NE:
	    OP_DOUBLE(!=);
	case GE:
	    OP_DOUBLE(>=);
	case LE:
	    OP_DOUBLE(<=);
	case INCR:
	    OP_DOUBLE_U(++);
	case DECR:
	    OP_DOUBLE_U(--);

	case UMINUS:
	    OP_DOUBLE_U(-);

	default:
	    dp = 0;
	    printf ("operands have incompatible types.\n");
	    SET_ERROR;
	}

    result.side     = RHS;
    result.type     = T_DOUBLE;
    result.value.dp = dp;

    return (result);
    }
#endif	/* _WRS_NO_TGT_SHELL_FP */

/* TYPE CONVERSION */

LOCAL void convUnknown ();
LOCAL void convByte ();
LOCAL void convWord ();
LOCAL void convInt ();
#ifndef	_WRS_NO_TGT_SHELL_FP
LOCAL void convFloat ();
LOCAL void convDouble ();
#endif	/* _WRS_NO_TGT_SHELL_FP */

typedef void (*VOID_FUNCPTR) ();	/* ptr to a function returning void */

LOCAL VOID_FUNCPTR convType [] =
    {
    /*  conversion	type	    */
    /*  ----------	----------- */
	convUnknown,	/* T_UNKNOWN*/
	convByte,	/* T_BYTE   */
	convWord,	/* T_WORD   */
	convInt,	/* T_INT    */
#ifndef	_WRS_NO_TGT_SHELL_FP
	convFloat,	/* T_FLOAT  */
	convDouble,	/* T_DOUBLE */
#endif	/* _WRS_NO_TGT_SHELL_FP */
    };

/*******************************************************************************
*
* typeConvert - change value to specified type
*/

LOCAL void typeConvert (pValue, type, side)
    FAST VALUE *pValue;
    TYPE type;
    SIDE side;

    {
    if (side == FHS)
	{
	pValue->side = RHS;
	pValue->type = type;
	}
    else if (side == RHS)
	{
	if (pValue->side == LHS)
	    pValue->type = type;
	else
	    (convType [(int) type]) (pValue);
	}
    else if (pValue->side == LHS)
	pValue->type = type;
    else
	{
	printf ("typeConvert: bad type.\n");
	SET_ERROR;
	}
    }
/*******************************************************************************
*
* convUnknown - convert value to unknown
*
* ARGSUSED
*/

LOCAL void convUnknown (pValue)
    VALUE *pValue;

    {
    printf ("convUnknown: bad type.\n");
    SET_ERROR;
    }
/*******************************************************************************
*
* convByte - convert value to byte
*/

LOCAL void convByte (pValue)
    FAST VALUE *pValue;

    {
    char value;

    if ((int)pValue->type > (int)T_BYTE)
	{
	convWord (pValue);
	value = pValue->value.word;
	pValue->value.byte = value;
	pValue->type = T_BYTE;
	}
    }
/*******************************************************************************
*
* convWord - convert value to word
*/

LOCAL void convWord (pValue)
    FAST VALUE *pValue;

    {
    short value;

    if ((int)pValue->type < (int)T_WORD)
	{
	value = pValue->value.byte;
	pValue->value.word = value;
	pValue->type = T_WORD;
	}
    else if ((int)pValue->type > (int)T_WORD)
	{
	convInt (pValue);
	value = pValue->value.rv;
	pValue->value.word = value;
	pValue->type = T_WORD;
	}
    }
/*******************************************************************************
*
* convInt - convert value to integer
*/

LOCAL void convInt (pValue)
    FAST VALUE *pValue;

    {
    int value;

    if ((int)pValue->type < (int)T_INT)
	{
	convWord (pValue);
	value = pValue->value.word;
	pValue->value.rv = value;
	pValue->type = T_INT;
	}
    else if ((int)pValue->type > (int)T_INT)
	{
#ifndef	_WRS_NO_TGT_SHELL_FP
	convFloat (pValue);
	value = pValue->value.fp;
	pValue->value.rv = value;
	pValue->type = T_INT;
#endif	/* _WRS_NO_TGT_SHELL_FP */
	}
    }
#ifndef	_WRS_NO_TGT_SHELL_FP
/*******************************************************************************
*
* convFloat - convert value to float
*/

LOCAL void convFloat (pValue)
    FAST VALUE *pValue;

    {
    float value;

    if ((int)pValue->type < (int)T_FLOAT)
	{
	convInt (pValue);
	value = pValue->value.rv;
	pValue->value.fp = value;
	pValue->type = T_FLOAT;
	}
    else if ((int)pValue->type > (int)T_FLOAT)
	{
	convDouble (pValue);
	value = pValue->value.dp;
	pValue->value.fp = value;
	pValue->type = T_FLOAT;
	}
    }
/*******************************************************************************
*
* convDouble - convert value to double
*/

LOCAL void convDouble (pValue)
    FAST VALUE *pValue;

    {
    double value;

    if ((int)pValue->type < (int)T_DOUBLE)
	{
	convFloat (pValue);

	value = pValue->value.fp;
	pValue->value.dp = value;
	pValue->type = T_DOUBLE;
	}
    }
#endif	/* _WRS_NO_TGT_SHELL_FP */

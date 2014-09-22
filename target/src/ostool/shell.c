
# line 2 "shell.yacc"
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

# define NL 0
# define T_SYMBOL 258
# define D_SYMBOL 259
# define U_SYMBOL 260
# define NUMBER 261
# define CHAR 262
# define STRING 263
# define FLOAT 264
# define OR 265
# define AND 266
# define EQ 267
# define NE 268
# define GE 269
# define LE 270
# define INCR 271
# define DECR 272
# define ROT_LEFT 273
# define ROT_RIGHT 274
# define UMINUS 275
# define PTR 276
# define TYPECAST 277
# define ENDFILE 278
# define LEX_ERROR 279
# define MULA 280
# define DIVA 281
# define MODA 282
# define ADDA 283
# define SUBA 284
# define SHLA 285
# define SHRA 286
# define ANDA 287
# define ORA 288
# define XORA 289
# define UNARY 290

#ifdef __STDC__
#include <stdlib.h>
#include <string.h>
#else
#include <malloc.h>
#include <memory.h>
#endif

#if defined(__cplusplus) || defined(__STDC__)

#if defined(__cplusplus) && defined(__EXTERN_C__)
extern "C" {
#endif
#ifndef yyerror
#if defined(__cplusplus)
	void yyerror(const char *);
#endif
#endif
#ifndef yylex
	int yylex(void);
#endif
	int yyparse(void);
#if defined(__cplusplus) && defined(__EXTERN_C__)
}
#endif

#endif

#define yyclearin yychar = -1
#define yyerrok yyerrflag = 0
extern int yychar;
extern int yyerrflag;
#ifndef YYSTYPE
#define YYSTYPE int
#endif
YYSTYPE yylval;
YYSTYPE yyval;
typedef int yytabelem;
#ifndef YYMAXDEPTH
#define YYMAXDEPTH 150
#endif
#if YYMAXDEPTH > 0
int yy_yys[YYMAXDEPTH], *yys = yy_yys;
YYSTYPE yy_yyv[YYMAXDEPTH], *yyv = yy_yyv;
#else	/* user does initial allocation */
int *yys;
YYSTYPE *yyv;
#endif
static int yymaxdepth = YYMAXDEPTH;
# define YYERRCODE 256

# line 335 "shell.yacc"


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
#if CPU_FAMILY!=I960
	nArgs++;	/* will need an extra arg slot */
#else /* CPU_FAMILY!=I960 */
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
#endif /* CPU_FAMILY!=I960 */
#endif	/* _WRS_NO_TGT_SHELL_FP */

    if (nArgs == MAX_SHELL_ARGS || 
        (nArgs - pArgList->value.rv) == MAX_FUNC_ARGS)
	{
#ifndef	_WRS_NO_TGT_SHELL_FP
	if (isfloat)
#if CPU_FAMILY!=I960
	    nArgs--;		/* return borrowed slot */
#else  /* CPU_FAMILY!=I960 */
	    nArgs = nArgsSave;	/* return borrowed slot(s) */
#endif /* CPU_FAMILY!=I960 */
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
#if CPU_FAMILY==I960
	    if (spawnFlag == FALSE)
#endif /* CPU_FAMILY==I960 */
		nArgs--;	/* return borrowed slot */
	    
	    /* put float as integers on argStack */

	    doubleToInts (pNewArg->type == T_FLOAT ?
			  val.value.fp : val.value.dp,
			  &partA, &partB);

	    argStack[nArgs++] = partA;
	    argStack[nArgs++] = partB;
	    }
	else if (checkRv (&val))
#else
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
static const yytabelem yyexca[] ={
-1, 1,
	0, -1,
	-2, 0,
	};
# define YYNPROD 63
# define YYLAST 810
static const yytabelem yyact[]={

    29,    32,   106,    21,    21,    27,    25,    20,    26,   108,
    28,   105,   104,   115,   111,   107,    69,     1,    66,    11,
    68,     2,     0,    42,    55,    41,    22,    29,    32,     0,
    21,     0,    27,    25,     0,    26,     0,    28,    67,     0,
     0,     0,     0,     0,     0,     0,     0,     0,   109,     0,
    42,    55,    41,    22,    23,    23,   110,    33,     0,     0,
     0,     0,     0,     0,     0,    29,    32,     0,    21,   103,
    27,    25,     0,    26,     0,    28,     0,     0,     0,     0,
     0,    23,     0,     0,    33,     0,     0,    34,    42,    55,
    41,    22,    29,    32,     0,    21,     0,    27,    25,     0,
    26,     0,    28,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,    34,    42,    55,    41,    22,    23,
     0,     0,    33,     0,     0,     0,     0,     0,    29,    32,
     0,    21,     0,    27,    25,     0,    26,     0,    28,     0,
     0,     0,     0,     0,     0,     0,    23,     0,     0,    33,
     0,    42,    34,    41,    22,    29,    32,     0,    21,     0,
    27,    25,     0,    26,     0,    28,    29,    32,     0,    21,
     0,    27,    25,     0,    26,     0,    28,     0,    42,    34,
    41,     0,    23,     0,     0,    33,     0,    29,     0,    42,
    21,    41,    27,    25,     0,    26,     0,    28,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,    23,
     0,     0,    33,     0,     0,    34,     0,     0,     0,     0,
    23,     0,     0,    33,     0,     0,     0,     0,    36,    35,
    37,    38,    39,    40,    43,    44,    31,    30,     0,    24,
    24,    23,    34,    51,    52,    49,    45,    46,    53,    54,
    47,    48,    50,    34,     0,    36,    35,    37,    38,    39,
    40,    43,    44,    31,    30,     0,    24,    21,     0,     0,
    51,    52,    49,    45,    46,    53,    54,    47,    48,    50,
    29,    32,     0,    21,     0,    27,    25,     0,    26,     0,
    28,     0,     0,    36,    35,    37,    38,    39,    40,    43,
    44,    31,    30,    42,    24,    41,     0,     0,    51,    52,
    49,    45,    46,    53,    54,    47,    48,    50,    23,     0,
    36,    35,    37,    38,    39,    40,    43,    44,    31,    30,
     0,    24,     0,     0,    23,    51,    52,    49,    45,    46,
    53,    54,    47,    48,    50,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,    36,    35,    37,    38,
    39,    40,    43,    44,    31,    30,     0,    24,     0,     0,
    29,    32,     0,    21,     0,    27,    25,     0,    26,     0,
    28,     0,     0,     0,    35,    37,    38,    39,    40,    43,
    44,    31,    30,    42,    24,    41,    37,    38,    39,    40,
    43,    44,    31,    30,     0,    24,    29,     0,     0,    21,
     0,    27,    25,     0,    26,     0,    28,     0,     0,     0,
     0,    43,    44,     0,    23,    15,    24,    33,     0,    42,
    13,    41,    10,     0,    12,     0,     0,    14,    29,     0,
     0,    21,     0,    27,    25,     0,    26,    29,    28,     0,
    21,     0,    27,    25,     0,    26,     0,    28,     0,     0,
    23,    42,    15,    41,     0,     0,     0,    13,     0,    10,
    29,    12,     0,    21,    14,    27,     0,     0,     0,     0,
    28,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,    23,     0,     0,     0,     0,     0,    43,    44,
     0,    23,     0,    24,     0,     0,     0,     0,     0,     0,
    37,    38,    39,    40,    43,    44,    31,    30,    16,    24,
     0,     0,     0,     0,    23,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,    16,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    37,    38,    39,    40,    43,    44,    31,    30,     0,    24,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,    37,    38,    39,    40,
    43,    44,    31,    30,     0,    24,     0,     0,     0,     0,
     5,     4,    19,     8,     7,     6,     9,     0,     0,     0,
     0,     0,     0,    17,    18,     0,     0,     0,     0,    57,
    39,    40,    43,    44,    31,    30,     0,    24,     0,     0,
     0,    43,    44,    31,    30,     0,    24,     5,     4,    19,
     8,     7,     6,     9,     0,     0,     0,     0,     0,     3,
    17,    18,     0,     0,    43,    44,     0,     0,     0,    24,
    56,    58,    59,    60,    61,    62,    63,    64,    65,     0,
     0,    70,    71,    72,    73,    74,    75,    76,    77,    78,
    79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
    89,    90,    91,     0,     0,    92,    93,    94,    95,    96,
    97,    98,    99,   100,   101,   102,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,   112,     0,   113,   114 };
static const yytabelem yypact[]={

   429,-10000000,   -52,    55,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,
   392,   429,   429,   429,   429,   429,   429,   429,   429,-10000000,
   429,   429,   429,   429,   429,   429,   429,   429,   429,   429,
   429,   429,   429,   429,   429,   429,   429,   429,   429,   429,
   429,   429,   429,-10000000,-10000000,   429,   429,   429,   429,   429,
   429,   429,   429,   429,   429,   429,    28,   -29,-10000000,   -36,
   -36,   -36,   -36,   -36,   -36,   -36,   -59,-10000000,   -26,   -35,
    55,   -10,   -37,-10000000,   433,   433,   227,   227,   227,   150,
   150,   369,   243,   333,   129,   118,   401,   401,   410,   410,
   410,   410,    91,    91,    91,    91,    91,    91,    91,    91,
    91,    91,    91,-10000000,-10000000,   -27,   429,-10000000,   429,   429,
-10000000,   -28,    91,    55,    91,-10000000 };
static const yytabelem yypgo[]={

     0,    17,    21,   699,    20,    19,    18,    16 };
static const yytabelem yyr1[]={

     0,     1,     1,     2,     2,     3,     3,     3,     3,     3,
     3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
     3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
     3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
     3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
     3,     3,     3,     3,     3,     6,     3,     4,     4,     7,
     7,     5,     5 };
static const yytabelem yyr2[]={

     0,     2,     6,     0,     3,     2,     3,     3,     2,     2,
     2,     7,     9,     5,     5,     5,     5,     5,     5,    11,
     9,     7,     7,     7,     7,     7,     7,     7,     7,     7,
     7,     7,     7,     7,     7,     7,     7,     7,     7,     7,
     5,     5,     5,     5,     7,     7,     7,     7,     7,     7,
     7,     7,     7,     7,     7,     1,     9,     1,     2,     3,
     7,     7,    11 };
static const yytabelem yychk[]={

-10000000,    -1,    -2,    -3,   259,   258,   263,   262,   261,   264,
    40,    -5,    42,    38,    45,    33,   126,   271,   272,   260,
    59,    40,    63,    91,   276,    43,    45,    42,    47,    37,
   274,   273,    38,    94,   124,   266,   265,   267,   268,   269,
   270,    62,    60,   271,   272,   283,   284,   287,   288,   282,
   289,   280,   281,   285,   286,    61,    -3,   277,    -3,    -3,
    -3,    -3,    -3,    -3,    -3,    -3,    -6,    -1,    -4,    -7,
    -3,    -3,    -3,    -3,    -3,    -3,    -3,    -3,    -3,    -3,
    -3,    -3,    -3,    -3,    -3,    -3,    -3,    -3,    -3,    -3,
    -3,    -3,    -3,    -3,    -3,    -3,    -3,    -3,    -3,    -3,
    -3,    -3,    -3,    41,    41,    40,    61,    41,    44,    58,
    93,    41,    -3,    -3,    -3,    41 };
static const yytabelem yydef[]={

     3,    -2,     1,     4,     5,     6,     7,     8,     9,    10,
     0,     0,     0,     0,     0,     0,     0,     0,     0,    55,
     3,    57,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,    42,    43,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,    13,    14,
    15,    16,    17,    18,    40,    41,     0,     2,     0,    58,
    59,     0,     0,    21,    22,    23,    24,    25,    26,    27,
    28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
    38,    39,    44,    45,    46,    47,    48,    49,    50,    51,
    52,    53,    54,    11,    61,     0,     0,    12,     0,     0,
    20,     0,    56,    60,    19,    62 };
typedef struct
#ifdef __cplusplus
	yytoktype
#endif
{ char *t_name; int t_val; } yytoktype;
#ifndef YYDEBUG
#	define YYDEBUG	0	/* don't allow debugging */
#endif

#if YYDEBUG

yytoktype yytoks[] =
{
	"NL",	0,
	"T_SYMBOL",	258,
	"D_SYMBOL",	259,
	"U_SYMBOL",	260,
	"NUMBER",	261,
	"CHAR",	262,
	"STRING",	263,
	"FLOAT",	264,
	"OR",	265,
	"AND",	266,
	"EQ",	267,
	"NE",	268,
	"GE",	269,
	"LE",	270,
	"INCR",	271,
	"DECR",	272,
	"ROT_LEFT",	273,
	"ROT_RIGHT",	274,
	"UMINUS",	275,
	"PTR",	276,
	"TYPECAST",	277,
	"ENDFILE",	278,
	"LEX_ERROR",	279,
	"=",	61,
	"MULA",	280,
	"DIVA",	281,
	"MODA",	282,
	"ADDA",	283,
	"SUBA",	284,
	"SHLA",	285,
	"SHRA",	286,
	"ANDA",	287,
	"ORA",	288,
	"XORA",	289,
	"?",	63,
	":",	58,
	"|",	124,
	"^",	94,
	"&",	38,
	">",	62,
	"<",	60,
	"+",	43,
	"-",	45,
	"*",	42,
	"/",	47,
	"%",	37,
	"UNARY",	290,
	"[",	91,
	"(",	40,
	"-unknown-",	-1	/* ends search */
};

char * yyreds[] =
{
	"-no such reduction-",
	"line : stmt",
	"line : stmt ';' line",
	"stmt : /* empty */",
	"stmt : expr",
	"expr : D_SYMBOL",
	"expr : T_SYMBOL",
	"expr : STRING",
	"expr : CHAR",
	"expr : NUMBER",
	"expr : FLOAT",
	"expr : '(' expr ')'",
	"expr : expr '(' arglist ')'",
	"expr : typecast expr",
	"expr : '*' expr",
	"expr : '&' expr",
	"expr : '-' expr",
	"expr : '!' expr",
	"expr : '~' expr",
	"expr : expr '?' expr ':' expr",
	"expr : expr '[' expr ']'",
	"expr : expr PTR expr",
	"expr : expr '+' expr",
	"expr : expr '-' expr",
	"expr : expr '*' expr",
	"expr : expr '/' expr",
	"expr : expr '%' expr",
	"expr : expr ROT_RIGHT expr",
	"expr : expr ROT_LEFT expr",
	"expr : expr '&' expr",
	"expr : expr '^' expr",
	"expr : expr '|' expr",
	"expr : expr AND expr",
	"expr : expr OR expr",
	"expr : expr EQ expr",
	"expr : expr NE expr",
	"expr : expr GE expr",
	"expr : expr LE expr",
	"expr : expr '>' expr",
	"expr : expr '<' expr",
	"expr : INCR expr",
	"expr : DECR expr",
	"expr : expr INCR",
	"expr : expr DECR",
	"expr : expr ADDA expr",
	"expr : expr SUBA expr",
	"expr : expr ANDA expr",
	"expr : expr ORA expr",
	"expr : expr MODA expr",
	"expr : expr XORA expr",
	"expr : expr MULA expr",
	"expr : expr DIVA expr",
	"expr : expr SHLA expr",
	"expr : expr SHRA expr",
	"expr : expr '=' expr",
	"expr : U_SYMBOL",
	"expr : U_SYMBOL '=' expr",
	"arglist : /* empty */",
	"arglist : neArglist",
	"neArglist : expr",
	"neArglist : neArglist ',' expr",
	"typecast : '(' TYPECAST ')'",
	"typecast : '(' TYPECAST '(' ')' ')'",
};
#endif /* YYDEBUG */
# line	1 "/usr/ccs/bin/yaccpar"
/*
 * Copyright (c) 1993 by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)yaccpar	6.15	97/12/08 SMI"

/*
** Skeleton parser driver for yacc output
*/

/*
** yacc user known macros and defines
*/
#define YYERROR		goto yyerrlab
#define YYACCEPT	return(0)
#define YYABORT		return(1)
#define YYBACKUP( newtoken, newvalue )\
{\
	if ( yychar >= 0 || ( yyr2[ yytmp ] >> 1 ) != 1 )\
	{\
		yyerror( "syntax error - cannot backup" );\
		goto yyerrlab;\
	}\
	yychar = newtoken;\
	yystate = *yyps;\
	yylval = newvalue;\
	goto yynewstate;\
}
#define YYRECOVERING()	(!!yyerrflag)
#define YYNEW(type)	malloc(sizeof(type) * yynewmax)
#define YYCOPY(to, from, type) \
	(type *) memcpy(to, (char *) from, yymaxdepth * sizeof (type))
#define YYENLARGE( from, type) \
	(type *) realloc((char *) from, yynewmax * sizeof(type))
#ifndef YYDEBUG
#	define YYDEBUG	1	/* make debugging available */
#endif

/*
** user known globals
*/
int yydebug;			/* set to 1 to get debugging */

/*
** driver internal defines
*/
#define YYFLAG		(-10000000)

/*
** global variables used by the parser
*/
YYSTYPE *yypv;			/* top of value stack */
int *yyps;			/* top of state stack */

int yystate;			/* current state */
int yytmp;			/* extra var (lasts between blocks) */

int yynerrs;			/* number of errors */
int yyerrflag;			/* error recovery flag */
int yychar;			/* current input token number */



#ifdef YYNMBCHARS
#define YYLEX()		yycvtok(yylex())
/*
** yycvtok - return a token if i is a wchar_t value that exceeds 255.
**	If i<255, i itself is the token.  If i>255 but the neither 
**	of the 30th or 31st bit is on, i is already a token.
*/
#if defined(__STDC__) || defined(__cplusplus)
int yycvtok(int i)
#else
int yycvtok(i) int i;
#endif
{
	int first = 0;
	int last = YYNMBCHARS - 1;
	int mid;
	wchar_t j;

	if(i&0x60000000){/*Must convert to a token. */
		if( yymbchars[last].character < i ){
			return i;/*Giving up*/
		}
		while ((last>=first)&&(first>=0)) {/*Binary search loop*/
			mid = (first+last)/2;
			j = yymbchars[mid].character;
			if( j==i ){/*Found*/ 
				return yymbchars[mid].tvalue;
			}else if( j<i ){
				first = mid + 1;
			}else{
				last = mid -1;
			}
		}
		/*No entry in the table.*/
		return i;/* Giving up.*/
	}else{/* i is already a token. */
		return i;
	}
}
#else/*!YYNMBCHARS*/
#define YYLEX()		yylex()
#endif/*!YYNMBCHARS*/

/*
** yyparse - return 0 if worked, 1 if syntax error not recovered from
*/
#if defined(__STDC__) || defined(__cplusplus)
int yyparse(void)
#else
int yyparse()
#endif
{
	register YYSTYPE *yypvt = 0;	/* top of value stack for $vars */

#if defined(__cplusplus) || defined(lint)
/*
	hacks to please C++ and lint - goto's inside switch should never be
	executed; yypvt is set to 0 to avoid "used before set" warning.
*/
	static int __yaccpar_lint_hack__ = 0;
	switch (__yaccpar_lint_hack__)
	{
		case 1: goto yyerrlab;
		case 2: goto yynewstate;
	}
#endif

	/*
	** Initialize externals - yyparse may be called more than once
	*/
	yypv = &yyv[-1];
	yyps = &yys[-1];
	yystate = 0;
	yytmp = 0;
	yynerrs = 0;
	yyerrflag = 0;
	yychar = -1;

#if YYMAXDEPTH <= 0
	if (yymaxdepth <= 0)
	{
		if ((yymaxdepth = YYEXPAND(0)) <= 0)
		{
			yyerror("yacc initialization error");
			YYABORT;
		}
	}
#endif

	{
		register YYSTYPE *yy_pv;	/* top of value stack */
		register int *yy_ps;		/* top of state stack */
		register int yy_state;		/* current state */
		register int  yy_n;		/* internal state number info */
	goto yystack;	/* moved from 6 lines above to here to please C++ */

		/*
		** get globals into registers.
		** branch to here only if YYBACKUP was called.
		*/
	yynewstate:
		yy_pv = yypv;
		yy_ps = yyps;
		yy_state = yystate;
		goto yy_newstate;

		/*
		** get globals into registers.
		** either we just started, or we just finished a reduction
		*/
	yystack:
		yy_pv = yypv;
		yy_ps = yyps;
		yy_state = yystate;

		/*
		** top of for (;;) loop while no reductions done
		*/
	yy_stack:
		/*
		** put a state and value onto the stacks
		*/
#if YYDEBUG
		/*
		** if debugging, look up token value in list of value vs.
		** name pairs.  0 and negative (-1) are special values.
		** Note: linear search is used since time is not a real
		** consideration while debugging.
		*/
		if ( yydebug )
		{
			register int yy_i;

			printf( "State %d, token ", yy_state );
			if ( yychar == 0 )
				printf( "end-of-file\n" );
			else if ( yychar < 0 )
				printf( "-none-\n" );
			else
			{
				for ( yy_i = 0; yytoks[yy_i].t_val >= 0;
					yy_i++ )
				{
					if ( yytoks[yy_i].t_val == yychar )
						break;
				}
				printf( "%s\n", yytoks[yy_i].t_name );
			}
		}
#endif /* YYDEBUG */
		if ( ++yy_ps >= &yys[ yymaxdepth ] )	/* room on stack? */
		{
			/*
			** reallocate and recover.  Note that pointers
			** have to be reset, or bad things will happen
			*/
			long yyps_index = (yy_ps - yys);
			long yypv_index = (yy_pv - yyv);
			long yypvt_index = (yypvt - yyv);
			int yynewmax;
#ifdef YYEXPAND
			yynewmax = YYEXPAND(yymaxdepth);
#else
			yynewmax = 2 * yymaxdepth;	/* double table size */
			if (yymaxdepth == YYMAXDEPTH)	/* first time growth */
			{
				char *newyys = (char *)YYNEW(int);
				char *newyyv = (char *)YYNEW(YYSTYPE);
				if (newyys != 0 && newyyv != 0)
				{
					yys = YYCOPY(newyys, yys, int);
					yyv = YYCOPY(newyyv, yyv, YYSTYPE);
				}
				else
					yynewmax = 0;	/* failed */
			}
			else				/* not first time */
			{
				yys = YYENLARGE(yys, int);
				yyv = YYENLARGE(yyv, YYSTYPE);
				if (yys == 0 || yyv == 0)
					yynewmax = 0;	/* failed */
			}
#endif
			if (yynewmax <= yymaxdepth)	/* tables not expanded */
			{
				yyerror( "yacc stack overflow" );
				YYABORT;
			}
			yymaxdepth = yynewmax;

			yy_ps = yys + yyps_index;
			yy_pv = yyv + yypv_index;
			yypvt = yyv + yypvt_index;
		}
		*yy_ps = yy_state;
		*++yy_pv = yyval;

		/*
		** we have a new state - find out what to do
		*/
	yy_newstate:
		if ( ( yy_n = yypact[ yy_state ] ) <= YYFLAG )
			goto yydefault;		/* simple state */
#if YYDEBUG
		/*
		** if debugging, need to mark whether new token grabbed
		*/
		yytmp = yychar < 0;
#endif
		if ( ( yychar < 0 ) && ( ( yychar = YYLEX() ) < 0 ) )
			yychar = 0;		/* reached EOF */
#if YYDEBUG
		if ( yydebug && yytmp )
		{
			register int yy_i;

			printf( "Received token " );
			if ( yychar == 0 )
				printf( "end-of-file\n" );
			else if ( yychar < 0 )
				printf( "-none-\n" );
			else
			{
				for ( yy_i = 0; yytoks[yy_i].t_val >= 0;
					yy_i++ )
				{
					if ( yytoks[yy_i].t_val == yychar )
						break;
				}
				printf( "%s\n", yytoks[yy_i].t_name );
			}
		}
#endif /* YYDEBUG */
		if ( ( ( yy_n += yychar ) < 0 ) || ( yy_n >= YYLAST ) )
			goto yydefault;
		if ( yychk[ yy_n = yyact[ yy_n ] ] == yychar )	/*valid shift*/
		{
			yychar = -1;
			yyval = yylval;
			yy_state = yy_n;
			if ( yyerrflag > 0 )
				yyerrflag--;
			goto yy_stack;
		}

	yydefault:
		if ( ( yy_n = yydef[ yy_state ] ) == -2 )
		{
#if YYDEBUG
			yytmp = yychar < 0;
#endif
			if ( ( yychar < 0 ) && ( ( yychar = YYLEX() ) < 0 ) )
				yychar = 0;		/* reached EOF */
#if YYDEBUG
			if ( yydebug && yytmp )
			{
				register int yy_i;

				printf( "Received token " );
				if ( yychar == 0 )
					printf( "end-of-file\n" );
				else if ( yychar < 0 )
					printf( "-none-\n" );
				else
				{
					for ( yy_i = 0;
						yytoks[yy_i].t_val >= 0;
						yy_i++ )
					{
						if ( yytoks[yy_i].t_val
							== yychar )
						{
							break;
						}
					}
					printf( "%s\n", yytoks[yy_i].t_name );
				}
			}
#endif /* YYDEBUG */
			/*
			** look through exception table
			*/
			{
				register const int *yyxi = yyexca;

				while ( ( *yyxi != -1 ) ||
					( yyxi[1] != yy_state ) )
				{
					yyxi += 2;
				}
				while ( ( *(yyxi += 2) >= 0 ) &&
					( *yyxi != yychar ) )
					;
				if ( ( yy_n = yyxi[1] ) < 0 )
					YYACCEPT;
			}
		}

		/*
		** check for syntax error
		*/
		if ( yy_n == 0 )	/* have an error */
		{
			/* no worry about speed here! */
			switch ( yyerrflag )
			{
			case 0:		/* new error */
				yyerror( "syntax error" );
				goto skip_init;
			yyerrlab:
				/*
				** get globals into registers.
				** we have a user generated syntax type error
				*/
				yy_pv = yypv;
				yy_ps = yyps;
				yy_state = yystate;
			skip_init:
				yynerrs++;
				/* FALLTHRU */
			case 1:
			case 2:		/* incompletely recovered error */
					/* try again... */
				yyerrflag = 3;
				/*
				** find state where "error" is a legal
				** shift action
				*/
				while ( yy_ps >= yys )
				{
					yy_n = yypact[ *yy_ps ] + YYERRCODE;
					if ( yy_n >= 0 && yy_n < YYLAST &&
						yychk[yyact[yy_n]] == YYERRCODE)					{
						/*
						** simulate shift of "error"
						*/
						yy_state = yyact[ yy_n ];
						goto yy_stack;
					}
					/*
					** current state has no shift on
					** "error", pop stack
					*/
#if YYDEBUG
#	define _POP_ "Error recovery pops state %d, uncovers state %d\n"
					if ( yydebug )
						printf( _POP_, *yy_ps,
							yy_ps[-1] );
#	undef _POP_
#endif
					yy_ps--;
					yy_pv--;
				}
				/*
				** there is no state on stack with "error" as
				** a valid shift.  give up.
				*/
				YYABORT;
			case 3:		/* no shift yet; eat a token */
#if YYDEBUG
				/*
				** if debugging, look up token in list of
				** pairs.  0 and negative shouldn't occur,
				** but since timing doesn't matter when
				** debugging, it doesn't hurt to leave the
				** tests here.
				*/
				if ( yydebug )
				{
					register int yy_i;

					printf( "Error recovery discards " );
					if ( yychar == 0 )
						printf( "token end-of-file\n" );
					else if ( yychar < 0 )
						printf( "token -none-\n" );
					else
					{
						for ( yy_i = 0;
							yytoks[yy_i].t_val >= 0;
							yy_i++ )
						{
							if ( yytoks[yy_i].t_val
								== yychar )
							{
								break;
							}
						}
						printf( "token %s\n",
							yytoks[yy_i].t_name );
					}
				}
#endif /* YYDEBUG */
				if ( yychar == 0 )	/* reached EOF. quit */
					YYABORT;
				yychar = -1;
				goto yy_newstate;
			}
		}/* end if ( yy_n == 0 ) */
		/*
		** reduction by production yy_n
		** put stack tops, etc. so things right after switch
		*/
#if YYDEBUG
		/*
		** if debugging, print the string that is the user's
		** specification of the reduction which is just about
		** to be done.
		*/
		if ( yydebug )
			printf( "Reduce by (%d) \"%s\"\n",
				yy_n, yyreds[ yy_n ] );
#endif
		yytmp = yy_n;			/* value to switch over */
		yypvt = yy_pv;			/* $vars top of value stack */
		/*
		** Look in goto table for next state
		** Sorry about using yy_state here as temporary
		** register variable, but why not, if it works...
		** If yyr2[ yy_n ] doesn't have the low order bit
		** set, then there is no action to be done for
		** this reduction.  So, no saving & unsaving of
		** registers done.  The only difference between the
		** code just after the if and the body of the if is
		** the goto yy_stack in the body.  This way the test
		** can be made before the choice of what to do is needed.
		*/
		{
			/* length of production doubled with extra bit */
			register int yy_len = yyr2[ yy_n ];

			if ( !( yy_len & 01 ) )
			{
				yy_len >>= 1;
				yyval = ( yy_pv -= yy_len )[1];	/* $$ = $1 */
				yy_state = yypgo[ yy_n = yyr1[ yy_n ] ] +
					*( yy_ps -= yy_len ) + 1;
				if ( yy_state >= YYLAST ||
					yychk[ yy_state =
					yyact[ yy_state ] ] != -yy_n )
				{
					yy_state = yyact[ yypgo[ yy_n ] ];
				}
				goto yy_stack;
			}
			yy_len >>= 1;
			yyval = ( yy_pv -= yy_len )[1];	/* $$ = $1 */
			yy_state = yypgo[ yy_n = yyr1[ yy_n ] ] +
				*( yy_ps -= yy_len ) + 1;
			if ( yy_state >= YYLAST ||
				yychk[ yy_state = yyact[ yy_state ] ] != -yy_n )
			{
				yy_state = yyact[ yypgo[ yy_n ] ];
			}
		}
					/* save until reenter driver code */
		yystate = yy_state;
		yyps = yy_ps;
		yypv = yy_pv;
	}
	/*
	** code supplied by user is placed in this switch
	*/
	switch( yytmp )
	{
		
case 4:
# line 220 "shell.yacc"
{ printValue (&yypvt[-0]); CHECK; } break;
case 6:
# line 224 "shell.yacc"
{ yypvt[-0].side = RHS; setRv (&yyval, &yypvt[-0]); } break;
case 7:
# line 225 "shell.yacc"
{ yyval = yypvt[-0]; yyval.value.rv = newString((char*)yypvt[-0].value.rv);
			   CHECK; } break;
case 11:
# line 230 "shell.yacc"
{ yyval = yypvt[-1]; } break;
case 12:
# line 232 "shell.yacc"
{ yyval = funcCall (&yypvt[-3], &yypvt[-1]); CHECK; } break;
case 13:
# line 234 "shell.yacc"
{ 
			typeConvert (&yypvt[-0], yypvt[-1].type, yypvt[-1].side); yyval = yypvt[-0];
			CHECK;
			} break;
case 14:
# line 238 "shell.yacc"
{ VALUE tmp;
					  (void)getRv (&yypvt[-0], &tmp);
					  setLv (&yyval, &tmp);
					  CHECK;
					} break;
case 15:
# line 243 "shell.yacc"
{ yyval.value.rv = (int)getLv (&yypvt[-0]);
					  yyval.type = T_INT; yyval.side = RHS; } break;
case 16:
# line 245 "shell.yacc"
{ rvOp (RV(yypvt[-0]), UMINUS, NULLVAL); } break;
case 17:
# line 246 "shell.yacc"
{ rvOp (RV(yypvt[-0]), '!', NULLVAL); } break;
case 18:
# line 247 "shell.yacc"
{ rvOp (RV(yypvt[-0]), '~', NULLVAL); } break;
case 19:
# line 248 "shell.yacc"
{ setRv (&yyval, RV(yypvt[-4])->value.rv ? &yypvt[-2]
								       : &yypvt[-0]); } break;
case 20:
# line 250 "shell.yacc"
{ BIN_OP ('+');
					  typeConvert (&yyval, T_INT, RHS);
					  setLv (&yyval, &yyval); } break;
case 21:
# line 253 "shell.yacc"
{ BIN_OP ('+');
					  typeConvert (&yyval, T_INT, RHS);
					  setLv (&yyval, &yyval); } break;
case 22:
# line 256 "shell.yacc"
{ BIN_OP ('+'); } break;
case 23:
# line 257 "shell.yacc"
{ BIN_OP ('-'); } break;
case 24:
# line 258 "shell.yacc"
{ BIN_OP ('*'); } break;
case 25:
# line 259 "shell.yacc"
{ BIN_OP ('/'); } break;
case 26:
# line 260 "shell.yacc"
{ BIN_OP ('%'); } break;
case 27:
# line 261 "shell.yacc"
{ BIN_OP (ROT_RIGHT); } break;
case 28:
# line 262 "shell.yacc"
{ BIN_OP (ROT_LEFT); } break;
case 29:
# line 263 "shell.yacc"
{ BIN_OP ('&'); } break;
case 30:
# line 264 "shell.yacc"
{ BIN_OP ('^'); } break;
case 31:
# line 265 "shell.yacc"
{ BIN_OP ('|'); } break;
case 32:
# line 266 "shell.yacc"
{ BIN_OP (AND); } break;
case 33:
# line 267 "shell.yacc"
{ BIN_OP (OR); } break;
case 34:
# line 268 "shell.yacc"
{ BIN_OP (EQ); } break;
case 35:
# line 269 "shell.yacc"
{ BIN_OP (NE); } break;
case 36:
# line 270 "shell.yacc"
{ BIN_OP (GE); } break;
case 37:
# line 271 "shell.yacc"
{ BIN_OP (LE); } break;
case 38:
# line 272 "shell.yacc"
{ BIN_OP ('>'); } break;
case 39:
# line 273 "shell.yacc"
{ BIN_OP ('<'); } break;
case 40:
# line 274 "shell.yacc"
{ rvOp (RV(yypvt[-0]), INCR, NULLVAL);
						  assign (&yypvt[-0], &yyval); CHECK; } break;
case 41:
# line 276 "shell.yacc"
{ rvOp (RV(yypvt[-0]), DECR, NULLVAL);
						  assign (&yypvt[-0], &yyval); CHECK; } break;
case 42:
# line 278 "shell.yacc"
{ VALUE tmp;
						  tmp = yypvt[-1];
						  rvOp (RV(yypvt[-1]), INCR, NULLVAL);
						  assign (&yypvt[-1], &yyval); CHECK;
						  yyval = tmp; } break;
case 43:
# line 283 "shell.yacc"
{ VALUE tmp;
						  tmp = yypvt[-1];
						  rvOp (RV(yypvt[-1]), DECR, NULLVAL);
						  assign (&yypvt[-1], &yyval); CHECK;
						  yyval = tmp; } break;
case 44:
# line 288 "shell.yacc"
{ BIN_OP (ADDA); assign (&yypvt[-2], &yyval); CHECK;} break;
case 45:
# line 289 "shell.yacc"
{ BIN_OP (SUBA); assign (&yypvt[-2], &yyval); CHECK;} break;
case 46:
# line 290 "shell.yacc"
{ BIN_OP (ANDA); assign (&yypvt[-2], &yyval); CHECK;} break;
case 47:
# line 291 "shell.yacc"
{ BIN_OP (ORA);  assign (&yypvt[-2], &yyval); CHECK;} break;
case 48:
# line 292 "shell.yacc"
{ BIN_OP (MODA); assign (&yypvt[-2], &yyval); CHECK;} break;
case 49:
# line 293 "shell.yacc"
{ BIN_OP (XORA); assign (&yypvt[-2], &yyval); CHECK;} break;
case 50:
# line 294 "shell.yacc"
{ BIN_OP (MULA); assign (&yypvt[-2], &yyval); CHECK;} break;
case 51:
# line 295 "shell.yacc"
{ BIN_OP (DIVA); assign (&yypvt[-2], &yyval); CHECK;} break;
case 52:
# line 296 "shell.yacc"
{ BIN_OP (SHLA); assign (&yypvt[-2], &yyval); CHECK;} break;
case 53:
# line 297 "shell.yacc"
{ BIN_OP (SHRA); assign (&yypvt[-2], &yyval); CHECK;} break;
case 54:
# line 298 "shell.yacc"
{ assign (&yypvt[-2], &yypvt[-0]); yyval = yypvt[-2]; } break;
case 55:
# line 303 "shell.yacc"
{ usymFlag = TRUE; usymVal = yypvt[-0]; } break;
case 56:
# line 305 "shell.yacc"
{
			if (yypvt[-3].type != T_UNKNOWN)
			    {
			    printf ("typecast of lhs not allowed.\n");
			    YYERROR;
			    }
			else
			    {
			    yyval = newSym ((char *)yypvt[-3].value.rv, yypvt[-0].type); CHECK;
			    assign (&yyval, &yypvt[-0]); CHECK;
			    }
			usymFlag = FALSE;
			} break;
case 57:
# line 321 "shell.yacc"
{ yyval = newArgList (); } break;
case 59:
# line 326 "shell.yacc"
{ yyval = newArgList (); addArg (&yyval, &yypvt[-0]); CHECK; } break;
case 60:
# line 328 "shell.yacc"
{ addArg (&yypvt[-2], &yypvt[-0]); CHECK; } break;
case 61:
# line 331 "shell.yacc"
{ yypvt[-1].side = RHS; yyval = yypvt[-1]; } break;
case 62:
# line 332 "shell.yacc"
{ yypvt[-3].side = FHS; yyval = yypvt[-3]; } break;
# line	531 "/usr/ccs/bin/yaccpar"
	}
	goto yystack;		/* reset registers in driver code */
}


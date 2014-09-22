/* mathHardLib.c - hardware floating point math library */

/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01c,05sep01,hdn  added cosh, sinh, tanh support(spr 25654)
01b,08jun93,hdn  updated to 5.1
		  -changed copyright notice
		  -changed VOID to void
		  -made logMsg calls indirect to reduce coupling
		  -changed includes to have absolute path from h/
01a,16sep92,hdn  written by modifying Tron's mathHardLib.c.
*/

/*
DESCRIPTION

This library provides support routines for using the hardware floating
point coprocessor i80387 with high-level math functions.
The high level functions include triginometric operations, exponents,
etc.  

The routines in this library will automatically be used for high-level
math functions only if mathHardInit() has been called previously.  

*/

#include "vxWorks.h"
#include "math.h"
#include "logLib.h"
#include "fppLib.h"
#include "intLib.h"
#include "private/funcBindP.h"

/* Externals */

extern double  mathHardAcos ();		/* functions in mathHardALib.s */
extern double  mathHardAsin ();
extern double  mathHardAtan ();
extern double  mathHardAtan2 ();
extern double  mathHardCeil ();
extern double  mathHardCos ();
extern double  mathHardExp ();
extern double  mathHardFabs ();
extern double  mathHardFloor ();
extern double  mathHardFmod ();
extern double  mathHardInfinity ();
extern int     mathHardIrint ();
extern int     mathHardIround ();
extern double  mathHardLog ();
extern double  mathHardLog2 ();
extern double  mathHardLog10 ();
extern double  mathHardPow ();
extern double  mathHardRound ();
extern double  mathHardSin ();
extern void    mathHardSincos ();
extern double  mathHardSqrt ();
extern double  mathHardTan ();
extern double  mathHardTrunc ();
extern double  cosh ();			/* functions in libc/math */
extern double  sinh ();
extern double  tanh ();

extern DBLFUNCPTR	mathAcosFunc;	/* double-precision function ptrs */
extern DBLFUNCPTR	mathAsinFunc;
extern DBLFUNCPTR	mathAtanFunc;
extern DBLFUNCPTR	mathAtan2Func;
extern DBLFUNCPTR	mathCbrtFunc;
extern DBLFUNCPTR	mathCeilFunc;
extern DBLFUNCPTR	mathCosFunc;
extern DBLFUNCPTR	mathCoshFunc;
extern DBLFUNCPTR	mathExpFunc;
extern DBLFUNCPTR	mathFabsFunc;
extern DBLFUNCPTR	mathFloorFunc;
extern DBLFUNCPTR	mathFmodFunc;
extern DBLFUNCPTR	mathHypotFunc;
extern DBLFUNCPTR	mathInfinityFunc;
extern FUNCPTR		mathIrintFunc;
extern FUNCPTR		mathIroundFunc;
extern DBLFUNCPTR	mathLogFunc;
extern DBLFUNCPTR	mathLog2Func;
extern DBLFUNCPTR	mathLog10Func;
extern DBLFUNCPTR	mathPowFunc;
extern DBLFUNCPTR	mathRoundFunc;
extern DBLFUNCPTR	mathSinFunc;
extern VOIDFUNCPTR	mathSincosFunc;
extern DBLFUNCPTR	mathSinhFunc;
extern DBLFUNCPTR	mathSqrtFunc;
extern DBLFUNCPTR	mathTanFunc;
extern DBLFUNCPTR	mathTanhFunc;
extern DBLFUNCPTR	mathTruncFunc;

extern FLTFUNCPTR	mathFacosFunc;	/* single-precision function ptrs */
extern FLTFUNCPTR	mathFasinFunc;
extern FLTFUNCPTR	mathFatanFunc;
extern FLTFUNCPTR	mathFatan2Func;
extern FLTFUNCPTR	mathFcbrtFunc;
extern FLTFUNCPTR	mathFceilFunc;
extern FLTFUNCPTR	mathFcosFunc;
extern FLTFUNCPTR	mathFcoshFunc;
extern FLTFUNCPTR	mathFexpFunc;
extern FLTFUNCPTR	mathFfabsFunc;
extern FLTFUNCPTR	mathFfloorFunc;
extern FLTFUNCPTR	mathFfmodFunc;
extern FLTFUNCPTR	mathFhypotFunc;
extern FLTFUNCPTR	mathFinfinityFunc;
extern FUNCPTR		mathFirintFunc;
extern FUNCPTR		mathFiroundFunc;
extern FLTFUNCPTR	mathFlogFunc;
extern FLTFUNCPTR	mathFlog2Func;
extern FLTFUNCPTR	mathFlog10Func;
extern FLTFUNCPTR	mathFpowFunc;
extern FLTFUNCPTR	mathFroundFunc;
extern FLTFUNCPTR	mathFsinFunc;
extern VOIDFUNCPTR	mathFsincosFunc;
extern FLTFUNCPTR	mathFsinhFunc;
extern FLTFUNCPTR	mathFsqrtFunc;
extern FLTFUNCPTR	mathFtanFunc;
extern FLTFUNCPTR	mathFtanhFunc;
extern FLTFUNCPTR	mathFtruncFunc;

extern void		mathErrNoInit (); /* initial value of func ptrs */


/* Forward declarations */

LOCAL void	mathHardNoSingle ();
LOCAL void	mathHardCbrt ();
LOCAL void	mathHardHypot ();


/******************************************************************************
*
* mathHardInit - initialize hardware floating point math support
*
* This routine is called from usrConfig if INCLUDE_I80387
* is defined.  This causes the linker to include the floating point
* hardware support library.
*
* This routine places the addresses of the hardware high-level math functions
* (triginometric functions, etc.) in a set of global variables.  This
* allows the standard math functions (e.g. sin(), pow()) to have a single
* entry point but be dispatched to the hardware or software support
* routines, as specified.
* 
* Certain routines which are present in the floating point software emulation
* library do not have equivalent hardware support routines.  (These are
* primarily routines which handle single-precision floating point.)  If
* no emulation routine address has already been put in the global variable
* for this function, the address of a dummy routine which logs an error
* message is placed in the variable; if an emulation routine address is
* present (the emulation initialization, via mathSoftInit(), must be done 
* prior to hardware floating point initialization), the emulation routine 
* address is left alone.  In this way, hardware routines will be used for all 
* available functions, while emulation will be used for the missing functions.
*
* SEE ALSO: mathSoftInit()
*
*/

void mathHardInit ()
    {

    /* Don't do more unless there really is hardware support */

    fppInit();			/* attempt to init hardware support */

    if (fppProbe() != OK)	/* is there a coprocessor ? */
	return;			/*  exit if no */
    
    /* Load hardware routine addresses into global variables
     * defined in mathALib.s.
     */

    /* Double-precision routines */
    
    if (mathAcosFunc == (DBLFUNCPTR) mathErrNoInit)
        mathAcosFunc = mathHardAcos;

    if (mathAsinFunc == (DBLFUNCPTR) mathErrNoInit)
        mathAsinFunc = mathHardAsin;

    if (mathAtanFunc == (DBLFUNCPTR) mathErrNoInit)
        mathAtanFunc = mathHardAtan;

    if (mathAtan2Func == (DBLFUNCPTR) mathErrNoInit)
        mathAtan2Func = mathHardAtan2;

    if (mathCeilFunc == (DBLFUNCPTR) mathErrNoInit)
        mathCeilFunc = mathHardCeil;

    if (mathCosFunc == (DBLFUNCPTR) mathErrNoInit)
        mathCosFunc = mathHardCos;

    if (mathCoshFunc == (DBLFUNCPTR) mathErrNoInit)
        mathCoshFunc = cosh;		/* drag in libc/math/cosh */

    if (mathExpFunc == (DBLFUNCPTR) mathErrNoInit)
        mathExpFunc = mathHardExp;

    if (mathFabsFunc == (DBLFUNCPTR) mathErrNoInit)
        mathFabsFunc = mathHardFabs;

    if (mathFmodFunc == (DBLFUNCPTR) mathErrNoInit)
        mathFmodFunc = mathHardFmod;

    if (mathFloorFunc == (DBLFUNCPTR) mathErrNoInit)
        mathFloorFunc = mathHardFloor;

    if (mathInfinityFunc == (DBLFUNCPTR) mathErrNoInit)
        mathInfinityFunc = mathHardInfinity;

    if (mathIrintFunc == (FUNCPTR) mathErrNoInit)
        mathIrintFunc = mathHardIrint;

    if (mathIroundFunc == (FUNCPTR) mathErrNoInit)
        mathIroundFunc = mathHardIround;

    if (mathLogFunc == (DBLFUNCPTR) mathErrNoInit)
        mathLogFunc = mathHardLog;

    if (mathLog2Func == (DBLFUNCPTR) mathErrNoInit)
        mathLog2Func = mathHardLog2;

    if (mathLog10Func == (DBLFUNCPTR) mathErrNoInit)
        mathLog10Func = mathHardLog10;

    if (mathPowFunc == (DBLFUNCPTR) mathErrNoInit)
        mathPowFunc = mathHardPow;

    if (mathRoundFunc == (DBLFUNCPTR) mathErrNoInit)
        mathRoundFunc = mathHardRound;

    if (mathSinFunc == (DBLFUNCPTR) mathErrNoInit)
        mathSinFunc = mathHardSin;

    if (mathSincosFunc == mathErrNoInit)
        mathSincosFunc = mathHardSincos;

    if (mathSinhFunc == (DBLFUNCPTR) mathErrNoInit)
        mathSinhFunc = sinh;		/* drag in libc/math/sinh */

    if (mathSqrtFunc == (DBLFUNCPTR) mathErrNoInit)
        mathSqrtFunc = mathHardSqrt;

    if (mathTanFunc == (DBLFUNCPTR) mathErrNoInit)
        mathTanFunc = mathHardTan;

    if (mathTanhFunc == (DBLFUNCPTR) mathErrNoInit)
        mathTanhFunc = tanh;		/* drag in libc/math/tanh */

    if (mathTruncFunc == (DBLFUNCPTR) mathErrNoInit)
        mathTruncFunc = mathHardTrunc;


    /* Single-precision functions (unsupported) */
    
    if (mathFacosFunc == (FLTFUNCPTR) mathErrNoInit)
    	mathFacosFunc = (FLTFUNCPTR) mathHardNoSingle;

    if (mathFasinFunc == (FLTFUNCPTR) mathErrNoInit)
	mathFasinFunc = (FLTFUNCPTR) mathHardNoSingle;

    if (mathFatanFunc == (FLTFUNCPTR) mathErrNoInit)
	mathFatanFunc = (FLTFUNCPTR) mathHardNoSingle;

    if (mathFatan2Func == (FLTFUNCPTR) mathErrNoInit)
	mathFatan2Func = (FLTFUNCPTR) mathHardNoSingle;

    if (mathFcbrtFunc == (FLTFUNCPTR) mathErrNoInit)
	mathFcbrtFunc = (FLTFUNCPTR) mathHardNoSingle;

    if (mathFceilFunc == (FLTFUNCPTR) mathErrNoInit)
	mathFceilFunc = (FLTFUNCPTR) mathHardNoSingle;

    if (mathFcosFunc == (FLTFUNCPTR) mathErrNoInit)
	mathFcosFunc = (FLTFUNCPTR) mathHardNoSingle;

    if (mathFcoshFunc == (FLTFUNCPTR) mathErrNoInit)
	mathFcoshFunc = (FLTFUNCPTR) mathHardNoSingle;

    if (mathFexpFunc == (FLTFUNCPTR) mathErrNoInit)
	mathFexpFunc = (FLTFUNCPTR) mathHardNoSingle;

    if (mathFfabsFunc == (FLTFUNCPTR) mathErrNoInit)
	mathFfabsFunc = (FLTFUNCPTR) mathHardNoSingle;

    if (mathFfmodFunc == (FLTFUNCPTR) mathErrNoInit)
	mathFfmodFunc = (FLTFUNCPTR) mathHardNoSingle;

    if (mathFfloorFunc == (FLTFUNCPTR) mathErrNoInit)
	mathFfloorFunc = (FLTFUNCPTR) mathHardNoSingle;

    if (mathFhypotFunc == (FLTFUNCPTR) mathErrNoInit)
	mathFhypotFunc = (FLTFUNCPTR) mathHardNoSingle;

    if (mathFinfinityFunc == (FLTFUNCPTR) mathErrNoInit)
	mathFinfinityFunc = (FLTFUNCPTR) mathHardNoSingle;

    if (mathFirintFunc == (FUNCPTR) mathErrNoInit)
	mathFirintFunc = (FUNCPTR) mathHardNoSingle;

    if (mathFiroundFunc == (FUNCPTR) mathErrNoInit)
	mathFiroundFunc = (FUNCPTR) mathHardNoSingle;

    if (mathFlogFunc == (FLTFUNCPTR) mathErrNoInit)
	mathFlogFunc = (FLTFUNCPTR) mathHardNoSingle;

    if (mathFlog2Func == (FLTFUNCPTR) mathErrNoInit)
	mathFlog2Func = (FLTFUNCPTR) mathHardNoSingle;

    if (mathFlog10Func == (FLTFUNCPTR) mathErrNoInit)
	mathFlog10Func = (FLTFUNCPTR) mathHardNoSingle;

    if (mathFpowFunc == (FLTFUNCPTR) mathErrNoInit)
	mathFpowFunc = (FLTFUNCPTR) mathHardNoSingle;

    if (mathFroundFunc == (FLTFUNCPTR) mathErrNoInit)
	mathFroundFunc = (FLTFUNCPTR) mathHardNoSingle;

    if (mathFsinFunc == (FLTFUNCPTR) mathErrNoInit)
	mathFsinFunc = (FLTFUNCPTR) mathHardNoSingle;

    if (mathFsincosFunc == mathErrNoInit)
	mathFsincosFunc = mathHardNoSingle;

    if (mathFsinhFunc == (FLTFUNCPTR) mathErrNoInit)
	mathFsinhFunc = (FLTFUNCPTR) mathHardNoSingle;

    if (mathFsqrtFunc == (FLTFUNCPTR) mathErrNoInit)
	mathFsqrtFunc = (FLTFUNCPTR) mathHardNoSingle;

    if (mathFtanFunc == (FLTFUNCPTR) mathErrNoInit)
	mathFtanFunc = (FLTFUNCPTR) mathHardNoSingle;

    if (mathFtanhFunc == (FLTFUNCPTR) mathErrNoInit)
	mathFtanhFunc = (FLTFUNCPTR) mathHardNoSingle;

    if (mathFtruncFunc == (FLTFUNCPTR) mathErrNoInit)
	mathFtruncFunc = (FLTFUNCPTR) mathHardNoSingle;


    /* Miscellaneous unsupported functions */

    if (mathCbrtFunc == (DBLFUNCPTR) mathErrNoInit)
    	mathCbrtFunc = (DBLFUNCPTR) mathHardCbrt;

    if (mathHypotFunc == (DBLFUNCPTR) mathErrNoInit)
    	mathHypotFunc = (DBLFUNCPTR) mathHardHypot;

    }

/******************************************************************************
*
* mathHardNoSingle - log error message for unsupported single-precision fp
*
* This routine will generate a log message to the VxWorks console if
* an attempt is made to use single-precision math functions with the
* hardware floating point coprocessor (not supported).
*
*/

LOCAL void mathHardNoSingle ()
    {

    if (_func_logMsg != NULL)
	(* _func_logMsg) ("ERROR - single-precision flt. point not supported\n",
			  0,0,0,0,0,0);
    }

/******************************************************************************
*
* mathHardCbrt - log error message for unsupported cube root function
*
* This routine will generate a log message to the VxWorks console if
* an attempt is made to use the cbrt() cube root function with the
* hardware floating point coprocessor (not supported).
*
*/

LOCAL void mathHardCbrt ()
    {

    if (_func_logMsg != NULL)
        (* _func_logMsg) ("ERROR - h/w flt. point cube root not supported\n",
			  0,0,0,0,0,0);
    }

/******************************************************************************
*
* mathHardHypot - log error message for unsupported hypot function
*
* This routine will generate a log message to the VxWorks console if
* an attempt is made to use the hypot() Euclidean distance function with 
* the hardware floating point coprocessor (not supported).
*
*/

LOCAL void mathHardHypot ()
    {

    if (_func_logMsg != NULL)
        (* _func_logMsg) ("ERROR - h/w flt. point hypot not supported\n",
			  0,0,0,0,0,0);
    }


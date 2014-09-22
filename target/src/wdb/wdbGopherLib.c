/* wdbGopherLib.c - info gathering interpreter for the agent */

/* Copyright 1994-2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01l,14sep01,jhw Fixed warnings from compiling with gnu -pedantic flag
01k,09sep97,elp added global variable to prevent taskLock() calls (SPR# 7653)
		+ replaced hardcoded value by macro.
01j,10dec96,elp gopherWriteString() cleanup.
01i,09dec96,elp fixed null length string error (gopher type was not written).
01h,30apr96,elp	Merged with the host version written by bss (SPR# 6497).
		+ enabled transfer of larger strings (SPR #6277)
		+ fix SPR #6403.
01g,20mar96,s_w modify the gopherWriteString to cope with strings near memory
		boundaries (SPR 6218).
01f,31oct95,ms  changed a couple of strtol's to strtoul's.
01e,14sep95,c_s added more syntax checking, bug fixes, and memory protection.
		(SPRs 4659, 4462).
01d,23jun95,tpr replaced memcpy() by bcopy()
01c,20jun95,ms	added static buffer back in to make DSA easier
01b,01jun95,ms	added interlocking in task mode, removed static buffer.
01a,01nov94,c_s written.
*/

/*
DESCRIPTION

*/

#ifdef HOST

#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#if 0   /* omitted because host.h redefines memset () and memchr () */
#include <memory.h>
#else   /* which breaks the declaration of memcpy () ... */
extern "C" void * memcpy (void * s1, const void * s2, size_t n);
#endif

#include "host.h"
#include "wdb.h"

#include "backend.h"

#else /* #ifdef HOST */

#include "wdb/wdb.h"
#include "wdb/wdbRtIfLib.h"
#include "wdb/wdbLibP.h"
#include "wdb/wdbSvcLib.h"
#include "limits.h"
#include "string.h"
#include "ctype.h"
#include "stdlib.h"

#endif  /* #ifdef HOST */

typedef struct
    {
    char *		program;	/* where to start executing */
    UINT32		p;		/* initial value of pointer */
    char *		pTape;		/* tape array */
    int			tapeIx;		/* index in tape to write next cell */
    int			tapeLen;	/* total length of tape */
    int			execute;	/* whether to execute, or just parse */
    unsigned int	status;		/* error code */
    } wdbGopherCtx_t;

/* The Gopher Byte Stream Format
 * 
 * The first byte of an item is the type code, one of GOPHER_UINT32, 
 * GOPHER_UINT16, GOPHER_FLOAT64, etc.  This code determines the number
 * of bytes that immediately follow and represent the tape cell value.
 * These data bytes will be in the target byte order.  If the type code
 * is GOPHER_STRING, all following bytes are part of the string up to 
 * a trailing NUL character.
 */

#define MAX_TAPE_LEN	1400

#ifdef HOST

/*
* XXX - because some emulator back ends only provide low-bandwidth
* memory read capabilities, the host implementation assumes that
* the maximum size of a string in target memory is 32 bytes.
* This is necessary for performance.  However, gopherWriteString()
* should be modified to make several small reads until it finds
* the end of a string.
*/
const int       MaxTgtStrLen    = 32;   /* XXX - Maximum length of a string */

#else

#define ADD_TAPE_NB	10

BOOL		wdbGopherLock = TRUE;	  /* lock during gopher evaluation */
static char *	pAddTape [ADD_TAPE_NB];	  /* additional tape pointers */
static int	pAddTapeIx [ADD_TAPE_NB]; /* additional tape fill indexes */
static int	tapeIndex;		  /* current tape number */

#endif /* #ifdef HOST */

static char 	tape [MAX_TAPE_LEN];	/* XXX should be passed to
					 * wdbGopherLib(), but that would
					 * make the host based DSA harder
					 * to implement */
static int	tapeLen = MAX_TAPE_LEN;

#define WDB_REPLY_HDR_SZ	24
#define WDB_WRAPPER_HDR_SZ	12
#define WDB_MEM_XFER_HDR_SZ	8
#define WDB_OPAQUE_HDR_SZ	4

/* it is required to reserve space for headers (WDB + XDR + IP) */

#define WDB_GOPHER_MTU_OFFSET  (WDB_REPLY_HDR_SZ + WDB_WRAPPER_HDR_SZ + \
				WDB_MEM_XFER_HDR_SZ + WDB_OPAQUE_HDR_SZ + 8)

/* forward declarations */

#ifdef HOST

/*
 * Note:  although this is a '.c' file, it must be
 * compiled with the '-x c++' option.
 */
extern "C" UINT32 wdbEvaluateGopher (WDB_STRING_T *program,
                                     WDB_MEM_XFER *pTape);

static STATUS gopherWriteP (wdbGopherCtx_t *gc);

#else

static UINT32 wdbEvaluateGopher (WDB_STRING_T *program, WDB_MEM_XFER *pTape);
static STATUS wdbTransferGopher (WDB_MEM_XFER *pTape);
static STATUS gopherNewTapeAllocate (wdbGopherCtx_t *gc);

#endif /* #ifdef HOST */

static STATUS gopher		(wdbGopherCtx_t *gc);
static STATUS gopherWriteScalar	(wdbGopherCtx_t *gc, UINT8 *src, int type);
static STATUS gopherWriteString (wdbGopherCtx_t *gc, char *string);

/******************************************************************************
*
* gopher -
*/

static STATUS gopher 
    (
    wdbGopherCtx_t *	gc		/* gopher execution context */
    )

    {
    while (*gc->program)
	{
	/* skip whitespace */

	while (*gc->program && isspace ((int) *gc->program))
	    ++gc->program;

	if (isxdigit ((int) *gc->program)) /* unsigned constants replace p */
	    {
	    char *newPc;

#ifdef HOST
            /* XXX - this is a work around for the bug in GNU's strtoul (). */
            unsigned int newP = (unsigned int) strtol (gc->program, &newPc, 0);
#else
	    unsigned int newP = (unsigned int) strtoul (gc->program, &newPc, 0);
#endif /* #ifdef HOST */

	    
	    if (gc->program == newPc)
		{
		gc->status = WDB_ERR_GOPHER_SYNTAX;
		return ERROR;
		}
		
	    if (gc->execute)
		{
		gc->p = newP;
		}
	    
	    gc->program = newPc;
	    }
	else if (*gc->program == '+'	/* signed constants increment p */
		 || *gc->program == '-')
	    {
	    char *newPc;
	    int minus = *gc->program == '-';
	    int delta = strtol (++gc->program, &newPc, 0);

	    if (gc->program == newPc)
		{
		gc->status = WDB_ERR_GOPHER_SYNTAX;
		return ERROR;
		}
		
	    if (gc->execute)
		{
		gc->p += minus ? -delta : delta;
		}

	    gc->program = newPc;
	    }
	else if (*gc->program == '*')	/* * replaces p with *p */
	    {
	    if (gc->execute)
		{
		UINT32 newP;

#ifdef HOST
                if (Backend_T::memProbe_s ((TGT_ADDR_T) gc->p, VX_READ,
                                        sizeof (UINT32), (char *) &newP) != OK)
#else
		if ((*pWdbRtIf->memProbe) ((INT8 *) gc->p, VX_READ,
					sizeof (UINT32), (char *) &newP) != OK)
#endif /* #ifdef HOST */

		    {	
		    gc->status = WDB_ERR_GOPHER_FAULT;
		    return ERROR;
		    }
		gc->p = newP;
		}

	    ++gc->program;
	    }
	else if (*gc->program == '!')	/* ! writes p on the tape */
	    {
	    if (gc->execute)
		{
#ifdef HOST
                if (gopherWriteP (gc) != OK)
                    return ERROR;
#else
		if (gopherWriteScalar (gc, (UINT8 *) &gc->p, GOPHER_UINT32)
		    != OK)
		    return ERROR;
#endif /* #ifdef HOST */
		}

	    ++gc->program;
	    }
	else if (*gc->program == '@')	/* @ writes *p on the tape, advance p */
	    {
	    int		size = 4;
	    int		type = GOPHER_UINT32;

	    /* If the next character is in [bwlfdx] then modify
	     * the size of the transfer. Otherwise pick up 32 bits.
	     */
	    
	    switch (gc->program [1])
		{
		case 'b':
		    size = 1; 
		    type = GOPHER_UINT8;
		    ++gc->program;
		    break;
		case 'w':
		    size = 2; 
		    type = GOPHER_UINT16;
		    ++gc->program;
		    break;
		case 'f':
		    size = 4; 
		    type = GOPHER_FLOAT32;
		    ++gc->program;
		    break;
		case 'd':
		    size = 8; 
		    type = GOPHER_FLOAT64;
		    ++gc->program;
		    break;
		case 'x':
		    size = 10; 
		    type = GOPHER_FLOAT80;
		    ++gc->program;
		    break;
		case 'l':
		    size = 4; 
		    type = GOPHER_UINT32;
		    ++gc->program;
		    break;
		}
		    
	    if (gc->execute)
		{
		if (gopherWriteScalar (gc, (UINT8 *) gc->p, type) != OK)
		    return ERROR;

		gc->p += size;
		}

	    ++gc->program;
	    }
	else if (*gc->program == '$')	/* $ writes string at p on tape */
	    {
	    if (gc->execute)
		{
		if (gopherWriteString (gc, (char *) gc->p) != OK)
		    return ERROR;
		}

	    ++gc->program;
	    }
	else if (*gc->program == '{')	/* { saves p and repeats contents */
	    {				/* while p is not NULL.           */
	    wdbGopherCtx_t subGc;
	    char *progStart = gc->program+1;
	    UINT32 oldP = gc->p;
	    int oldX = gc->execute;
	    int result = OK;

	    subGc = *gc;		/* set up initial context */
	    ++subGc.program;		/* skip the initial left-brace */

	    /* We set the execute flag to zero if the pointer value 
	       is currently zero in our context.  This is because if
	       p == 0 upon encountering an open-brace, we should not execute 
	       the contents of the brace construct, but we still need
	       to know where to resume execution, so we must parse
	       in a subcontext. */

	    if (gc->p == 0)
		{
		subGc.execute = 0;
		result = gopher (&subGc);
		}
	    else
		{
		while (result == OK && subGc.p != 0)
		    {
		    subGc.program = progStart;
		    result = gopher (&subGc);
		    }
		}


	    /* Now set our program pointer to the character after the 
	       execution of the subprogram. */

	    *gc = subGc;

	    if (result != OK)
		{	
		return result;
		}
	    
	    /* restore p, execute. */

	    gc->p = oldP;
	    gc->execute = oldX;
	    }
	else if (*gc->program == '}')	/* } ends the loop opened by {. */
	    {
	    ++gc->program;
	    return OK;
	    }
	else if (*gc->program == '<')	/* < saves p and executes body once. */
	    {
	    wdbGopherCtx_t subGc;
	    UINT32 oldP = gc->p;
	    int result;

	    subGc = *gc;		/* set up initial context */
	    ++subGc.program;		/* skip the initial left-bracket */

	    result = gopher (&subGc);

	    if (result != OK)  
		{
		*gc = subGc;
		return result;
		}

	    /* Now set our program pointer to the character after the 
	       execution of the subprogram. */

	    *gc = subGc;
	    gc->p = oldP;
	    }
	else if (*gc->program == '>')	/* > closes the block opened by <. */
	    {
	    ++gc->program;
	    return OK;
	    }
	else if (*gc->program == '(')	/* perform "n" times, where n follows */
	    {
	    char *newPc;
	    UINT32 oldP = gc->p;
	    wdbGopherCtx_t subGc;

#ifdef HOST
            /* XXX - fix for GNU's bug in strtoul (). */
            unsigned int count = (unsigned int) strtol (gc->program + 1,
                                                        &newPc, 0);
#else
	    unsigned int count = (unsigned int) strtoul (gc->program + 1,
							&newPc, 0);
#endif /* #ifdef HOST */

	    int ix;
	    
	    if (gc->program+1 == newPc || count <= 0)
		{
		gc->status = WDB_ERR_GOPHER_SYNTAX;
		return ERROR;
		}

	    /* if we're not executing, just execute the loop once, so we 
	       can find the end of the nesting. */

	    if (! gc->execute) count = 1;

	    gc->program = newPc;
	    subGc = *gc;

	    for (ix = 0; ix < count; ++ix)
		{
		/* start the program at the beginning: after the (# part. */

		subGc.program = newPc;

		if (gopher (&subGc) != OK)
		    {
		    *gc = subGc;
		    return ERROR;
		    }
		}
	    
	    *gc = subGc;
	    gc->p = oldP;
	    }
	else if (*gc->program == ')')
	    {
	    ++gc->program;
	    return OK;
	    }
	else if (*gc->program == '_')
	    {
	    /* _ is ignored.  It can be used to separate two numbers
	       in a generated gopher program. */

	    ++gc->program;
	    }
	else if (*gc->program)
	    {
	    /* unknown character: syntax error. */

	    gc->status = WDB_ERR_GOPHER_SYNTAX;
	    return ERROR;
	    }
	}
    
    return OK;
    }

#ifdef HOST

/******************************************************************************
*
* wdbEvaluateGopher -
*/

UINT32 wdbEvaluateGopher
    (
    WDB_STRING_T *      pProgram,
    WDB_MEM_XFER *      pValueTape
    )
    {
    wdbGopherCtx_t      gc;

    gc.program  = *pProgram;
    gc.p        = 0;
    gc.pTape    = tape;
    gc.tapeIx   = 0;
    gc.tapeLen  = tapeLen;
    gc.execute  = 1;
    gc.status   = OK;

    gopher (&gc);

    pValueTape->source          = gc.pTape;
    pValueTape->numBytes        = gc.tapeIx;

    return (gc.status);
    }

/******************************************************************************
*
* gopherWriteScalar -
*/

static STATUS gopherWriteScalar
    (
    wdbGopherCtx_t *    gc,                     /* gopher context */
    UINT8 *             src,                    /* source address */
    int                 type                    /* GOPHER_UINT32, etc. */
    )
    {
    int                 nbytes = 4;             /* UINT32 is most common */
    int			status;

    if (type == GOPHER_UINT16)   nbytes = 2;    /* fix nbytes if size != 4 */
    if (type == GOPHER_UINT8)    nbytes = 1;
    if (type == GOPHER_FLOAT64)  nbytes = 8;
    if (type == GOPHER_FLOAT80)  nbytes = 10;

    /* We must have at least nbytes+1 bytes left: one for type marker,
       nbytes for the scalar itself. */

    if (gc->tapeLen - gc->tapeIx < nbytes+1)
        {
        gc->status = WDB_ERR_GOPHER_TRUNCATED;
        return ERROR;
        }

    /* Write the scalar type. */

    gc->pTape [gc->tapeIx++] = type;

    status = Backend_T::tgtRead_s ((TGT_ADDR_T) src,
				   (void *) (gc->pTape + gc->tapeIx),
				   nbytes);
    if (status != OK)
	{
	gc->status = WDB_ERR_MEM_ACCES;
	return ERROR;
	}
    
    gc->tapeIx += nbytes;
    return OK;
    }

/******************************************************************************
*
* gopherWriteString -
*
*/

static STATUS gopherWriteString
    (
    wdbGopherCtx_t *    gc,
    char *                              string
    )
    {
    int                 maxlen;                 /* how much we can carry */

    if (string == 0)
        {
        if (gc->tapeLen - gc->tapeIx < 2)
            {
            gc->status = WDB_ERR_GOPHER_TRUNCATED;
            return ERROR;
            }
        gc->pTape [gc->tapeIx++] = GOPHER_STRING;
        gc->pTape [gc->tapeIx++] = EOS;
        return OK;
        }

    /* we can write no more than the size of the remaining buffer minus 1
       (we have to save a byte for the null terminator). */

    maxlen = gc->tapeLen - gc->tapeIx;

        {
        int     status;
        char *  pEnd;
        int     nRead;
        int     numToRead = min (maxlen, MaxTgtStrLen);

        char *  pReadBuf =  new char [maxlen];

        /* Get a chunk of memory which is of size numToRead */
        status = Backend_T::tgtRead_s ((TGT_ADDR_T) string,
                                       pReadBuf, numToRead);
        if (status != OK)
            {
            gc->status = WDB_ERR_MEM_ACCES;
            return ERROR;
            }

        /* Null terminate the chunk */
        pReadBuf [numToRead - 1] = EOS;

        /* Find the real end of the string */
        pEnd = strchr (pReadBuf, EOS);

        if (pEnd == NULL)
            {
            memcpy (gc->pTape + gc->tapeIx, pReadBuf, numToRead - 1);
            gc->tapeIx += numToRead - 1;
            gc->status = WDB_ERR_GOPHER_TRUNCATED;
            return ERROR;
            }

        nRead = (UINT32) pEnd - (UINT32) pReadBuf;
        nRead++;        /* account for EOS character ('\0') */
        memcpy (gc->pTape + gc->tapeIx, pReadBuf, nRead);
        gc->tapeIx += nRead;

        delete [] pReadBuf;

        return OK;
        }
    }

/******************************************************************************
*
* gopherWriteP - write the pointer to the tape
*
* NOMANUAL
*/

STATUS gopherWriteP
    (
    wdbGopherCtx_t *    gc              /* gopher context */
    )
    {
    int                 nbytes = 4;             /* p is a UINT32 */

    /* We must have at least nbytes+1 bytes left: one for type marker,
       nbytes for the scalar itself. */

    if (gc->tapeLen - gc->tapeIx < nbytes+1)
        {
        gc->status = WDB_ERR_GOPHER_TRUNCATED;
        return ERROR;
        }

    /* Write the scalar type. */

    gc->pTape [gc->tapeIx++] = GOPHER_UINT32;

    /* Write the scalar itself. */

    memcpy ((gc->pTape + gc->tapeIx), (char *) &gc->p, nbytes);
    gc->tapeIx += nbytes;

    return OK;
    }

#else

/******************************************************************************
*
* wdbGopherLibInit -
*/

void wdbGopherLibInit (void)
    {
    pAddTape [0] = tape;
    wdbSvcAdd (WDB_EVALUATE_GOPHER, wdbEvaluateGopher, xdr_WDB_STRING_T,
		xdr_WDB_MEM_XFER);
    }

/******************************************************************************
*
* wdbEvaluateGopher -
*/

static UINT32 wdbEvaluateGopher
    (
    WDB_STRING_T * 	pProgram,
    WDB_MEM_XFER * 	pValueTape
    )
    {
    wdbGopherCtx_t	gc;
    int			ix;

    gc.status = OK;
    pAddTape [0] = tape;
    if (*pProgram != NULL)
	{
	/* starting a new evaluation */

	tapeIndex = 0;
	gc.program	= *pProgram;
	gc.p		= 0;
	gc.tapeLen	= tapeLen;	
	gc.execute	= 1;

	if (wdbIsNowTasking() && (pWdbRtIf->taskLock != NULL))
	    {
	    /* we check that no info is stored */

	    for (ix = 0; (ix < ADD_TAPE_NB); ix++)
		{
		if ((pAddTape[ix]) && ix && (pWdbRtIf->free != NULL))
		    {
		    (*pWdbRtIf->free) (pAddTape[ix]);
		    pAddTape [ix] = NULL;
		    }
		pAddTapeIx[ix] = 0;
		}

	    gc.pTape = pAddTape[0];
	    gc.tapeIx = 0;

	    if (wdbGopherLock)
		{
		(*pWdbRtIf->taskLock)();
		gopher (&gc); 
		(*pWdbRtIf->taskUnlock)();
		}
	    else
		gopher (&gc);
	    pAddTapeIx[tapeIndex] = gc.tapeIx;
	    }
	else
	    {
	    /*
	     * system mode : malloc is forbidden we can only store result
	     * in pAddTape[0]
	     */

	    gc.pTape = pAddTape[0];
	    gc.tapeIx = 0;

	    gopher (&gc); 
	    pAddTapeIx [0] = gc.tapeIx;
	    }
	
	/* prepare transfer by resetting tapeIndex */

	tapeIndex = 0;
	}

    if (gc.status == OK)
	gc.status = wdbTransferGopher (pValueTape);
    else
	{
	/* delete additional tapes if some were allocated */

	for (ix = 0; (ix < ADD_TAPE_NB) && (pWdbRtIf->free != NULL); ix++)
	    if ((pAddTape[ix]) && ix)
		{
		(*pWdbRtIf->free) (pAddTape[ix]);
		pAddTape[ix] = NULL;
		}
	}

    return (gc.status);
    }

/******************************************************************************
*
* wdbTransferGopher - answer to a gopher request using stored results
*
* This routine also frees tapes when they become out of date (ie when we are
* transfering the next one).
*
*/

static STATUS wdbTransferGopher
    (
    WDB_MEM_XFER * 	pValueTape
    )
    {
    int		offset = 0;
    int		tIndex = 0;
    int		ix;

    /* 
     * from the index of the request we computed the start address of the
     * result string
     */

#ifdef DEBUG
    printErr ("tapeIndex = %d\n", tapeIndex);
#endif
    for (ix = 0; (ix < tapeIndex); ix++)
	{
	offset += min (wdbCommMtu - WDB_GOPHER_MTU_OFFSET, tapeLen);
	if (offset >= pAddTapeIx[tIndex])
	    {
	    offset = 0;
	    tIndex++;
	    }
	}
    tapeIndex ++;

    /* free the last used tape if necessary */

    if ((tIndex > 1) && (pAddTape[tIndex - 1]) && (pWdbRtIf->free != NULL))
	{
	(*pWdbRtIf->free) (pAddTape[tIndex - 1]);
	pAddTape[tIndex - 1] = NULL;
	}

    /* check if every thing is transfered or if we should send another tape */

    if ((tIndex == ADD_TAPE_NB) || (pAddTapeIx[tIndex] == 0))
	{
	pValueTape->numBytes = 0;
	return OK;
	}

    pValueTape->source = (char *)((int)pAddTape[tIndex] + offset);
    pValueTape->numBytes = min (pAddTapeIx[tIndex] - offset,
				wdbCommMtu - WDB_GOPHER_MTU_OFFSET);

#ifdef DEBUG
    printErr ("pAddTapeIx[%d]= %d, offset = %d, numbytes = %d\n", tIndex,
	      pAddTapeIx[tIndex], offset, pValueTape->numBytes);
#endif

    /*
     * what about status to return
     * the only case to return OK is no tapes were allocated and the current
     * tape is transfered
     */

    if (tIndex == 0)
	{
	if ((pValueTape->numBytes + offset >= pAddTapeIx[tIndex]) &&
	    (pAddTape[tIndex + 1] == NULL))
	
	    /* tape 0 is finished and there is no other one */
	    return OK;
	}
    
    return (OK | WDB_TO_BE_CONTINUED);
    }

/*****************************************************************************
*
* gopherNewTapeAllocate - when possible allocate a buffer to store a longer
*			  gopher string
*
*/
static STATUS gopherNewTapeAllocate
    (
    wdbGopherCtx_t *    gc
    )
    {
    pAddTapeIx[tapeIndex] = gc->tapeIx;

    if (wdbIsNowTasking ())
	{
	/* try to allocate a new tape if possible */

	if ((pWdbRtIf->malloc == NULL) ||
	    (++tapeIndex >= ADD_TAPE_NB) ||
	    ((pAddTape[tapeIndex] = (*pWdbRtIf->malloc) (gc->tapeLen))
	     == NULL))
	    {
	    gc->status = WDB_ERR_GOPHER_TRUNCATED;
	    return ERROR;
	    }
	gc->pTape = pAddTape[tapeIndex];
	gc->tapeIx = 0;
	}
    else
	{
	/* system mode: we can't allocate more than the static buffer */

	gc->status = WDB_ERR_GOPHER_TRUNCATED;
	return ERROR;
	}
    return OK;
    }

/*****************************************************************************
* 
* gopherWriteScalar - write a scalar value on a gopher tape
*
*/

static STATUS gopherWriteScalar
    (
    wdbGopherCtx_t *	gc,			/* gopher context */
    UINT8 *		src,			/* source address */
    int			type			/* GOPHER_UINT32, etc. */
    )

    {
    int 		nbytes = 4;		/* UINT32 is most common */
    int			r1;			/* memProbe result */
    int			r2;			/* memProbe result */
    INT8		dummy [4];		/* dummy memProbe buf */

    if (type == GOPHER_UINT16)   nbytes = 2;	/* fix nbytes if size != 4 */
    if (type == GOPHER_UINT8)    nbytes = 1;
    if (type == GOPHER_FLOAT64)  nbytes = 8;
    if (type == GOPHER_FLOAT80)  nbytes = 10;

    /* We must have at least nbytes+1 bytes left: one for type marker,
       nbytes for the scalar itself. */

    if (gc->tapeLen - gc->tapeIx < nbytes+1)
	{
	if (gopherNewTapeAllocate (gc) == ERROR)
	    return ERROR;
	}

    /* Write the scalar type. */

    gc->pTape [gc->tapeIx++] = type;

    /* Write the scalar itself--probe the two endpoints. */

    if (nbytes <= 4)
	{
	r1 = r2 = (*pWdbRtIf->memProbe) ((char *) src, VX_READ, nbytes, dummy); 
	}
    else
	{
	r1 = (*pWdbRtIf->memProbe) ((char *) src, VX_READ, 1, dummy); 
	r2 = (*pWdbRtIf->memProbe) ((char *) src + nbytes - 1, VX_READ, 1,
				    dummy);
	}

    if (r1 == OK && r2 == OK)
	{
	bcopy ((char *) src, gc->pTape + gc->tapeIx, nbytes);

	gc->tapeIx += nbytes;
	return OK;
	}
    else
	{
	gc->status = WDB_ERR_GOPHER_FAULT;
	return ERROR;
	}
    }

/*****************************************************************************
*
* gopherWriteString - write a string to a gopher tape
*
*/

static STATUS gopherWriteString
    (
    wdbGopherCtx_t *	gc,
    char *		string
    )

    {
    int			r1;			/* memProbe result */
    int			r2;			/* memProbe result */
    int			r3 = 0;			/* memProbe result */
    char		dummy;			/* memProbe read value */
    int			maxlen;			/* how much we can carry */
    char *		p = string;
    int 		ix;

    if (string == 0)
	{
	if (gc->tapeLen - gc->tapeIx < 1)
	    {
	    if (gopherNewTapeAllocate (gc) == ERROR)
		return ERROR;
	    }
	gc->pTape [gc->tapeIx++] = GOPHER_STRING;

	if (gc->tapeLen - gc->tapeIx < 1)
	    {
	    if (gopherNewTapeAllocate (gc) == ERROR)
		return ERROR;
	    }
	gc->pTape [gc->tapeIx++] = EOS;
	return OK;
	}

    /* if possible, write the gopher type on the tape */

    if (gc->tapeLen - gc->tapeIx < 1)
	{
	if (gopherNewTapeAllocate (gc) == ERROR)
	    return ERROR;
	}
    gc->pTape [gc->tapeIx++] = GOPHER_STRING;
	
    /* write the string itself */

    while (1)
	{
	/* We can write no more than the size of the remaining buffer */

	maxlen = gc->tapeLen - gc->tapeIx;

	/* probe the two endpoints.  Note there's a weakness here; it may
	   be that a short string lies near the boundary of a memory
	   accessibility region and we're about to probe past the end of
	   it, which may give a false result.  But we can't call strlen on
	   the string without some hope that it is bounded by real
	   memory. */

	r1 = (*pWdbRtIf->memProbe) (p, VX_READ, 1, &dummy);
	r2 = (*pWdbRtIf->memProbe) (p + maxlen, VX_READ, 1, &dummy);

	if (r1 == OK && r2 != OK)
	    {
	    /* If the start endpoint probe was okay, but not the end one then
	     * we assume we have tried to read outside the memory boundary.
	     * Given that the boundary is probably at the very least 2K byte
	     * aligned (more likely 64k or more), we modify the top 
	     * address to account for this and repeat the probe just for good 
	     * measure. 
	     */
	    maxlen = ((((long) (p + maxlen)) & ~0x7ff) - 1) - (long) p;
	    r3 = (*pWdbRtIf->memProbe) (p + maxlen, VX_READ, 1, &dummy);
	    }

	if (r1 != OK || r3 != OK)
	    break;	/* exit loop: there is an error */

	for (ix = 0;
	     *p && ix < maxlen;
	     p++, ix++)
	    {
	    /*
	     * Do not need to test if there is place for each character.
	     * We can't write more than <maxlen> character (i.e. fill the 
	     * current tape ).
	     */
	    gc->pTape [gc->tapeIx++] = *p;
	    }

#if DEBUG
	printf ("maxlen=%d, r1=%d, r2=%d, r3=%d, ix=%d, *p=%#x\n",
		maxlen, r1, r2, r3, ix, *p);
#endif
	if (*p == EOS)
	    {
	    /* EOS found : write null terminator and return OK */

	    if (gc->tapeLen - gc->tapeIx < 1)
		{
		if (gopherNewTapeAllocate (gc) == ERROR)
		    return ERROR;
		}
	    gc->pTape [gc->tapeIx++] = *p;
	    return OK;
	    }
	else if (r2 != OK)
	    {
	    /* we reached maxlen (hardware limitation) : return ERROR */

	    gc->status = WDB_ERR_GOPHER_TRUNCATED;
	    return (ERROR);
	    }
	else
	    {
	    /*
	     * we finished the probed zone so re-probe before continuing
	     * we allocate a new tape (if needed) to update maxlen computation
	     */

	    if (gc->tapeLen - gc->tapeIx < 1)
		{
		if (gopherNewTapeAllocate (gc) == ERROR)
		    return (ERROR);
		}
	    }
	}

    /* one of the endpoints was bogus.  Can't write the string. */

    gc->status = WDB_ERR_GOPHER_FAULT;
    return ERROR;
    }

#endif /* #ifdef HOST */

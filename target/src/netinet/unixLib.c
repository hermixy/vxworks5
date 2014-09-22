/* unixLib.c - UNIX kernel compatability library */

/* Copyright 1984 - 2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02o,15oct01,rae  merge from truestack ver 02r, base 02n (SPR 32626 etc.)
02n,25nov97,vin  changed mBufClGet() to netTupleGet().
02m,14nov97,vin  added panic hook.
02l,08mar97,vin  added hashinit() for pcb hash look ups (FREEBSD 2.2.1).
02k,03dec96,vin  added _netMalloc(..) & _netFree(..) which use network buffers
02j,19mar95,dvs  removed tron references.
02i,15nov94,tmk  added 29k fix from philm
02h,07nov94,tmk  added MC68LC040 support.
02g,31oct94,kdl  merge cleanup.
02f,02aug94,tpr  added MC68060 cpu support.
02e,09jun93,hdn  added a support for I80X86
02d,29jul92,smb  moved perror() to fioLib.c.
02c,17jun92,jwt  removed SPARC from !PORTABLE list - it no longer works.
02b,16jun92,jwt  added SPARC to !PORTABLE list; fixed 02a version number.
02a,26may92,rrr  the tree shuffle
		  -changed includes to have absolute path from h/
01z,17oct91,yao  added support for CPU32.
01y,04oct91,rrr  passed through the ansification filter
		  -changed includes to have absolute path from h/
		  -fixed #else and #endif
		  -changed VOID to void
		  -changed copyright notice
01x,26aug91,rrr  clean up function decls
01w,23jul91,hdn  added conditional macro for optimized TRON codes.
01v,26apr91,hdn  modified to use portable version of checksum routine.
01u,26jun90,jcf  added semSplSemInit ().  Changed splSem to mutex.
01t,20mar90,jcf  added timeout of WAIT_FOREVER to semTake calls.
01s,18mar90,hjb  ifdef's to use optimized assembly checksum routine for 68k.
01r,20aug89,gae  #if'd out un-portable in_cksum.
01q,16apr89,gae  updated to 4.3BSD.
01p,15oct88,dnw  changed name of sleep() to ksleep() to avoid confusion
		   with C library routine of same name.
		 changed 'panic' to not suspend unless panicSuspend=TRUE.
01o,22jun88,dnw  removed ovbcopy(), copyin(), copyout(), and imin() which
		   are now macros in systm.h.
		 made spl...() use taskIdCurrent directly.
		 made spl...() return 0/1 for "previous level" instead of
		   task id.
01n,29may88,dnw  removed bcmp() and bzero() now in bLib.
		 changed to v4 names.
01m,04may88,jcf  changed splSem to splSemId, and sem calls for new semLib.
01l,28jan88,jcf  made kernel independent.
01k,23jan88,rdc  brought back spl routines because of size issues.
01j,04jan88,rdc  made spl{net,imp,x} macro's in systm.h
01i,17nov87,dnw  removed printStatus from perror.
01h,13nov87,rdc  added perror;
01g,22oct87,ecs  delinted.
01f,02may87,dnw  added suspend to panic().
01e,03apr87,ecs  added copyright.
01d,02apr87,jlf  delinted.
01c,27feb87,dnw  change myTaskId() to vxMyTaskId().
01b,22dec86,dnw  added bcmp.
01a,28jul86,rdc  written.
*/

/*
DESCRIPTION
This library provides routines that simulate or replace Unix kernel functions
that are used in the network code.  This includes the spl...() processor
level locking routines, wakeup() and sleep() (renamed ksleep() to avoid
confusion with the C library routine called sleep()), perror() and panic(),
and a checksum routine.
*/

#include "vxWorks.h"
#include "semLib.h"
#include "sys/types.h"
#include "net/mbuf.h"
#include "taskLib.h"
#include "netinet/in.h"
#include "netinet/in_systm.h"
#include "lstLib.h"
#include "net/systm.h"
#include "logLib.h"
#include "intLib.h"

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET
#include "wvNetLib.h"
#endif
#endif

/* local variables */

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET
    /* Set common fields of event identifiers for this module. */

LOCAL UCHAR wvNetModuleId = WV_NET_UNIXLIB_MODULE; /* Value for unixLib.c */
LOCAL UCHAR wvNetLocalFilter = WV_NET_NONE;     /* Available event filter */

LOCAL ULONG wvNetEventId;       /* Event identifier: see wvNetLib.h */
#endif    /* INCLUDE_WVNET */
#endif

/* global variables */

BOOL   panicSuspend;		/* TRUE = suspend task causing panic */

#ifndef VIRTUAL_STACK
int    splTid;			/* owner of network processor level semaphore */
SEM_ID splSemId;		/* spl mutex semaphore id */
#else
#include "netinet/vsLib.h"
#endif /* VIRTUAL_STACK */

int mutexOptionsUnixLib = SEM_Q_PRIORITY | SEM_DELETE_SAFE | SEM_INVERSION_SAFE;

FUNCPTR	_panicHook = NULL;	/* panicHook for error recovery mechanism */

/*******************************************************************************
*
* splSemInit - initialize the semaphore used by splnet, splimp, etc.
*
* This routine is called by netLibInit () to initialize the spl semaphore.
*
* NOMANUAL
*/

void splSemInit ()
    {
    splSemId = semMCreate (mutexOptionsUnixLib);
    }

/******************************************************************************
*
* splnet - set network processor level
*
* splnet may be used to insure mutual exclusion among processes which
* share common network data structures.  In UNIXland, this is done by
* setting the processor interrupt level.  Here in VxWorksLand, however,
* we use a simple semaphore mechanism.  splx should be called after
* completion of the critical section.  We must check first to see if we're
* already at network level.
*
* RETURNS: previous "processor level"
*/

int splnet ()

    {
    if ((ULONG) taskIdCurrent == splTid)
	return (1);

    semTake (splSemId, WAIT_FOREVER);
    splTid = (ULONG) taskIdCurrent;
    return (0);
    }

/******************************************************************************
*
* splnet2 - set network processor level _immediately_
*
* WARNING:  Calling this routine with a timeout that is not WAIT_FOREVR 
*           is a violation of mutual exclusion and will cause problems in
*           a running system.
*           The caller must have very specific knowledge of the state of
*           the networking system.
*
* RETURNS: previous "processor level"
*/

int splnet2 (timeout)
    int timeout;		/* Number of ticks to wait spl semaphore */
    {
    if ((ULONG) taskIdCurrent == splTid)
        return (1);

    semTake (splSemId, timeout);
    splTid = (ULONG) taskIdCurrent;
    return (0);
    }

/*******************************************************************************
*
* splimp - set imp processor level
*
* This routine is indentical to splnet().
* Currently, all network interlocking uses the same network semaphore.
*
* RETURNS: previous "processor level"
*/

int splimp ()

    {
    if ((ULONG) taskIdCurrent == splTid)
	return (1);

    semTake (splSemId, WAIT_FOREVER);
    splTid = (ULONG) taskIdCurrent;
    return (0);
    }
/*******************************************************************************
*
* splx - set processor level
*
* splx is used in UNIX to restore the processor level.
* It is used in VxWorks in conjunction with splnet() and splimp() to provide
* mutual exclusion of network data structures.  splx marks the end of the
* critical section.
*/

void splx (x)
    int x;	/* processor level to restore */

    {
    if (x == 0)
	{
	splTid = 0;
	semGive (splSemId);
	}
    }
/*******************************************************************************
*
* ksleep -  put a process to sleep
*
* In Unix, processes can sleep on any value (typically an address).
* In VxWorks, we substitute a semaphore mechanism so sleep()
* calls have to be changed to provide the address of a semaphore.
*
* Note: In Unix there are two "sleep" routines.  In the kernel, the sleep
* routine causes a process to block until a wakeup is done on the same
* value that sleep was called with.  This routine, and the wakeup routine
* below, are replacements for that mechanism.  However, in the C library
* there is another sleep() routine that will suspend a process for a
* specified number of seconds.  Since VxWorks users porting an application
* may have calls to the latter but not the former, there is potential for
* vast confusion.  Thus this routine has been renamed "ksleep" ("kernel sleep")
* and all calls to it in network code have been changed.
*/

void ksleep (semId)
    SEM_ID semId;

    {
    BOOL hadSplSem;

    /* first see if we've got the network semaphore (splSemId);
     * if so, give it, and then take it back when we are awakened.
     */

    hadSplSem = (splTid == (ULONG) taskIdCurrent);
    if (hadSplSem)
	{
	splTid = 0;
	semGive (splSemId);
	}

    semTake (semId, WAIT_FOREVER);

    if (hadSplSem)
	{
	semTake (splSemId, WAIT_FOREVER);
	splTid = (ULONG) taskIdCurrent;
	}
    }
/*******************************************************************************
*
* wakeup - wakeup a sleeping process
*/

void wakeup (semId)
    SEM_ID semId;
    {
    semGive (semId);
    }

#ifdef unixLib_PORTABLE

/*******************************************************************************
*
* _insque - insert node in list after specified node.
*
* Portable version of _insque ().
*/

void _insque (pNode, pPrev)
    NODE *pNode;
    NODE *pPrev;
    {
    NODE *pNext;

    pNext = pPrev->next;
    pPrev->next = pNode;
    pNext->previous = pNode;
    pNode->next = pNext;
    pNode->previous = pPrev;
    }

/*******************************************************************************
*
* _remque - remove specified node in list.
*
* Portable version of _remque ().
*/

void _remque (pNode)
    NODE *pNode;
    {
    pNode->previous->next = pNode->next;
    pNode->next->previous = pNode->previous;
    }

#endif	/* unixLib_PORTABLE */

/*******************************************************************************
*
* panic - something is very wrong
*
* panic simply logs a message to the console prepended with "panic".
* If the panic occurs at task level, the current task is suspended.
*/

void panic (msg)
    char *msg;

    {
    /* execute the panic hook if one exists */
    if (_panicHook != NULL)
        {
        (*_panicHook) (msg); 
        return; 
        }
        
    logMsg ("panic: %s\n", (int)msg, 0,0,0,0,0);

    if (!intContext () && panicSuspend)
	taskSuspend (0);
    }

/*******************************************************************************
*
* _netMalloc - allocates network buffers
*
* This function allocates the relevant cluster and returns the 
* the pointer to the memory allocated.
* 
* INTERNAL
* The first 4 bytes of the cluster allocated hold a back pointer to the mBlk
* The actual cluster size allocated is the user requested size + size of 
* mBlk pointer to pointer.
*
* NOMANUAL
*
* RETURNS: char pointer /NULL 
*/

char * _netMalloc
    (
    int			bufSize,		/* size of the buffer to get */
    UCHAR		type,			/* type of mbuf to allocate */
    int 		canWait			/* M_WAIT/M_DONTWAIT */
    )
    { 
    FAST struct mbuf * 	pMblk; 
    FAST struct mbuf ** pPtrMblk; 
    pMblk = netTupleGet (_pNetSysPool, (bufSize + sizeof(struct mbuf *)),
                         canWait, type, TRUE);
    if (pMblk != NULL) 
	{ 
	pPtrMblk = mtod(pMblk, struct mbuf **); 
	*pPtrMblk = pMblk; 
	pMblk->m_data += sizeof(struct mbuf **); 
	return (mtod(pMblk, char *));
	} 
    else 
	return (NULL); 
    }

/*******************************************************************************
*
* _netFree - frees network buffers
*
* This function allocates the relevant cluster and returns the 
* the pointer to the memory allocated.
* 
* INTERNAL
* The first 4 bytes of the cluster allocated hold a back pointer to the mBlk
* The actual cluster size allocated is the user requested size + 4.
* Since the mBlk will retain the type,m_free() will automatically free the 
* relevant type
*
* NOMANUAL
*
* RETURNS: pointer/NULL 
*/

void _netFree
    (
    char * pBuf		/* pointer to buffer to free */	 
    ) 
    {
    if (pBuf != NULL)
	(void)m_free (*((struct mbuf **)(pBuf - sizeof(struct mbuf **))));
    }

/*******************************************************************************
*
* hashinit - initialize a hash table of a given size.
*
* This function initializes the hashtable of a given number of elements.
* It returns a pointer to the hash mask. 
* 
* NOMANUAL
*
* RETURNS: pointer/NULL 
*/

void * hashinit
    (
    int 	elements,	/* number of elements in the hash table */
    int 	type,		/* type of entries PCB/IFADDR etc */
    u_long *	hashmask	/* ptr to the hash mask */
    )
    {
    long 	hashsize;
    int 	ix;
    LIST_HEAD(generic, generic) *hashtbl;

    if (elements <= 0)
        {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_EMERGENCY event */
        WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_EMERGENCY, 45, 1,
                         WV_NETEVENT_HASHINIT_PANIC, type)
#endif  /* INCLUDE_WVNET */
#endif

        panic("hashinit: bad elements");
        }
    
    for (hashsize = 1; hashsize <= elements; hashsize <<= 1)
        continue;
    
    hashsize >>= 1;
    hashtbl = malloc((u_long)hashsize * sizeof(*hashtbl));

    for (ix = 0; ix < hashsize; ix++)
        LIST_INIT(&hashtbl[ix]);

    *hashmask = hashsize - 1;

    return (hashtbl);
    }

/* sigLib.c - software signal facility library */

/* Copyright 1984-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01m,17jul00,jgn  merge DOT-4 pthreads code
01l,20apr00,hk   changed sysBErrVecNum to excBErrVecNum for SH.
01k,03mar00,zl   merged SH support into T2
01j,18aug98,tpr  added PowerPC EC 603 support.
02l,08may98,pr   added extra parameter in WV macro
02k,23jan98,pr   commented out some of the WindView code
02j,13dec97,pr   added ifdef WV_INSTRUMENTATION to the WindView code
02i,13nov96,dgp  doc: add PowerPC info to table
02h,11jun96,dbt  fixed spr 5789. Added brackets for errnoSet in function
		 sigtimedwait().
02g,25nov95,jdi  removed 29k stuff.
02f,02feb95,rhp  correct argument refs in signal() man page,
                 and misc doc cleanup
02e,16jan95,rhp  add pointer to POSIX 1003.1b fns in library man page
01w,19oct93,cd   added R4000 to documentation.
01w,03feb94,hdn  fixed spr 2996. for-loop in sigExcKill() ends if both are 0.
		 added a comment for I80X86 support.
01w,20mar94,pme  Added Am29030 and Am29200 exception signals definitions.
02d,06jul94,rrr  added save/restore of resumption record for the I960KB.
02c,08apr94,dvs  added doc on signal handler definition (SPR #3222).
02b,07apr94,smb  changed parameter to EVENT_SIGSUSPEND
02a,24mar94,smb  fixed event collection for sigsuspend
01z,02feb94,kdl  fixed condition around ffsMsb() prototype; deleted sigwait()
		 from structure diagram.
01y,28jan94,kdl  added declaration of sigEvtRtn (from sigLibP.h); added include
		 of sysLib.h; added prototype for ffsMsb()..
01x,24jan94,smb  added instrumentation macros.
		 changed description for 01w.
01w,10dec93,smb  added instrumentation and POSIX phase 1 additions.
01v,16sep93,rrr  fixed spr 2226,2227
01u,01mar93,jdi  doc: additions to section on signal handlers.
01t,09feb93,rrr  fixed spelling in doc, removed sigstack() and sigreturn()
		 from table correlating bsd and posix interface.
01s,03feb93,rrr  fixed src 1903,1904,1905 which are not reseting the handler
		 and corrected the use of sa_mask
01r,03feb93,jdi  documentation cleanup for 5.1; augmented arch-specific
		 exception tables.
01q,13nov92,dnw  added include of smObjLib.h
01p,15oct92,rrr  Fixed alignment of stack for 960 and sparc (spr #1628)
01o,29sep92,rrr  NOMANUAL a bunch of routines. The function signal produces
                 an error with mangen (but the output looks ok).
01n,31aug92,rrr  fixed the code passed from excLib to be the exception vector
01m,18aug92,rrr  bug fix for timers.
01l,31jul92,rrr  changed _sig_timeout_recalc to _func_sigTimeoutRecalc
01k,30jul92,rrr  undo of 01j (now back to 01i)
01j,30jul92,kdl  backed out 01i changes pending rest of exc handling.
01i,29jul92,rrr  added in hook for exceptions to call signals directly.
01h,28jul92,jcf  falsed out delete hook that was causing grief for shell scripts
01g,27jul92,rrr  a quick checkin to test timers, still more testing needed.
01f,23jul92,ajm  moved _sig_timeout_recalc to taskLib to shrink proms
01e,27jul92,rrr  added install of signals to excLib 
01d,19jul92,pme  changed sigWindRestart() to handle shared semaphore take.
		 added include stddef.h.
01c,18jul92,smb  Changed errno.h to errnoLib.h.
01b,10jul92,rrr  removed setjmp (now it is a macro in setjmp.h)
01a,31jan92,rrr  written.
*/

/*
DESCRIPTION
This library provides a signal interface for tasks.  Signals are used to
alter the flow control of tasks by communicating asynchronous events
within or between task contexts.  Any task or interrupt service can
"raise" (or send) a signal to a particular task.  The task being signaled
will immediately suspend its current thread of execution and invoke a
task-specified "signal handler" routine.  The signal handler is a
user-supplied routine that is bound to a specific signal and performs
whatever actions are necessary whenever the signal is received.  Signals
are most appropriate for error and exception handling, rather than as a
general purpose intertask communication mechanism.

This library has both a BSD 4.3 and POSIX signal interface.  The POSIX
interface provides a standardized interface which is more functional than
the traditional BSD 4.3 interface.  The chart below shows the correlation
between BSD 4.3 and POSIX 1003.1 functions.  An application should use only one
form of interface and not intermix them.

.TS
tab(|);
lf3 lf3
l   l.
.ne 4
\%BSD 4.3    | POSIX 1003.1
_
sigmask()    | sigemptyset(), sigfillset(), sigaddset(),
	     |    sigdelset(), sigismember()
sigblock()   | sigprocmask()
sigsetmask() | sigprocmask()
pause()      | sigsuspend()
sigvec()     | sigaction()
(none)       | sigpending()
signal()     | signal()
kill()       | kill()
.TE

POSIX 1003.1b (Real-Time Extensions) also specifies a queued-signal
facility that involves four additional routines: sigqueue(),
sigwaitinfo(), and sigtimedwait().

In many ways, signals are analogous to hardware interrupts.  The signal
facility provides a set of 31 distinct signals.  A signal can be raised by
calling kill(), which is analogous to an interrupt or hardware exception.
A signal handler is bound to a particular signal with sigaction() in much
the same way that an interrupt service routine is connected to an
interrupt vector with intConnect().  Signals are blocked for the duration
of the signal handler, just as interrupts are locked out for the duration
of the interrupt service routine.  Tasks can block the occurrence of
certain signals with sigprocmask(), just as the interrupt level can be
raised or lowered to block out levels of interrupts.  If a signal is
blocked when it is raised, its handler routine will be called when the
signal becomes unblocked.

Several routines (sigprocmask(), sigpending(), and sigsuspend()) take 
`sigset_t' data structures as parameters.  These data structures are 
used to specify signal set masks.  Several routines are provided for 
manipulating these data structures: sigemptyset() clears all the bits 
in a `segset_t',  sigfillset() sets all the bits in a `sigset_t', 
sigaddset() sets the bit in a `sigset_t' corresponding to a particular 
signal number, sigdelset() resets the bit in a `sigset_t' corresponding 
to a particular signal number, and sigismember() tests to see if the 
bit corresponding to a particular signal number is set.

FUNCTION RESTARTING
If a task is pended (for instance, by waiting for a semaphore to become
available) and a signal is sent to the task for which the task has a
handler installed, then the handler will run before the semaphore is
taken.  When the handler is done, the task will go back to being pended
(waiting for the semaphore).  If there was a timeout used for the pend,
then the original value will be used again when the task returns from the
signal handler and goes back to being pended.

Signal handlers are typically defined as:
.ne 7
.CS
    void sigHandler
        (
	int sig,			/@ signal number                 @/
        )
	{
            ...
        }
.CE
In VxWorks, the signal handler is passed additional arguments and can be
defined as:
.ne 9
.CS
    void sigHandler
        (
	int sig,			/@ signal number                 @/
        int code,			/@ additional code               @/
        struct sigcontext *pSigContext	/@ context of task before signal @/
        )
	{
            ...
        }
.CE
The parameter <code> is valid only for signals caused by hardware
exceptions.  In this case, it is used to distinguish signal variants.
For example, both numeric overflow and zero divide raise SIGFPE
(floating-point exception) but have different values for <code>.
(Note that when the above VxWorks extensions are used, the compiler
may issue warnings.)

SIGNAL HANDLER DEFINITION
Signal handling routines must follow one of two specific formats, so that they
may be correctly called by the operating system when a signal occurs.  

Traditional signal handlers receive the signal number as the sole input 
parameter.  However, certain signals generated by routines which make up the
POSIX Real-Time Extensions (P1003.1b) support the passing of an additional
application-specific value to the handler routine.  These include signals
generated by the sigqueue() call, by asynchronous I/O, by POSIX real-time
timers, and by POSIX message queues.

If a signal handler routine is to receive these additional parameters,
SA_SIGINFO must be set in the sa_flags field of the sigaction structure which
is a parameter to the sigaction() routine.  Such routines must take the
following form:

.CS
	void sigHandler (int sigNum, siginfo_t * pInfo, void * pContext);
.CE

Traditional signal handling routines must not set SA_SIGINFO in the sa_flags
field, and must take the form of:

.CS
        void sigHandler (int sigNum);
.CE

EXCEPTION PROCESSING:
Certain signals, defined below, are raised automatically when
hardware exceptions are encountered.  This mechanism allows user-defined
exception handlers to be installed.  This is useful for recovering from
catastrophic events such as bus or arithmetic errors.  Typically, setjmp()
is called to define the point in the program where control will be
restored, and longjmp() is called in the signal handler to restore that
context.  Note that longjmp() restores the state of the task's signal
mask.  If a user-defined handler is not installed or the installed handler
returns for a signal raised by a hardware exception, then the task is
suspended and a message is logged to the console.

The following is a list of hardware exceptions caught by VxWorks and delivered
to the offending task.  The user may include the higher-level header file
sigCodes.h in order to access the appropriate architecture-specific header file
containing the code value.
.SS "Motorola 68K"
.TS
tab(|);
lf3 lf3 lf3
l1p8 l0p8 l.
Signal  | Code               | Exception
_
SIGSEGV | NULL               | bus error
SIGBUS  | BUS_ADDERR         | address error
SIGILL  | ILL_ILLINSTR_FAULT | illegal instruction
SIGFPE  | FPE_INTDIV_TRAP    | zero divide
SIGFPE  | FPE_CHKINST_TRAP   | chk trap
SIGFPE  | FPE_TRAPV_TRAP     | trapv trap
SIGILL  | ILL_PRIVVIO_FAULT  | privilege violation
SIGTRAP | NULL               | trace exception
SIGEMT  | EMT_EMU1010        | line 1010 emulator
SIGEMT  | EMT_EMU1111        | line 1111 emulator
SIGILL  | ILL_ILLINSTR_FAULT | coprocessor protocol violation
SIGFMT  | NULL               | format error
SIGFPE  | FPE_FLTBSUN_TRAP   | compare unordered
SIGFPE  | FPE_FLTINEX_TRAP   | inexact result
SIGFPE  | FPE_FLTDIV_TRAP    | divide by zero
SIGFPE  | FPE_FLTUND_TRAP    | underflow
SIGFPE  | FPE_FLTOPERR_TRAP  | operand error
SIGFPE  | FPE_FLTOVF_TRAP    | overflow
SIGFPE  | FPE_FLTNAN_TRAP    | signaling "Not A Number"
.TE
.SS "SPARC"
.TS
tab(|);
lf3 lf3 lf3
l1p8 l0p8 l.
Signal | Code                | Exception
_
SIGBUS | BUS_INSTR_ACCESS    | bus error on instruction fetch
SIGBUS | BUS_ALIGN           | address error (bad alignment)
SIGBUS | BUS_DATA_ACCESS     | bus error on data access
SIGILL | ILL_ILLINSTR_FAULT  | illegal instruction
SIGILL | ILL_PRIVINSTR_FAULT | privilege violation
SIGILL | ILL_COPROC_DISABLED | coprocessor disabled
SIGILL | ILL_COPROC_EXCPTN   | coprocessor exception
SIGILL | ILL_TRAP_FAULT(n)   | uninitialized user trap
SIGFPE | FPE_FPA_ENABLE      | floating point disabled
SIGFPE | FPE_FPA_ERROR       | floating point exception
SIGFPE | FPE_INTDIV_TRAP     | zero divide
SIGEMT | EMT_TAG             | tag overflow
.TE
.SS "Intel i960"
.TS
tab(|);
lf3 lf3 lf3
l1p8 l0p8 l.
Signal  | Code                           | Exception
_
SIGBUS  | BUS_UNALIGNED                  | address error (bad alignment)
SIGBUS  | BUS_BUSERR                     | bus error
SIGILL  | ILL_INVALID_OPCODE             | invalid instruction
SIGILL  | ILL_UNIMPLEMENTED              | instr fetched from on-chip RAM
SIGILL  | ILL_INVALID_OPERAND            | invalid operand
SIGILL  | ILL_CONSTRAINT_RANGE           | constraint range failure
SIGILL  | ILL_PRIVILEGED                 | privilege violation
SIGILL  | ILL_LENGTH                     | bad index to sys procedure table
SIGILL  | ILL_TYPE_MISMATCH              | privilege violation
SIGTRAP | TRAP_INSTRUCTION_TRACE         | instruction trace fault
SIGTRAP | TRAP_BRANCH_TRACE              | branch trace fault
SIGTRAP | TRAP_CALL_TRACE                | call trace fault
SIGTRAP | TRAP_RETURN_TRACE              | return trace fault
SIGTRAP | TRAP_PRERETURN_TRACE           | pre-return trace fault
SIGTRAP | TRAP_SUPERVISOR_TRACE          | supervisor trace fault
SIGTRAP | TRAP_BREAKPOINT_TRACE          | breakpoint trace fault
SIGFPE  | FPE_INTEGER_OVERFLOW           | integer overflow
SIGFPE  | FST_ZERO_DIVIDE                | integer zero divide
SIGFPE  | FPE_FLOATING_OVERFLOW          | floating point overflow
SIGFPE  | FPE_FLOATING_UNDERFLOW         | floating point underflow
SIGFPE  | FPE_FLOATING_INVALID_OPERATION | invalid floating point operation
SIGFPE  | FPE_FLOATING_ZERO_DIVIDE       | floating point zero divide
SIGFPE  | FPE_FLOATING_INEXACT           | floating point inexact
SIGFPE  | FPE_FLOATING_RESERVED_ENCODING | floating point reserved encoding
.TE
.SS "MIPS R3000/R4000"
.TS
tab(|);
lf3 lf3 lf3
l1p8 l0p8 l.
Signal  | Code | Exception
_
SIGBUS  | BUS_TLBMOD          | TLB modified
SIGBUS  | BUS_TLBL            | TLB miss on a load instruction
SIGBUS  | BUS_TLBS            | TLB miss on a store instruction
SIGBUS  | BUS_ADEL            | address error (bad alignment) on load instr
SIGBUS  | BUS_ADES            | address error (bad alignment) on store instr
SIGSEGV | SEGV_IBUS           | bus error (instruction)
SIGSEGV | SEGV_DBUS           | bus error (data)
SIGTRAP | TRAP_SYSCALL        | syscall instruction executed
SIGTRAP | TRAP_BP             | break instruction executed
SIGILL  | ILL_ILLINSTR_FAULT  | reserved instruction
SIGILL  | ILL_COPROC_UNUSABLE | coprocessor unusable
SIGFPE  | FPE_FPA_UIO, SIGFPE | unimplemented FPA operation
SIGFPE  | FPE_FLTNAN_TRAP     | invalid FPA operation
SIGFPE  | FPE_FLTDIV_TRAP     | FPA divide by zero
SIGFPE  | FPE_FLTOVF_TRAP     | FPA overflow exception
SIGFPE  | FPE_FLTUND_TRAP     | FPA underflow exception
SIGFPE  | FPE_FLTINEX_TRAP    | FPA inexact operation
.TE
.SS "Intel i386/i486"
.TS
tab(|);
lf3 lf3 lf3
l1p8 l0p8 l.
Signal  | Code                 | Exception
_
SIGILL  | ILL_DIVIDE_ERROR     | divide error
SIGEMT  | EMT_DEBUG            | debugger call
SIGILL  | ILL_NON_MASKABLE     | NMI interrupt
SIGEMT  | EMT_BREAKPOINT       | breakpoint
SIGILL  | ILL_OVERFLOW         | INTO-detected overflow
SIGILL  | ILL_BOUND            | bound range exceeded
SIGILL  | ILL_INVALID_OPCODE   | invalid opcode
SIGFPE  | FPE_NO_DEVICE        | device not available
SIGILL  | ILL_DOUBLE_FAULT     | double fault
SIGFPE  | FPE_CP_OVERRUN       | coprocessor segment overrun
SIGILL  | ILL_INVALID_TSS      | invalid task state segment
SIGBUS  | BUS_NO_SEGMENT       | segment not present
SIGBUS  | BUS_STACK_FAULT      | stack exception
SIGILL  | ILL_PROTECTION_FAULT | general protection
SIGBUS  | BUS_PAGE_FAULT       | page fault
SIGILL  | ILL_RESERVED         | (intel reserved)
SIGFPE  | FPE_CP_ERROR         | coprocessor error
SIGBUS  | BUS_ALIGNMENT        | alignment check
.TE
.SS "PowerPC"
.TS
tab(|);
lf3 lf3 lf3
l1p8 l0p8 l.
Signal  | Code                 | Exception
_
SIGBUS  | _EXC_OFF_MACH        | machine check
SIGBUS  | _EXC_OFF_INST        | instruction access
SIGBUS  | _EXC_OFF_ALIGN       | alignment
SIGILL  | _EXC_OFF_PROG        | program
SIGBUS  | _EXC_OFF_DATA        | data access
SIGFPE  | _EXC_OFF_FPU         | floating point unavailable
SIGTRAP | _EXC_OFF_DBG         | debug exception (PPC403)
SIGTRAP | _EXC_OFF_INST_BRK    | inst. breakpoint (PPC603, PPCEC603, PPC604)
SIGTRAP | _EXC_OFF_TRACE       | trace (PPC603, PPCEC603, PPC604, PPC860)
SIGBUS  | _EXC_OFF_CRTL        | critical interrupt (PPC403)
SIGILL  | _EXC_OFF_SYSCALL     | system call
.TE
.SS "Hitachi SH770x"
.TS
tab(|);
lf3 lf3 lf3
l1p8 l0p8 l.
Signal  | Code                       | Exception
_
SIGSEGV | TLB_LOAD_MISS              | TLB miss/invalid (load)
SIGSEGV | TLB_STORE_MISS             | TLB miss/invalid (store)
SIGSEGV | TLB_INITITIAL_PAGE_WRITE   | Initial page write
SIGSEGV | TLB_LOAD_PROTEC_VIOLATION  | TLB protection violation (load)
SIGSEGV | TLB_STORE_PROTEC_VIOLATION | TLB protection violation (store)
SIGBUS  | BUS_LOAD_ADDRESS_ERROR     | Address error (load)
SIGBUS  | BUS_STORE_ADDRESS_ERROR    | Address error (store)
SIGILL  | ILLEGAL_INSTR_GENERAL      | general illegal instruction
SIGILL  | ILLEGAL_SLOT_INSTR         | slot illegal instruction
SIGFPE  | FPE_INTDIV_TRAP            | integer zero divide
.TE
.SS "Hitachi SH7604/SH704x/SH703x/SH702x"
.TS
tab(|);
lf3 lf3 lf3
l1p8 l0p8 l.
Signal | Code                 | Exception
_
SIGILL | ILL_ILLINSTR_GENERAL | general illegal instruction
SIGILL | ILL_ILLINSTR_SLOT    | slot illegal instruction
SIGBUS | BUS_ADDERR_CPU       | CPU address error
SIGBUS | BUS_ADDERR_DMA       | DMA address error
SIGFPE | FPE_INTDIV_TRAP      | integer zero divide
.TE

Two signals are provided for application use: SIGUSR1 and SIGUSR2.  
VxWorks will never use these signals; however, other signals may be used by
VxWorks in the future.

INTERNAL:
	WINDVIEW INSTRUMENTATION
Level 1:
        signal() causes EVENT_SIGNAL
        sigsuspend() causes EVENT_SIGSUSPEND
        pause() causes EVENT_PAUSE
        kill() causes EVENT_KILL
        sigWrapper() causes EVENT_SIGWRAPPER

Level 2:
        sigsuspend() causes EVENT_OBJ_SIGSUSPEND
        pause() causes EVENT_OBJ_SIGPAUSE
        sigtimedwait() causes EVENT_OBJ_SIGWAIT
        sigWindPendKill() causes EVENT_OBJ_SIGKILL

Level 3:
        N/A

INTERNAL:
  raise                           sigreturn                         sigwaitinfo
    |                                   \                             /
  kill         sigPendKill            sigprocmask  sigsuspend  sigtimedwait
    |               |                       \_________/            |
sigWindKill         |                            |                 |
    |               |                            |                 |
     \_____   _____/                             |                 |
           \ /                              sigPendRun             |
      sigWindPendKill                            |                 |
            |                                    |\_______   _____/
            |\__________________________________ |        \ /
            |    (->if I sent myself a sig ->)  \|     sigPendGet
            |                                    |
            |                                    |
            |\_sigWindRestart                    |
            |\__sigCtxStackEnd                   |
             \__sigCtxSetup         _sigCtxSave_/
                     

                  _________________________________________
                  |                                       |
                  |        ----------->sigWrapper         |
                  |                        |              |
                  |                    sigreturn          |
                  |_______________________________________|
                   ran in context of task receiving signal

The function sigstack is not, and will not be implemented.
All other functions below are done.
	
	BSD 4.3		POSIX
	-------		-----
	sigmask		sigemptyset sigfillset sigaddset sigdelset sigismember
	sigblock	sigprocmask
	sigsetmask	sigprocmask
	pause		sigsuspend
	sigvec		sigaction
	sigstack	(none)
	sigreturn	(none)
	(none)		sigpending
	signal		signal

The only major difference between this and posix is that the default action
when a signal occurs in vxWorks is to ignore it and in Posix is to kill the
process.

INCLUDE FILES: signal.h

SEE ALSO: intLib, IEEE 
.I "POSIX 1003.1b,"
.pG "Basic OS"
*/

#include "vxWorks.h"
#include "private/sigLibP.h"
#include "private/windLibP.h"
#include "private/taskLibP.h"
#include "private/kernelLibP.h"
#include "private/funcBindP.h"
#include "taskHookLib.h"
#include "errnoLib.h"
#include "intLib.h"
#include "qFifoGLib.h"
#include "stddef.h"
#include "stdlib.h"
#include "string.h"
#include "excLib.h"
#include "smObjLib.h"
#include "taskArchLib.h"
#include "timers.h"
#include "private/eventP.h"
#include "sysLib.h"
#define __PTHREAD_SRC
#include "pthread.h"

/*
 * Given a structure type, a member, and a pointer to that member, find the
 * pointer to the structure.
 */
#define structbase(s,m,p)         ((s *)((char *)(p) - offsetof(s,m)))

#define issig(m)	(1 <= (m) && (m) <= _NSIGS)

struct sigwait
    {
    sigset_t sigw_set;
    struct siginfo sigw_info;
    Q_HEAD sigw_wait;
    };


#if CPU_FAMILY != I960
extern int ffsMsb (unsigned long);      /* not in any header */
#endif

/* global variables */

#ifdef WV_INSTRUMENTATION
VOIDFUNCPTR     sigEvtRtn;		/* windview - level 1 event logging */
#endif

/* static variables */

struct sigpend *_pSigQueueFreeHead;

/* forward static functions */

static void		sigWindPendKill (WIND_TCB *, struct sigpend *);
static struct sigtcb	*sigTcbGet (void);
static void		sigWindKill (WIND_TCB *pTcb, int signo);
static int		sigWindRestart (WIND_TCB *pTcb);
static int		sigTimeoutRecalc(int i);
static void		sigDeleteHook (WIND_TCB *pNewTcb);
static void		sigWrapper (struct sigcontext *pContext);
static int		sigPendRun (struct sigtcb *pSigTcb);
static int		sigPendGet (struct sigtcb *pSigTcb,
				    const sigset_t *sigset,
				    struct siginfo *info);
static void		sigExcKill (int faultType, int faultSubtype,
				    REG_SET *pRegSet);

/*******************************************************************************
*
* sigInit - initialize the signal facilities
*
* This routine initializes the signal facilities.  It is usually called from
* the system start-up routine usrInit() in usrConfig, before interrupts are
* enabled.
*
* RETURNS: OK, or ERROR if the delete hooks cannot be installed.
*
* ERRNO: S_taskLib_TASK_HOOK_TABLE_FULL
*/

int sigInit (void)

    {
    static BOOL sigInstalled = FALSE;

    if (!sigInstalled)
        {
        _func_sigTimeoutRecalc	= sigTimeoutRecalc;
	_func_sigExcKill	= sigExcKill;
	_func_sigprocmask	= sigprocmask;

        if (taskDeleteHookAdd ((FUNCPTR) sigDeleteHook) == ERROR)
            return (ERROR);

/* FIXME-PR I do not think sigEvtRtn should be initialized here 
 * It is also initialize in wvLib.c and that should be enough 
 */
#if 0 
#ifdef WV_INSTRUMENTATION
        /* windview instrumentation - level 1 */
        if (WV_EVTCLASS_IS_SET(WV_CLASS_3))                  /* connect event logging routine */
            sigEvtRtn = _func_evtLogOIntLock;
        else
            sigEvtRtn = NULL;           /* disconnect event logging */
#endif
#endif

        sigInstalled = TRUE;
        }
 
    return (OK);
    }

/*******************************************************************************
*
* sigqueueInit - initialize the queued signal facilities
*
* This routine initializes the queued signal facilities. It must
* be called before any call to sigqueue().  It is usually
* called from the system start-up routine usrInit() in usrConfig,
* after sysInit() is called.
*
* It allocates <nQueues> buffers to be used by sigqueue(). A buffer is
* used by each call to sigqueue() and freed when the signal is delivered
* (thus if a signal is block, the buffer is unavailable until the signal
* is unblocked.)
*
* RETURNS: OK, or ERROR if memory could not be allocated.
*/

int sigqueueInit
    (
    int nQueues
    )
    {
    static BOOL sigqueueInstalled = FALSE;
    struct sigpend *p;

    if (!sigqueueInstalled)
        {
	if (nQueues < 1)
	    return (ERROR);

	if ((p = (struct sigpend *)malloc(nQueues * sizeof(struct sigpend)))
	    == NULL)
	    return (ERROR);

	while (nQueues-- > 0)
	    {
	    *(struct sigpend **)p= _pSigQueueFreeHead;
	    _pSigQueueFreeHead = p;
	    p++;
	    }

        sigqueueInstalled = TRUE;
        }
 
    return (OK);
    }

/*******************************************************************************
*
* sigDeleteHook - task delete hook for signal facility
*
* Cleanup any signals a task has hanging on it before the task gets
* deleted.
*
* RETURNS: Nothing.
*/

static void sigDeleteHook
    (
    WIND_TCB *pTcb		/* pointer to old task's TCB */
    )
    {
    struct sigtcb	*pSigTcb = pTcb->pSignalInfo;
    struct sigpend	*pSigPend;
    struct sigq		*pSigQ;
    int			ix;

    /*
     * If I never got any signals, nothing to clean up
     */
    if (pSigTcb == NULL)
	return;

    kernelState = TRUE;				/* KERNEL ENTER */

    for (ix = 0; ix <= _NSIGS; ix++)
	{
	pSigQ = pSigTcb->sigt_qhead[ix].sigq_next;
	while (pSigQ != &pSigTcb->sigt_qhead[ix])
	    {
	    pSigPend = structbase(struct sigpend, sigp_q, pSigQ);
	    pSigQ = pSigQ->sigq_next;
	    pSigPend->sigp_q.sigq_next = pSigPend->sigp_q.sigq_prev = NULL;

	    /*
	     * free queued signal buffer
	     */
	    if (pSigPend->sigp_info.si_code == SI_QUEUE)
		{
		*(struct sigpend **)pSigPend = _pSigQueueFreeHead;
		_pSigQueueFreeHead = pSigPend;
		}
	    }
	}

    windExit ();				/* KERNEL EXIT */
    }

/*******************************************************************************
*
* sigTimeoutRecalc - Compute the new timeout when a function restarts
*
* Compute the new timeout when a function restarts.  For example, a
* task pends on a semTake() with a timeout, receives a signal, runs the signal
* handler, and then goes back to waiting on the semTake().  What timeout
* should it use when it blocks?  In this example, it uses the original
* timeout passed to semTake().
*
* RETURNS: The new timeout value.
*/

static int sigTimeoutRecalc
    (
    int timeout			/* original timeout value */
    )
    {

    return (timeout);
    }

/*******************************************************************************
*
* sigemptyset - initialize a signal set with no signals included (POSIX)
*
* This routine initializes the signal set specified by <pSet>, 
* such that all signals are excluded.
*
* RETURNS: OK (0), or ERROR (-1) if the signal set cannot be initialized.
*
* ERRNO: No errors are detectable.
*/

int sigemptyset
    (
    sigset_t	*pSet		/* signal set to initialize */
    )
    {
    *pSet = 0;

    return (OK);
    }

/*******************************************************************************
*
* sigfillset - initialize a signal set with all signals included (POSIX)
*
* This routine initializes the signal set specified by <pSet>, such that
* all signals are included.
*
* RETURNS: OK (0), or ERROR (-1) if the signal set cannot be initialized.
*
* ERRNO: No errors are detectable.
*/

int sigfillset
    (
    sigset_t	*pSet		/* signal set to initialize */
    )
    {
    *pSet = 0xffffffff;

    return (OK);
    }

/*******************************************************************************
*
* sigaddset - add a signal to a signal set (POSIX)
*
* This routine adds the signal specified by <signo> to the signal set 
* specified by <pSet>.
*
* RETURNS: OK (0), or ERROR (-1) if the signal number is invalid.
*
* ERRNO: EINVAL
*/

int sigaddset
    (
    sigset_t	*pSet,		/* signal set to add signal to */
    int		signo		/* signal to add */
    )
    {

    if (issig (signo))
	{
	*pSet |= sigmask (signo);
	return (OK);
	}

    errnoSet (EINVAL);

    return (ERROR);
    }

/*******************************************************************************
*
* sigdelset - delete a signal from a signal set (POSIX)
*
* This routine deletes the signal specified by <signo> from the signal set
* specified by <pSet>.
*
* RETURNS: OK (0), or ERROR (-1) if the signal number is invalid.
*
* ERRNO: EINVAL
*/

int sigdelset
    (
    sigset_t	*pSet,		/* signal set to delete signal from */
    int		signo		/* signal to delete */
    )
    {

    if (issig (signo))
	{
	*pSet &= ~sigmask (signo);

	return (OK);
	}

    errnoSet (EINVAL);

    return (ERROR);
    }

/*******************************************************************************
*
* sigismember - test to see if a signal is in a signal set (POSIX)
*
* This routine tests whether the signal specified by <signo> is
* a member of the set specified by <pSet>.
*
* RETURNS: 1 if the specified signal is a member of the specified set, OK
* (0) if it is not, or ERROR (-1) if the test fails.
* 
* ERRNO: EINVAL
*/

int sigismember
    (
    const sigset_t      *pSet,  /* signal set to test */
    int                 signo   /* signal to test for */
    )
    {

    if (issig (signo))
        return ((*pSet & sigmask (signo)) != 0);

    errnoSet (EINVAL);

    return (ERROR);
    }

/*******************************************************************************
*
* signal - specify the handler associated with a signal
*
* This routine chooses one of three ways in which receipt of the signal
* number <signo> is to be subsequently handled.  If the value of <pHandler> is
* SIG_DFL, default handling for that signal will occur.  If the value of
* <pHandler> is SIG_IGN, the signal will be ignored.  Otherwise, <pHandler>
* must point to a function to be called when that signal occurs.
*
* RETURNS: The value of the previous signal handler, or SIG_ERR.
*/

void (*signal
    (
    int		signo,
    void	(*pHandler) ()
    )) ()
    {
    struct sigaction in, out;

    in.sa_handler = pHandler;
    in.sa_flags = 0;
    (void) sigemptyset (&in.sa_mask);

    return ((sigaction (signo, &in, &out) == ERROR) ? SIG_ERR : out.sa_handler);
    }

/*******************************************************************************
*
* sigaction - examine and/or specify the action associated with a signal (POSIX)
*
* This routine allows the calling process to examine and/or specify
* the action to be associated with a specific signal.
*
* RETURNS: OK (0), or ERROR (-1) if the signal number is invalid.
*
* ERRNO: EINVAL
*/

int sigaction
    (
    int				signo,	/* signal of handler of interest */
    const struct sigaction	*pAct,	/* location of new handler */
    struct sigaction		*pOact	/* location to store old handler */
    )
    {
    struct sigtcb *pSigTcb;
    struct sigaction *pSigAction;
    struct sigpend	*pSigPend;
    struct sigq		*pSigQ;

    if (!issig (signo))
	{
	errnoSet (EINVAL);
	return (ERROR);
	}

    if ((pSigTcb = sigTcbGet ()) == NULL)
	return (ERROR);				/* errno was set */

#ifdef WV_INSTRUMENTATION
    /* windview - level 1 event logging */
    EVT_OBJ_SIG (EVENT_SIGNAL, 2, signo, ((int) pAct->sa_handler));
#endif

    pSigAction = &pSigTcb->sigt_vec[signo];

    if (pOact != NULL)
	*pOact = *pSigAction;

    if (pAct != NULL)
	{
	kernelState = TRUE;			/* KERNEL ENTER */

	*pSigAction = *pAct;

	/*
	 * if the new action is to ignore, then any pending
	 * signals must be removed.
	 */
	if (pSigAction->sa_handler == SIG_IGN)
	    {
	    sigdelset (&pSigTcb->sigt_pending, signo);
	    sigdelset (&pSigTcb->sigt_kilsigs, signo);

		/* XXX need to destroy queued signals */
	    pSigQ = pSigTcb->sigt_qhead[signo].sigq_next;
	    while (pSigQ != &pSigTcb->sigt_qhead[signo])
		{
		pSigPend = structbase(struct sigpend, sigp_q, pSigQ);
		pSigQ = pSigQ->sigq_next;
		pSigPend->sigp_q.sigq_next = pSigPend->sigp_q.sigq_prev = NULL;

		/*
		 * free queued signal buffer
		 */
		if (pSigPend->sigp_info.si_code == SI_QUEUE)
		    {
		    *(struct sigpend **)pSigPend = _pSigQueueFreeHead;
		    _pSigQueueFreeHead = pSigPend;
		    }
		}
	    }

	windExit ();				/* KERNEL EXIT */
	}

    return (OK);
    }

/*******************************************************************************
*
* sigprocmask - examine and/or change the signal mask (POSIX)
*
* This routine allows the calling process to examine and/or change its
* signal mask.  If the value of <pSet> is not NULL, it points to a set of
* signals to be used to change the currently blocked set.
*
* The value of <how> indicates the manner in which the set is changed and
* consists of one of the following, defined in signal.h:
* .iP SIG_BLOCK 4
* the resulting set is the union of the current set and the signal set
* pointed to by <pSet>.
* .iP SIG_UNBLOCK
* the resulting set is the intersection of the current set and the complement
* of the signal set pointed to by <pSet>.
* .iP SIG_SETMASK
* the resulting set is the signal set pointed to by <pSset>.
*
* RETURNS: OK (0), or ERROR (-1) if <how> is invalid.
*
* ERRNO: EINVAL
*
* SEE ALSO: sigsetmask(), sigblock()
*/

int sigprocmask
    (
    int			how,	/* how signal mask will be changed */
    const sigset_t	*pSet,	/* location of new signal mask */
    sigset_t		*pOset	/* location to store old signal mask */
    )
    {
    struct sigtcb *pSigTcb;

    if ((pSigTcb = sigTcbGet()) == NULL)
	return (ERROR);				/* errno was set */

    kernelState = TRUE;				/* KERNEL ENTER */

    if (pOset != NULL)
	*pOset = pSigTcb->sigt_blocked;

    if (pSet != NULL)
	switch (how)
	    {
	    case SIG_BLOCK:
		pSigTcb->sigt_blocked |= *pSet;
		windExit ();			/* KERNEL EXIT */
		return (OK);

	    case SIG_UNBLOCK:
		pSigTcb->sigt_blocked &= ~*pSet;
		break;

	    case SIG_SETMASK:
		pSigTcb->sigt_blocked = *pSet;
		break;

	    default:
		windExit ();			/* KERNEL EXIT */
		errnoSet (EINVAL);
		return (ERROR);
	    }

    /*
     * check to see if we opened up any pending signals and run them.
     * sigpendrun has the horrible symantic of doing a windExit if
     * successful.
     */
    if (sigPendRun(pSigTcb) == FALSE)
	windExit ();				/* KERNEL EXIT */

    return (OK);
    }

/*******************************************************************************
*
* sigpending - retrieve the set of pending signals blocked from delivery (POSIX)
*
* This routine stores the set of signals that are blocked from delivery and
* that are pending for the calling process in the space pointed to by
* <pSet>.
*
* RETURNS: OK (0), or ERROR (-1) if the signal TCB cannot
* be allocated.
*
* ERRNO: ENOMEM
*/

int sigpending
    (
    sigset_t	*pSet		/* location to store pending signal set */
    )
    {
    struct sigtcb *pSigTcb;

    if ((pSigTcb = sigTcbGet ()) == NULL)
	return (ERROR);

    *pSet = pSigTcb->sigt_pending;
    return (OK);
    }

/*******************************************************************************
*
* sigsuspend - suspend the task until delivery of a signal (POSIX)
*
* This routine suspends the task until delivery of a signal.  While
* suspended, <pSet> is used as the set of masked signals.
*
* NOTE: Since the sigsuspend() function suspends thread execution
* indefinitely, there is no successful completion return value.
*
* RETURNS: -1, always.
*
* ERRNO: EINTR
*/

int sigsuspend
    (
    const sigset_t	*pSet	/* signal mask while suspended */
    )
    {
    Q_HEAD qHead;
    sigset_t oldset;
    struct sigtcb *pSigTcb;
    int savtype;

    if ((pSigTcb = sigTcbGet ()) == NULL)
        return (ERROR);			/* errno was set */

#ifdef WV_INSTRUMENTATION
    /* windview - level 1 event logging */
    EVT_OBJ_SIG (EVENT_SIGSUSPEND, 1, (int) *pSet, 0);
#endif

    if (_func_pthread_setcanceltype != NULL)
        {
        _func_pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &savtype);
        }

    kernelState = TRUE;				/* KERNEL ENTER */

    /* save old set */
    oldset = pSigTcb->sigt_blocked;
    pSigTcb->sigt_blocked = *pSet;

    /* check for pending */
    if (sigPendRun (pSigTcb) == TRUE)
	{
	sigprocmask (SIG_SETMASK, &oldset, NULL);
	if (_func_pthread_setcanceltype != NULL)
            {
            _func_pthread_setcanceltype(savtype, NULL);
            }
	errnoSet (EINTR);
	return (ERROR);
	}

    /*
     * Put ourself to sleep until a signal comes in.
     */
    qInit (&qHead, Q_FIFO, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

#ifdef WV_INSTRUMENTATION
    /* windview - level 2 event logging */
    EVT_TASK_1 (EVENT_OBJ_SIGSUSPEND, pSet); 
#endif

    if (windPendQPut (&qHead, WAIT_FOREVER) != OK)
        {
	windExit ();				/* KERNEL EXIT */

        if (_func_pthread_setcanceltype != NULL)
            {
            _func_pthread_setcanceltype(savtype, NULL);
            }
	return (ERROR);
        }
    windExit ();				/* KERNEL EXIT */

    /*
     * Restore old mask set.
     */
    sigprocmask(SIG_SETMASK, &oldset, NULL);

    if (_func_pthread_setcanceltype != NULL)
        {
        _func_pthread_setcanceltype(savtype, NULL);
        }

    errnoSet (EINTR);

    return (ERROR);
    }

/*******************************************************************************
*
* pause - suspend the task until delivery of a signal (POSIX)
*
* This routine suspends the task until delivery of a signal.
*
* NOTE: Since the pause() function suspends thread execution indefinitely,
* there is no successful completion return value.
*
* RETURNS: -1, always.
*
* ERRNO: EINTR
*/

int pause (void)

    {
    Q_HEAD qHead;
    int savtype;

#ifdef WV_INSTRUMENTATION
    /* windview - level 1 event logging */
    EVT_OBJ_SIG (EVENT_PAUSE, 1, taskIdCurrent,0);
#endif

    if (_func_pthread_setcanceltype != NULL)
        {
        _func_pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &savtype);
        }

    /*
     * Put ourself to sleep until a signal comes in.
     */
    qInit (&qHead, Q_FIFO, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    kernelState = TRUE;				/* KERNEL ENTER */

#ifdef WV_INSTRUMENTATION
    /* windview - level 2 event logging */
    EVT_TASK_1 (EVENT_OBJ_SIGPAUSE, taskIdCurrent);  /* log event */
#endif

    if (windPendQPut (&qHead, WAIT_FOREVER) != OK)
        {
	windExit ();				/* KERNEL EXIT */
	if (_func_pthread_setcanceltype != NULL)
            {
            _func_pthread_setcanceltype(savtype, NULL);
            }
	return (ERROR);
        }

    windExit ();				/* KERNEL EXIT */

    if (_func_pthread_setcanceltype != NULL)
        {
        _func_pthread_setcanceltype(savtype, NULL);
        }
    errnoSet (EINTR);

    return (ERROR);
    }

/*******************************************************************************
*
* sigtimedwait - wait for a signal
*
* The function sigtimedwait() selects the pending signal from the set
* specified by <pSet>.  If multiple signals in <pSet> are pending, it will
* remove and return the lowest numbered one.  If no signal in <pSet> is pending
* at the
* time of the call, the task will be suspend until one of the signals in <pSet>
* become pending, it is interrupted by an unblocked caught signal, or
* until the time interval specified by <pTimeout> has expired.
* If <pTimeout> is NULL, then the timeout interval is forever.
*
* If the <pInfo> argument is non-NULL, the selected signal number is
* stored in the `si_signo' member, and the cause of the signal is
* stored in the `si_code' member.  If the signal is a queued signal,
* the value is stored in the `si_value' member of <pInfo>; otherwise
* the content of `si_value' is undefined.
*
* The following values are defined in signal.h for `si_code':
* .iP SI_USER
* the signal was sent by the kill() function.
* .iP SI_QUEUE
* the signal was sent by the sigqueue() function.
* .iP SI_TIMER
* the signal was generated by the expiration of a timer set by timer_settime().
* .iP SI_ASYNCIO
* the signal was generated by the completion of an asynchronous I/O request.
* .iP SI_MESGQ
* the signal was generated by the arrival of a message on an empty message
* queue.
* .LP
*
* The function sigtimedwait() provides a synchronous mechanism for tasks
* to wait for asynchromously generated signals.  A task should use
* sigprocmask() to block any signals it wants to handle synchronously and
* leave their signal handlers in the default state.  The task can then
* make repeated calls to sigtimedwait() to remove any signals that are
* sent to it.
*
* RETURNS: Upon successful completion (that is, one of the signals specified
* by <pSet> is pending or is generated) sigtimedwait() will return the selected
* signal number.  Otherwise, a value of -1 is returned and `errno' is set to
* indicate the error.
*
* ERRNO
* .iP EINTR
* The wait was interrupted by an unblocked, caught signal.
* .iP EAGAIN
* No signal specified by <pSet> was delivered within the specified timeout
* period.
* .iP EINVAL
* The <pTimeout> argument specified a `tv_nsec' value less than zero or greater
* than or equal to 1000 million.
*
* SEE ALSO: sigwait()
*/

int sigtimedwait
    (
    const sigset_t		*pSet,	/* the signal mask while suspended */
    struct siginfo		*pInfo,	/* return value */
    const struct timespec	*pTimeout
    )
    {
    struct sigtcb *pSigTcb;
    struct sigwait waitinfo;
    struct siginfo info;
    int signo;
    int wait;
    int status;
    int savtype;

    if ((pSigTcb = sigTcbGet()) == NULL)
	return (ERROR);			/* errno was set */

    if (_func_pthread_setcanceltype != NULL)
        {
        _func_pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &savtype);
        }

    if (pTimeout != 0)
	{
	int tickRate = sysClkRateGet();

	if ((pTimeout->tv_nsec < 0) || (pTimeout->tv_nsec > 1000000000))
	    {
	    errno = EINVAL;
	    if (_func_pthread_setcanceltype != NULL)
                {
                _func_pthread_setcanceltype(savtype, NULL);
                }
	    return (ERROR);
	    }

	wait = pTimeout->tv_sec * tickRate +
		pTimeout->tv_nsec / (1000000000 / tickRate);
	}
    else
	wait = WAIT_FOREVER;

    kernelState = TRUE;				/* KERNEL ENTER */

    if ((signo = sigPendGet(pSigTcb, pSet, &info)) > 0)
	{
	windExit ();				/* KERNEL EXIT */
	if (pInfo != NULL)
		*pInfo = info;

        if (_func_pthread_setcanceltype != NULL)
            {
            _func_pthread_setcanceltype(savtype, NULL);
            }
	
	return (signo);
	}

    waitinfo.sigw_set = *pSet;
    qInit (&waitinfo.sigw_wait, Q_FIFO, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    pSigTcb->sigt_wait = &waitinfo;

#ifdef WV_INSTRUMENTATION
    /* windview - level 2 event logging */
    EVT_TASK_1 (EVENT_OBJ_SIGWAIT, waitinfo.sigw_wait);
#endif

    if (windPendQPut (&waitinfo.sigw_wait, wait) != OK)
        {
	windExit ();				/* KERNEL EXIT */
 
        if (_func_pthread_setcanceltype != NULL)
            {
            _func_pthread_setcanceltype(savtype, NULL);
            }

        return (ERROR);
        }

    status = windExit ();				/* KERNEL EXIT */

    pSigTcb->sigt_wait = 0;

    if (status != 0)
	{ 
	if (_func_pthread_setcanceltype != NULL)
            {
            _func_pthread_setcanceltype(savtype, NULL);
            }

	errnoSet ((status == RESTART) ? EINTR : EAGAIN);
	return (-1);
	}

    if (pInfo != NULL)
	*pInfo = waitinfo.sigw_info;

    if (_func_pthread_setcanceltype != NULL)
        {
        _func_pthread_setcanceltype(savtype, NULL);
        }
 
    return (waitinfo.sigw_info.si_signo);
    }

/*******************************************************************************
*
* sigwaitinfo - wait for real-time signals
*
* The function sigwaitinfo() is equivalent to calling sigtimedwait() with
* <pTimeout> equal to NULL.  See that manual entry for more information.
*
* RETURNS: Upon successful completion (that is, one of the signals specified
* by <pSet> is pending or is generated) sigwaitinfo() returns the selected
* signal number.  Otherwise, a value of -1 is returned and `errno' is set to
* indicate the error.
*
* ERRNO
* .iP EINTR
* The wait was interrupted by an unblocked, caught signal.
*/

int sigwaitinfo
    (
    const sigset_t	*pSet,	/* the signal mask while suspended */
    struct siginfo	*pInfo	/* return value */
    )
    {

    return (sigtimedwait (pSet, pInfo, (struct timespec *)NULL));
    }

/*******************************************************************************
*
* sigwait - wait for a signal to be delivered (POSIX)
*
* This routine waits until one of the signals specified in <pSet> is delivered
* to the calling thread. It then stores the number of the signal received in
* the the location pointed to by <pSig>.
*
* The signals in <pSet> must not be ignored on entrance to sigwait().
* If the delivered signal has a signal handler function attached, that
* function is not called.
*
* RETURNS: OK, or ERROR on failure.
*
* SEE ALSO: sigtimedwait()
*/
 
int sigwait
    (
    const sigset_t *pSet,
    int *pSig
    )
    {
    struct siginfo info;
 
    if (sigtimedwait(pSet, &info, (struct timespec *)NULL) == ERROR)
        return(ERROR);
 
    *pSig = info.si_signo;
    return(OK);
    }

/********************************************************************************
*
* sigvec - install a signal handler
*
* This routine binds a signal handler routine referenced by <pVec> to a
* specified signal <sig>.  It can also be used to determine which handler,
* if any, has been bound to a particular signal:  sigvec() copies current
* signal handler information for <sig> to <pOvec> and does not install a
* signal handler if <pVec> is set to NULL (0).
*
* Both <pVec> and <pOvec> are pointers to a structure of type \f3struct
* sigvec\fP.  The information passed includes not only the signal handler
* routine, but also the signal mask and additional option bits.  The
* structure `sigvec' and the available options are defined in signal.h.
*
* RETURNS: OK (0), or ERROR (-1) if the signal number is invalid or the
* signal TCB cannot be allocated.
*
* ERRNO: EINVAL, ENOMEM
*/
int sigvec
    (
    int			sig,	/* signal to attach handler to */
    const struct sigvec	*pVec,	/* new handler information */
    struct sigvec	*pOvec	/* previous handler information */
    )
    {
    struct sigaction act, oact;
    int retVal;

    if (pVec != NULL)
	{
	act.sa_handler = pVec->sv_handler;
	act.sa_mask = pVec->sv_mask;
	act.sa_flags = pVec->sv_flags;
	retVal = sigaction(sig, &act, &oact);
	}
    else
	retVal = sigaction(sig, NULL, &oact);
    if (pOvec != NULL)
	{
	pOvec->sv_handler = oact.sa_handler;
	pOvec->sv_mask = oact.sa_mask;
	pOvec->sv_flags = oact.sa_flags;
	}
    return (retVal);
    }

/*******************************************************************************
*
* sigreturn - early return from a signal handler
*
* NOMANUAL
*
*/

void sigreturn
    (
    struct sigcontext	*pContext
    )
    {
    (void) sigprocmask (SIG_SETMASK, &pContext->sc_mask, 0);
    _sigCtxLoad (&pContext->sc_regs);
    }

/*******************************************************************************
*
* sigsetmask - set the signal mask
*
* This routine sets the calling task's signal mask to a specified value.
* A one (1) in the bit mask indicates that the specified signal is blocked
* from delivery.  Use the macro SIGMASK to construct the mask for a specified
* signal number.
*
* RETURNS: The previous value of the signal mask.
*
* SEE ALSO: sigprocmask()
*/
 
int sigsetmask
    (
    int			mask	/* new signal mask */
    )
    {
    sigset_t in;
    sigset_t out;

    in = (sigset_t) mask;
    (void) sigprocmask (SIG_SETMASK, &in, &out);
    return ((int) out);
    }

/*******************************************************************************
*
* sigblock - add to a set of blocked signals
*
* This routine adds the signals in <mask> to the task's set of blocked signals.
* A one (1) in the bit mask indicates that the specified signal is blocked
* from delivery.  Use the macro SIGMASK to construct the mask for a specified
* signal number.
*
* RETURNS: The previous value of the signal mask.
*
* SEE ALSO: sigprocmask()
*/
 
int sigblock
    (
    int			mask	/* mask of additional signals to be blocked */
    )
    {
    sigset_t in;
    sigset_t out;

    in = (sigset_t) mask;
    (void) sigprocmask (SIG_BLOCK, &in, &out);
    return ((int) out);
    }

/*******************************************************************************
*
* raise - send a signal to the caller's task
*
* This routine sends the signal <signo> to the task invoking the call.
*
* RETURNS: OK (0), or ERROR (-1) if the signal number or task ID is invalid.
*
* ERRNO: EINVAL
*/

int raise
    (
    int			signo	/* signal to send to caller's task */
    )
    {

    return (kill ((int) taskIdCurrent, signo));
    }

/*******************************************************************************
*
* kill - send a signal to a task (POSIX)
*
* This routine sends a signal <signo> to the task specified by <tid>.
*
* RETURNS: OK (0), or ERROR (-1) if the task ID or signal number is invalid.
*
* ERRNO: EINVAL
*/

int kill
    (
    int			tid,	/* task to send signal to */
    int			signo	/* signal to send to task */
    )
    {

    if (!issig (signo))
	{
	errnoSet (EINVAL);
	return (ERROR);
	}

#ifdef WV_INSTRUMENTATION
    /* windview - level 1 event logging */
    EVT_OBJ_SIG (EVENT_KILL , 2, signo, tid);
#endif

    if (kernelState || (INT_CONTEXT () && (int) taskIdCurrent == tid))
	{
	if (TASK_ID_VERIFY (tid) != OK)
	    {
	    errnoSet (EINVAL);
	    return (ERROR);
	    }

	excJobAdd ((VOIDFUNCPTR) kill, tid, signo, 0, 0, 0, 0);
	return (OK);
	}

    kernelState = TRUE;				/* KERNEL ENTER */
 
    if (TASK_ID_VERIFY (tid) != OK)		/* verify task */
	{
	windExit ();				/* KERNEL EXIT */
	errnoSet (EINVAL);
	return (ERROR);
	}

    sigWindKill ((WIND_TCB *) tid, signo);	/* send signal */
 
    windExit ();				/* KERNEL EXIT */
 
    return (OK);
    }

/*******************************************************************************
*
* sigqueue - send a queued signal to a task
* 
* The function sigqueue() sends the signal specified by <signo> with
* the signal-parameter value specified by <value> to the process
* specified by <tid>.
*
* RETURNS: OK (0), or ERROR (-1) if the task ID or signal number is invalid,
* or if there are no queued-signal buffers available.
*
* ERRNO: EINVAL EAGAIN
*/
int sigqueue
    (
    int			tid,
    int			signo,
    const union sigval	value
    )
    {
    struct sigpend pend;

    if (!issig (signo))
	{
	errnoSet (EINVAL);
	return (ERROR);
	}

#ifdef WV_INSTRUMENTATION
    /* windview - level 1 event logging */
    EVT_OBJ_SIG (EVENT_KILL , 2, signo, tid);
#endif

    if (kernelState || (INT_CONTEXT () && (int) taskIdCurrent == tid))
	{
	if (TASK_ID_VERIFY (tid) != OK)
	    {
	    errnoSet (EINVAL);
	    return (ERROR);
	    }

	if (_pSigQueueFreeHead == NULL)
	    {
	    errnoSet (EAGAIN);
	    return (ERROR);
	    }

	excJobAdd ((VOIDFUNCPTR)sigqueue, tid, signo, value.sival_int, 0, 0, 0);
	return (OK);
	}

    kernelState = TRUE;				/* KERNEL ENTER */
 
    if (TASK_ID_VERIFY (tid) != OK)		/* verify task */
	{
	windExit ();				/* KERNEL EXIT */
	errnoSet (EINVAL);
	return (ERROR);
	}

    if (_pSigQueueFreeHead == NULL)
	{
	windExit ();				/* KERNEL EXIT */
	errnoSet (EAGAIN);
	return (ERROR);
	}

    sigPendInit (&pend);

    pend.sigp_info.si_signo = signo;
    pend.sigp_info.si_code  = SI_QUEUE;
    pend.sigp_info.si_value = value;

    sigWindPendKill ((WIND_TCB *) tid, &pend);	/* signal a task */
 
    windExit ();				/* KERNEL EXIT */

    return (0);
    }

/*******************************************************************************
*
* sigWrapper - trampoline code to get to users signal handler from kernel
*
*/

static void sigWrapper
    (
    struct sigcontext	*pContext
    )
    {
    int signo = pContext->sc_info.si_signo;
    struct sigaction *pAction =
	&((taskIdCurrent->pSignalInfo))->sigt_vec[signo];
    void (*handler)() = pAction->sa_handler;

#ifdef WV_INSTRUMENTATION
    /* windview - level 1 event logging */
    EVT_OBJ_SIG (EVENT_SIGWRAPPER, 2, signo, taskIdCurrent);
#endif

    if (pAction->sa_flags & SA_RESETHAND)
	(void) signal (signo, SIG_DFL);

    if (handler != SIG_DFL && handler != SIG_IGN && handler != SIG_ERR)
	{
	if (pAction->sa_flags & SA_SIGINFO)
	    (*handler) (signo, &pContext->sc_info, pContext);
	else
	    (*handler) (signo, pContext->sc_info.si_value.sival_int, pContext);
	}

    sigreturn (pContext);
    }

/*******************************************************************************
*
* sigTcbGet - get the pointer to the signal tcb
*
* Get the pointer to the signal tcb.  If there is no signal tcb, try and
* allocate it.
*
*/

static struct sigtcb *sigTcbGet (void)

    {
    WIND_TCB *pTcb = taskIdCurrent;
    struct sigtcb *pSigTcb;
    int ix;

    if (INT_RESTRICT () != OK)
	return (NULL);			/* errno was set */

    if (pTcb->pSignalInfo != NULL)
	return (pTcb->pSignalInfo);

    pSigTcb = (struct sigtcb *) taskStackAllot((int) taskIdCurrent,
					       sizeof (struct sigtcb));
    if (pSigTcb == NULL)
	{
	errnoSet (ENOMEM);
	return (NULL);
	}

    pTcb->pSignalInfo = pSigTcb;
    bzero ((void *) pSigTcb, sizeof (struct sigtcb));

    for (ix = 0; ix <= _NSIGS; ix++)
	pSigTcb->sigt_qhead[ix].sigq_next = pSigTcb->sigt_qhead[ix].sigq_prev =
		&pSigTcb->sigt_qhead[ix];

    return (pSigTcb);
    }

/*******************************************************************************
*
* sigPendRun - get a signal from the set of pending signals and run it
*
* NOTE: YOU MUST BE IN THE KERNEL.
*/

static int sigPendRun
    (
    struct sigtcb	*pSigTcb
    )
    {
    sigset_t set = ~pSigTcb->sigt_blocked;
    struct sigcontext sigCtx;
    int signo;

    if ((signo = sigPendGet (pSigTcb, &set, &sigCtx.sc_info)) > 0)
	{
	sigCtx.sc_onstack      = 0;
	sigCtx.sc_restart      = 0;
	sigCtx.sc_mask         = pSigTcb->sigt_blocked;
	pSigTcb->sigt_blocked |= (pSigTcb->sigt_vec[signo].sa_mask |
				  sigmask(signo));
#if CPU_FAMILY == MC680X0
	sigCtx.sc_fformat      = 0;
#endif
	windExit ();				/* KERNEL EXIT */

	if (_sigCtxSave (&sigCtx.sc_regs) == 0)
	    {
	    _sigCtxRtnValSet (&sigCtx.sc_regs, 1);
	    sigWrapper (&sigCtx);
	    }

	return (TRUE);
	}

    return (FALSE);
    }

/*******************************************************************************
*
* sigPendGet - get a signal from the set of pending signals
*
* NOTE: YOU MUST BE IN THE KERNEL.
*/

static int sigPendGet
    (
    struct sigtcb	*pSigTcb,
    const sigset_t	*sigset,
    struct siginfo 	*info
    )
    {
    long		sig;
    long		mask;
    struct sigpend	*pSigpend;

    mask = *sigset & pSigTcb->sigt_pending;

    if (mask == 0)
	return (OK);

    mask &= -mask;			/* find lowest bit set */
    sig = ffsMsb (mask);

    if (mask & pSigTcb->sigt_kilsigs)
	{
	pSigTcb->sigt_kilsigs		&= ~mask;
	info->si_signo			= sig;
	info->si_code			= SI_KILL;
	info->si_value.sival_int	= 0;
	}
    else
	{
	pSigpend = structbase(struct sigpend, sigp_q,
			      pSigTcb->sigt_qhead[sig].sigq_next);

	pSigpend->sigp_q.sigq_prev->sigq_next = pSigpend->sigp_q.sigq_next;
	pSigpend->sigp_q.sigq_next->sigq_prev = pSigpend->sigp_q.sigq_prev;
	pSigpend->sigp_q.sigq_next = NULL;
	pSigpend->sigp_q.sigq_prev = NULL;
	*info = pSigpend->sigp_info;
	pSigpend->sigp_overruns = pSigpend->sigp_active_overruns;
	pSigpend->sigp_active_overruns = 0;

	/*
	 * free queued signal buffer
	 */
	if (pSigpend->sigp_info.si_code == SI_QUEUE)
	    {
	    *(struct sigpend **)pSigpend = _pSigQueueFreeHead;
	    _pSigQueueFreeHead = pSigpend;
	    }

	}

    if (pSigTcb->sigt_qhead[sig].sigq_next == &pSigTcb->sigt_qhead[sig])
	pSigTcb->sigt_pending &= ~mask;

    return (sig);
    }

/*******************************************************************************
*
* sigWindPendKill - add a signal to the set of pending signals
*
* NOTE: YOU MUST BE IN THE KERNEL.
*/

static void sigWindPendKill
    (
    WIND_TCB		*pTcb,
    struct sigpend	*pSigPend
    )
    {
    int			signo = pSigPend->sigp_info.si_signo;
    struct sigtcb	*pSigTcb;
    void		(*func)();
    unsigned		mask;

    /*
     * Check to see if the task has the signal structure.  If it does
     * not, we pretend the task is ignoring them.
     */
    if ((pSigTcb = pTcb->pSignalInfo) == NULL)
	return;

    /*
     * If task is waiting for signal, deliver it by waking up the task.
     */
    if ((pSigTcb->sigt_wait != NULL) &&
	(sigismember(&pSigTcb->sigt_wait->sigw_set, signo) == 1) &&
	(Q_FIRST(&pSigTcb->sigt_wait->sigw_wait) != NULL))
	{
	pSigTcb->sigt_wait->sigw_info = pSigPend->sigp_info;

#ifdef WV_INSTRUMENTATION
        /* windview - level 2 event logging */
  	EVT_TASK_1 (EVENT_OBJ_SIGKILL, pTcb); /* log event */
#endif

	windPendQGet (&pSigTcb->sigt_wait->sigw_wait);

	/*
	 * Mark task as no longer waiting
	 */
	pSigTcb->sigt_wait = NULL;

	return;
	}

    /*
     * The default action for signals is to ignore them, in posix
     * it is to kill the process.
     */
    func = pSigTcb->sigt_vec[signo].sa_handler;
    if (func == SIG_DFL)
	return;			/* should maybe kill the task */

    /*
     * Check to see if signal should be ignored.
     */
    if (func == SIG_IGN)
	return;

    /*
     * Check to see if signal should be queued up because they are being
     * blocked.
     */
    mask = sigmask(signo);

    if (pSigTcb->sigt_blocked & mask)
	{
	/*
	 * kill signals just get a bit to say a signal has come
	 */
	if (pSigPend->sigp_info.si_code == SI_KILL)
	    {
	    pSigTcb->sigt_kilsigs |= mask;
	    pSigTcb->sigt_pending |= mask;
	    }

	/*
	 * if the signal is all ready queued, mark it as overrun
	 */
	else if (pSigPend->sigp_q.sigq_next != NULL)
	    pSigPend->sigp_active_overruns++;

	/*
	 * must be a signal to queue
	 */
	else
	    {
	    struct sigq *head = &pSigTcb->sigt_qhead[signo];

	    /*
	     * alloc buffer for sigqueue
	     */
	    if (pSigPend->sigp_info.si_code == SI_QUEUE)
		{
		struct sigpend *pTmp = _pSigQueueFreeHead;

		_pSigQueueFreeHead = *(struct sigpend **)_pSigQueueFreeHead;
		*pTmp = *pSigPend;
		pSigPend = pTmp;
		}

	    pSigPend->sigp_q.sigq_prev = head->sigq_prev;
	    pSigPend->sigp_q.sigq_next = head;
	    pSigPend->sigp_q.sigq_prev->sigq_next = &pSigPend->sigp_q;
	    head->sigq_prev = &pSigPend->sigp_q;
	    pSigTcb->sigt_pending |= mask;
	    pSigPend->sigp_tcb = pSigTcb;
	    }
	return;
	}

    /*
     * Force delivery of the signal.
     */
    if (taskIdCurrent != pTcb)
	{
	int args[2];
	struct sigcontext *pCtx;

#if _STACK_DIR == _STACK_GROWS_DOWN
	pCtx = (struct sigcontext *) MEM_ROUND_DOWN
	       (((struct sigcontext *)_sigCtxStackEnd (&pTcb->regs)) - 1);
#else
	taskRegsStackToTcb (pTcb);
	pCtx = (struct sigcontext *)
	     MEM_ROUND_UP(_sigCtxStackEnd (&pTcb->regs));
#endif
	args[0] = 1;
	args[1] = (int)pCtx;

	/*
	 * Mark task as no longer waiting
	 */
	pSigTcb->sigt_wait     = NULL;

	pCtx->sc_onstack       = 0;
	pCtx->sc_restart       = sigWindRestart (pTcb);
	pCtx->sc_mask          = pSigTcb->sigt_blocked;
	pSigTcb->sigt_blocked |= (pSigTcb->sigt_vec[signo].sa_mask | mask);
	pCtx->sc_info          = pSigPend->sigp_info;
	pCtx->sc_regs          = pTcb->regs;
#if CPU_FAMILY == MC680X0
	pCtx->sc_fformat = pTcb->foroff;
#endif
#if CPU == I960KB
	bcopy (pTcb->intResumptionRec, pCtx->sc_resumption, 16);
#endif
	pCtx->sc_pregs         = pSigPend->sigp_pregs;
#if _STACK_DIR == _STACK_GROWS_DOWN
	_sigCtxSetup (&pTcb->regs, (void *)STACK_ROUND_DOWN(pCtx), sigWrapper, args);
#else
	_sigCtxSetup (&pTcb->regs, (void *)STACK_ROUND_UP(pCtx + 1), sigWrapper, args);
#endif
	}
    else
	{
	struct sigcontext Ctx;

	Ctx.sc_onstack         = 0;
	Ctx.sc_restart         = 0;
	Ctx.sc_mask            = pSigTcb->sigt_blocked;
	pSigTcb->sigt_blocked |= (pSigTcb->sigt_vec[signo].sa_mask | mask);
	Ctx.sc_info            = pSigPend->sigp_info;
#if CPU_FAMILY == MC680X0
	Ctx.sc_fformat         = pTcb->foroff;
#endif
#if CPU == I960KB
	bcopy (pTcb->intResumptionRec, Ctx.sc_resumption, 16);
#endif
	Ctx.sc_pregs           = pSigPend->sigp_pregs;
	windExit ();				/* KERNEL EXIT */

	if (_sigCtxSave(&Ctx.sc_regs) == 0)
	    {
	    _sigCtxRtnValSet (&Ctx.sc_regs, 1);
	    sigWrapper(&Ctx);
	    }
	kernelState = TRUE;			/* KERNEL ENTER */
	}
    }

/*******************************************************************************
*
* sigPendKill - send a queue signal
*
* NOMANUAL
*
* RETURNS: nothing
*/

int sigPendKill
    (
    int			tid,
    struct sigpend	*pSigPend
    )
    {

#if 0
    /*
     * for now, blindly assume it is valid.
     */
    if (!issig (pSigPend))
	{
	errnoSet (EINVAL);
	return (ERROR);
	}
#endif

    if (kernelState || (INT_CONTEXT () && (int) taskIdCurrent == tid))
	{
	if (TASK_ID_VERIFY (tid) != OK)
	    {
	    errnoSet (EINVAL);
	    return (ERROR);
	    }

	excJobAdd ((VOIDFUNCPTR) sigPendKill, tid, (int) pSigPend, 0, 0, 0, 0);
	return (OK);
	}

    kernelState = TRUE;				/* KERNEL ENTER */
 
    if (TASK_ID_VERIFY (tid) != OK)		/* verify task */
	{
	windExit ();				/* KERNEL EXIT */
	errnoSet (EINVAL);
	return (ERROR);
	}

    sigWindPendKill ((WIND_TCB *) tid, pSigPend);	/* signal a task */
 
    windExit ();				/* KERNEL EXIT */
 
    return (OK);
    }

/*******************************************************************************
*
* sigPendInit - init a queue signal
*
* NOMANUAL
*
* RETURNS: nothing
*/

void sigPendInit
    (
    struct sigpend	*pSigPend
    )
    {

    pSigPend->sigp_q.sigq_prev = pSigPend->sigp_q.sigq_next = NULL;
    pSigPend->sigp_overruns = 0; 
    pSigPend->sigp_pregs = NULL; 
    }

/*******************************************************************************
*
* sigPendDestroy - destory a queue signal
*
* NOMANUAL
*
* RETURNS: OK or ERROR
*/

int sigPendDestroy
    (
    struct sigpend	*pSigPend
    )
    {

    if (INT_RESTRICT () != OK)
	return (ERROR);

    pSigPend->sigp_overruns = 0; 

    kernelState = TRUE;				/* KERNEL ENTER */

    if (pSigPend->sigp_q.sigq_next != 0)
	{
	/*
	 * Clear sigpending if last one on chain. (and there was no kilsigs)
	 */
	if (pSigPend->sigp_q.sigq_next == pSigPend->sigp_q.sigq_prev)
	    pSigPend->sigp_tcb->sigt_pending &=
    		(pSigPend->sigp_tcb->sigt_kilsigs |
		~sigmask(pSigPend->sigp_info.si_signo));
	pSigPend->sigp_q.sigq_next->sigq_prev = pSigPend->sigp_q.sigq_prev;
	pSigPend->sigp_q.sigq_prev->sigq_next = pSigPend->sigp_q.sigq_next;
	pSigPend->sigp_q.sigq_prev = pSigPend->sigp_q.sigq_next = NULL;
	}

    windExit ();				/* KERNEL EXIT */

    return (OK);
    }

/*******************************************************************************
*
* sigWindKill - do the dirty work of sending a signal.
*
* NOTE: YOU MUST BE IN THE KERNEL.
*/

static void sigWindKill
    (
    WIND_TCB		*pTcb,
    int			signo
    )
    {
    struct sigpend pend;

    pend.sigp_info.si_signo	      = signo;
    pend.sigp_info.si_code            = SI_KILL;
    pend.sigp_info.si_value.sival_int = 0;
    sigWindPendKill(pTcb, &pend);
    }

/*******************************************************************************
*
* sigWindRestart - test the state of a task
*
* Test the state of a task you are sending a signal to.  If it is
* pended, tell it to restart itself after it has returned.
*
* NOTE: YOU MUST BE IN THE KERNEL.
*/

static int sigWindRestart
    (
    WIND_TCB		*pTcb
    )
    {
    int delay = WAIT_FOREVER;
    int status;			/* Q_REMOVE return value */

    if ((pTcb->status & (WIND_PEND | WIND_DELAY)) != 0)
	{
	taskRtnValueSet (pTcb, RESTART);
	if ((pTcb->status & WIND_DELAY) != 0)
	    {
	    pTcb->status &= ~WIND_DELAY;
	    Q_REMOVE (&tickQHead, &pTcb->tickNode);	/* remove from queue */
	    delay = Q_KEY (&tickQHead, &pTcb->tickNode, 1);
	    }
	if ((pTcb->status & WIND_PEND) != 0)
	    {
	    pTcb->pPriMutex = NULL;			/* clear pPriMutex */
	    pTcb->status   &= ~WIND_PEND;		/* turn off pend bit */
	    status = Q_REMOVE (pTcb->pPendQ, pTcb);	/* out of pend queue */

	    /* 
	     * If task was pended on a shared semaphore its shared TCB 
	     * could have been remove by a semGive on another CPU.  In that 
	     * case Q_REMOVE returns ALREADY_REMOVED, What we have to do
	     * is set task return value to OK to avoid task to be restarted.
	     * Q_REMOVE can also return ERROR if it was unable to take 
	     * the shared semaphore lock to remove the shared TCB.
	     * In that case we set task return value to ERROR and 
	     * errorStatus to S_smObjLib_LOCK_TIMEOUT.
	     */

	    switch (status)
		{
		case ALREADY_REMOVED :
		    {
		    taskRtnValueSet (pTcb, OK);
		    break;
		    }
		case ERROR :
		    {
		    taskRtnValueSet (pTcb, ERROR);
		    pTcb->errorStatus = S_smObjLib_LOCK_TIMEOUT;
		    break;
		    }
		}
	    }
	if (pTcb->status == WIND_READY)			/* task is now ready */
	    Q_PUT (&readyQHead, pTcb, pTcb->priority);
	}

    return (delay);
    }


/*******************************************************************************
*
* sigExcSend - signal a task that has an exception
*
* RETURNS: nothing
*/
static void sigExcSend
    (
    int		signo,
    int		code,
    REG_SET	*pRegSet
    )
    {
    struct sigpend pend;
    REG_SET *pTmpRegSet;


    pTmpRegSet = taskIdCurrent->pExcRegSet;
    taskIdCurrent->pExcRegSet = 0;

    sigPendInit (&pend);

    pend.sigp_info.si_signo	      = signo;
    pend.sigp_info.si_code            = SI_SYNC;
    pend.sigp_info.si_value.sival_int = code;
    pend.sigp_pregs                   = pRegSet;

    sigPendKill ((int) taskIdCurrent, &pend);
    sigPendDestroy (&pend);

    taskIdCurrent->pExcRegSet = pTmpRegSet;
    }

/*******************************************************************************
*
* sigExcKill - signal a task that has an exception
*
* RETURNS: nothing
*/
static void sigExcKill
    (
    int      faultType,
    int      code,
    REG_SET  *pRegSet
    )
    {
    extern struct sigfaulttable _sigfaulttable[];
    struct sigfaulttable *pFTab;

    for (pFTab = &_sigfaulttable[0]; 
	 !((pFTab->sigf_fault == 0) && (pFTab->sigf_signo == 0));
	 pFTab++)
	{
	if (pFTab->sigf_fault == faultType)
	    {
	    sigExcSend (pFTab->sigf_signo, code, pRegSet);
	    return;
	    }
	}

    /* not found in the table */

    if (pFTab->sigf_signo != 0)
        sigExcSend (pFTab->sigf_signo, code, pRegSet);

#if (CPU_FAMILY == SH)
    /* XXX  Here `pFTab' always points at the last member {0, 0}
     * XXX  in `_sigfaulttable[]'.  Therefore the above `sigExcSend()'
     * XXX  for any `not found in the table' case is useless.
     */
    {
    /* XXX  excBErrVecInit() initializes `excBErrVecNum', thus it cannot
     * XXX  be hard-coded in `_sigfaulttable[]'.  One idea is to modify
     * XXX  the above `for(;;)' looping condition and to have the
     * XXX  `not found in the table' case effective for a generic signal
     * XXX  catcher like {0, SIGBUS}.  However, {0, SIGILL} is defined
     * XXX  as a valid member for i86 architecture, so let's treat the
     * XXX  bus timeout error at outside of `_sigfaulttable[]':
     */
    IMPORT int excBErrVecNum;

    if ((excBErrVecNum != 0) && (faultType == excBErrVecNum))
	sigExcSend (SIGBUS, code, pRegSet);
    }
#endif
    }

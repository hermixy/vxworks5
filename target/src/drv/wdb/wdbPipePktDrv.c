/* wdbPipePktDrv.c - pipe packet driver for lightweight UDP/IP */

/* Copyright 1998-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01q,15apr02,hbh  Removed call to excJobAdd() to support System Mode Debugging
		 on Windows simulator (SPR# 23866) and fix SPR# 26247.
01p,21nov01,jmp  added intLock()/intUnlock() around NT system call.
01o,19oct01,hbh  Opened transmit pipe with L_NONBLOCK flag to avoid process
                 lock (SPR #34107).
		 Freed allocated buffer if excJobAdd() fails.	
		 Fixed passing of right # of parameters to excJobAdd(). 
		 Got rid of compilation warnings.
01n,16mar99,dbt  In wdbPipeInt() routine, call wdbPipeFlush() only if some
                 data have been received (SPR #25734).
01m,24nov98,cym  moving interrupt handling to task level
01l,06oct98,jmp  doc : cleanup.
01k,27aug98,fle  doc : fixed a typo in file header and removed dash
                 underscores in library description
01j,06aug98,pdm  used wd timer instead of poll task (SIMHPPA only)
01i,19may98,cym  changed forward for wdbPipeInt for SIMNT.
01h,19may98,dbt  test if the pipe is used by another simulator before using
		 it. 
		 Test if the kernel is initialized (with taskIdCurrent) 
		 before running intConnect() or taskCreate() routines.
		 Disable pipe interrupts when we enter polling mode.
		 Removed build warnings.
01g,27apr98,dbt  no longer unlink the pipe if it already exists.
		 added copyright.
01f,09apr98,pdn  used select while polling the fd so we don't block.
01e,08apr98,pdn  used poll mode for HP Sim.
01d,20mar98,cym  changed for vxSim NT.
01c,20mar98,pdn  fixed wdbPipeInt() to return if packet size can not be
                 determent.
01b,12mar98,pdn  fixed to work with system mode (poll.)
01a,11mar98,pdn  written.
*/

/*
DESCRIPTION

OVERVIEW
This module is a pipe for drivers interfacing with the WDB agent's
lightweight UDP/IP interpreter. It can be used as a starting point
when writing new drivers. Such drivers are the lightweight equivalent
of a network interface driver.

These drivers, along with the lightweight UDP-IP interpreter, have two
benefits over the stand combination of a netif driver + the full VxWorks
networking stack; First, they can run in a much smaller amout of target
memory because the lightweight UDP-IP interpreter is much smaller than
the VxWorks network stack (about 800 bytes total). Second, they provide
a communication path which is independant of the OS, and thus can be
used to support an external mode (e.g., monitor style) debug agent.

Throughout this file the word "pipe" is used in place of a real
driver name. For example, if you were writing a lightweight driver
for the lance ethernet chip, you would want to substitute "pipe"
with "ln" throughout this file.

PACKET READY CALLBACK
When the driver detects that a packet has arrived (either in its receiver
ISR or in its poll input routine), it invokes a callback to pass the
data to the debug agent. Right now the callback routine is called "udpRcv",
however other callbacks may be added in the future. 
The driver's wdbPipeDevInit() routine should be passed the callback as
a parameter and place it in the device data structure. That way the driver
will continue to work if new callbacks are added later.

MODES
Ideally the driver should support both polled and interrupt mode, and be
capable of switching modes dynamically. However this is not required.
When the agent is not running, the driver will be placed in "interrupt mode"
so that the agent can be activated as soon as a packet arrives.
If your driver does not support an interrupt mode, you can simulate this
mode by spawning a VxWorks task to poll the device at periodic intervals
and simulate a receiver ISR when a packet arrives.

For dynamically mode switchable drivers, be aware that the driver may be
asked to switch modes in the middle of its input ISR. A driver's input ISR
will look something like this:

.CS
   doSomeStuff();
   pPktDev->wdbDrvIf.stackRcv (pMbuf);   /@ invoke the callback @/
   doMoreStuff();
.CE

If this channel is used as a communication path to an external mode
debug agent, then the agent's callback will lock interrupts, switch
the device to polled mode, and use the device in polled mode for awhile.
Later on the agent will unlock interrupts, switch the device back to
interrupt mode, and return to the ISR.
In particular, the callback can cause two mode switches, first to polled mode
and then back to interrupt mode, before it returns.
This may require careful ordering of the callback within the interrupt
handler. For example, you may need to acknowledge the interrupt within
the doSomeStuff() processing rather than the doMoreStuff() processing.

USAGE
The driver is typically only called only from usrWdb.c. The only directly
callable routine in this module is wdbPipePktDevInit().
You will need to modify usrWdb.c to allow your driver to be initialized
by the debug agent.
You will want to modify usrWdb.c to include your driver's header
file, which should contain a definition of WDB_PIPE_PKT_MTU.
There is a default user-selectable macro called WDB_MTU, which must
be no larger than WDB_PIPE_PKT_MTU. Modify the begining of
usrWdb.c to insure that this is the case by copying the way
it is done for the other drivers.
The routine wdbCommIfInit() also needs to be modified so that if your
driver is selected as the WDB_COMM_TYPE, then your drivers init
routine will be called. Search usrWdb.c for the macro "WDB_COMM_CUSTOM"
and mimic that style of initialization for your driver.

DATA BUFFERING
The drivers only need to handle one input packet at a time because
the WDB protocol only supports one outstanding host-request at a time.
If multiple input packets arrive, the driver can simply drop them.
The driver then loans the input buffer to the WDB agent, and the agent
invokes a driver callback when it is done with the buffer.

For output, the agent will pass the driver a chain of mbufs, which
the driver must send as a packet. When it is done with the mbufs,
it calls wdbMbufChainFree() to free them.
The header file wdbMbuflib.h provides the calls for allocating, freeing,
and initializing mbufs for use with the lightweight UDP/IP interpreter.
It ultimatly makes calls to the routines wdbMbufAlloc and wdbMbufFree, which
are provided in source code in usrWdb.c.
*/

/* includes */

#include "vxWorks.h"

#if (CPU_FAMILY==SIMHPPA) || (CPU_FAMILY==SIMSPARCSOLARIS) || \
    (CPU_FAMILY==SIMNT)

#include "taskLib.h"
#include "wdLib.h"
#include <fcntl.h>
#include <stdio.h>
#include "string.h"
#include "errno.h"
#include "sioLib.h"
#include "intLib.h"
#include "iv.h"
#include "simLib.h"
#include "wdb/wdbMbufLib.h"
#include "drv/wdb/wdbPipePktDrv.h"
#if CPU != SIMNT
#include "u_Lib.h"
#endif	/* CPU != SIMNT */

/* externals */

#undef READ
#undef WRITE

#if CPU == SIMNT
extern void simMailSlotInit(void *inBuf,int *busyFlag, int *writeFd,
			    void (* intHandler)(),int intArg);
extern void simDelay (int);
extern int mailslotWrite (int hFile, char * szMessage, int len);
extern int mailslotConnect (char * szSlotName, int timeout);
extern int mailslotClose (int hSlot);
#define TGTSVR_MAIL_SLOT	"\\\\.\\mailslot\\tgtsvr.1"
extern int simLastMailSize;
extern int simInIntHandle;
int polledPackets = 0;
int pollingInt = 0;
#else /* CPU != SIMNT */
extern int UNLINK (char *);
extern int MKFIFO (char *, int);
extern int OPEN (char *, int);
extern int READ (int, char *, int);
extern int WRITE (int, char*, int);
extern int GETUID (void);
extern int SELECT (int, fd_set *, fd_set *, fd_set *, struct timeval *);
#endif	/*  CPU == SIMNT */ 

/* pseudo-macros */

#if CPU == SIMSPARCSOLARIS
#define DRIVER_MODE_SET(pDev, mode) wdbPipeDrvModeSet (pDev, mode)
#else /* CPU != SIMSPARCSOLARIS */
#define DRIVER_MODE_SET(pDev, mode)
#endif /* CPU == SIMSPARCSOLARIS */
#define DRIVER_RESET_INPUT(pDev) {pDev->inputBusy=FALSE;}
#define DRIVER_RESET_OUTPUT(pDev) {pDev->outputBusy=FALSE;}
#define DRIVER_POLL_TX(pDev, pBuf) {}
#define DRIVER_DROP_PACKET(pDev) {}

/* forward declarations */

LOCAL STATUS wdbPipePoll (void *pDev);
LOCAL STATUS wdbPipeTx   (void *pDev, struct mbuf * pMbuf);
LOCAL STATUS wdbPipeModeSet (void *pDev, uint_t newMode);
LOCAL void   wdbPipeFree (void *pDev);
#if CPU == SIMNT
LOCAL STATUS wdbPipeInt  (WDB_PIPE_PKT_DEV * pPktDev,int byteRead);
#else /* CPU != SIMNT */
LOCAL void   wdbPipeFlush (int pipeFd);
LOCAL STATUS wdbPipeInt  (WDB_PIPE_PKT_DEV * pPktDev);
#if CPU == SIMHPPA
LOCAL void   wdbPipeInputChk (WDB_PIPE_PKT_DEV * pPktDev);
LOCAL WDOG_ID wdPipeId = NULL;
#define WD_TIMEOUT 2
#else /* CPU != SIMHPPA */
LOCAL void  wdbPipeDrvModeSet (WDB_PIPE_PKT_DEV * pPktDev, uint_t newMode);
#endif /* CPU == SIMHPPA */
#endif	/* CPU == SIMNT */

/******************************************************************************
*
* wdbPipePktDevInit - initialize a pipe packet device.
*/

STATUS wdbPipePktDevInit
    (
    WDB_PIPE_PKT_DEV * pPktDev,	/* pipe device structure to init */
    void (*stackRcv)()		/* receive packet callback (udpRcv) */
    )
    {
#if CPU != SIMNT    
    char h2tPipe[64] = {0};
    char t2hPipe[64] = {0};
    int rc;
#else
    int lvl;
#endif /* CPU != SIMNT */

    /* initialize the wdbDrvIf field with driver info */

    pPktDev->wdbDrvIf.mode	= WDB_COMM_MODE_POLL | WDB_COMM_MODE_INT;
    pPktDev->wdbDrvIf.mtu	= WDB_PIPE_PKT_MTU;
    pPktDev->wdbDrvIf.stackRcv	= stackRcv;		/* udpRcv */
    pPktDev->wdbDrvIf.devId	= (WDB_PIPE_PKT_DEV *)pPktDev;
    pPktDev->wdbDrvIf.pollRtn	= wdbPipePoll;
    pPktDev->wdbDrvIf.pktTxRtn	= wdbPipeTx;
    pPktDev->wdbDrvIf.modeSetRtn = wdbPipeModeSet;

    /* initialize the device specific fields in the driver structure */

    pPktDev->inputBusy		= FALSE;
    pPktDev->outputBusy		= FALSE;

#if CPU != SIMNT
    {
    struct unix_stat buf;

    /*
     * Initialize the named pipe, and connect the driver interrupt
     * handler.
     */

    sprintf (h2tPipe, "/tmp/vxsim_h2t_%d", GETUID());
    sprintf (t2hPipe, "/tmp/vxsim_t2h_%d", GETUID());

    /* create the named pipes if they don't already exist */

    if (u_stat (h2tPipe, (char*)&buf) < 0)
	{
	rc = UNLINK (h2tPipe);
	rc = MKFIFO (h2tPipe, 0600);
	}

    if (u_stat (t2hPipe, (char*)&buf) < 0)
	{
	rc = UNLINK (t2hPipe);
	rc = MKFIFO (t2hPipe, 0600);
	}

    rc = pPktDev->h2t_fd = OPEN (h2tPipe, L_RDWR|L_NDELAY);

    /* test if the pipe is already in used by another simulator */

    if (u_lockf (rc, F_TLOCK, 0) != 0)
	{
	if (_func_printErr != NULL)
	    _func_printErr ("wdbPipePktDevInit: pipe is already used by another simulator\n");
	return (ERROR);
	}

    /* 
     * SPR 34107 : use NONBLOCK flag to avoid the simulator to lock on the
     * next write if fifo stream is full.
     */
     
    rc = pPktDev->t2h_fd = OPEN (t2hPipe, L_RDWR|L_NONBLOCK);

    /* flush reception pipe */

    wdbPipeFlush (pPktDev->h2t_fd);

    /* 
     * Connect interrupt or spawn the task (for HPPA) only if the OS is 
     * initialized.
     */

    if (taskIdCurrent)
	{
#if CPU == SIMHPPA
	/* named pipe on HP host does not generate SIGIO. */

        if (wdPipeId == NULL)
            wdPipeId = wdCreate();

        wdStart (wdPipeId, WD_TIMEOUT, (FUNCPTR) wdbPipeInputChk, (int)pPktDev);

#else /* CPU != SIMHPPA */
	/* setting up interrupt. */

	s_fdint (pPktDev->h2t_fd, 1);
	intConnect (FD_TO_IVEC (pPktDev->h2t_fd), 
				(void (*)())wdbPipeInt, (int)pPktDev);
#endif /* CPU == SIMHPPA */
	}
    }
#else /* CPU != SIMNT */
    /* Initialize the mailslot. */

    pPktDev->t2h_fd = 0;

    lvl = intLock();		/* lock windows system call */

    simMailSlotInit(pPktDev->inBuf,
		    &(pPktDev->inputBusy),
		    &(pPktDev->t2h_fd),
		    (void (*)())&wdbPipeInt, 
		    (int)pPktDev);
    
    intUnlock (lvl);		/* unlock windows system call */

#endif /* CPU != SIMNT */
    return (OK);
    }

#if CPU == SIMHPPA
/******************************************************************************
*
* wdbPipeInputChk - fake an input ISR when a packet arrives
*/

static void wdbPipeInputChk
    (
    WDB_PIPE_PKT_DEV * pDev
    )
    {
    int key;
    fd_set readFds;
    struct timeval timeout;

    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    FD_ZERO (&readFds);
    FD_SET (pDev->h2t_fd, &readFds);

    if (SELECT (FD_SETSIZE, &readFds, (fd_set *)NULL,
            (fd_set *)NULL, &timeout) > 0)
        {
        key = intLock();
        wdbPipeInt (pDev);
        intUnlock (key);
        }

    wdStart (wdPipeId, WD_TIMEOUT, (FUNCPTR) wdbPipeInputChk, (int)pDev);
    }
#endif /* CPU == SIMHPPA */

/******************************************************************************
*
* wdbPipeInt - pipe driver interrupt handler
*
* RETURNS: N/A
*/

static STATUS wdbPipeInt
    (
    WDB_PIPE_PKT_DEV * pPktDev
#if CPU==SIMNT
    ,
    int byteRead
#endif
    )
    {
#if CPU!=SIMNT
    int 	byteRead;
    short 	packetSize = 0;
    char 	szPacketSize [sizeof(short)] = {0};
#endif
    struct 	mbuf * pMbuf;

#if CPU==SIMNT
    if ( polledPackets && !pollingInt )
        {
        polledPackets--;
	return(ERROR);
        }
#endif
        
    /*
     * The code below is for handling a packet-recieved interrupt.
     * A real driver may also have to check for other interrupting
     * conditions such as DMA errors, etc.
     */

#if CPU != SIMNT	/* SIMNT Fake device does all of this */
    /* input buffer already in use - drop this packet */

    if (pPktDev->inputBusy)
	{
        wdbPipeFlush (pPktDev->h2t_fd);
        DRIVER_DROP_PACKET(pPktDev);
	return FALSE;
	}

    /* 
     * get the packet size
     */
    
    byteRead = READ (pPktDev->h2t_fd, szPacketSize, sizeof(short));

    if (byteRead < sizeof(short))
        {
	/*
	 * Flush the pipe only if we have received something. This is
	 * usefull for polling mode if a packet was received between
	 * READ() call and this test.
	 */
	    
	if (byteRead > 0)
	    wdbPipeFlush (pPktDev->h2t_fd);

        return FALSE;
        }

    packetSize = * (short *) szPacketSize;

    if (packetSize > WDB_PIPE_PKT_MTU) 
        {
        wdbPipeFlush (pPktDev->h2t_fd);
        return FALSE;
        }

    /*
     * Fill the input buffer with the packet. Use an mbuf cluster to pass
     * the packet on to the agent.
     */

    byteRead = READ (pPktDev->h2t_fd, pPktDev->inBuf, packetSize);

    if (byteRead != packetSize)
        {
        wdbPipeFlush (pPktDev->h2t_fd);
        return FALSE;
        }

#endif /* CPU != SIMNT */

    pMbuf = wdbMbufAlloc();
    if (pMbuf == NULL)
	{
	DRIVER_DROP_PACKET(pPktDev);
	return FALSE;
	}

    wdbMbufClusterInit (pMbuf, pPktDev->inBuf, byteRead, \
			(int (*)())wdbPipeFree, (int)pPktDev);

    pPktDev->inputBusy = TRUE;

    (*pPktDev->wdbDrvIf.stackRcv) (pMbuf);  /* invoke callback */

    return TRUE;
    }

/******************************************************************************
*
* wdbPipeTx - transmit a packet.
*
* The packet is realy a chain of mbufs. We may have to just queue up
* this packet is we are already transmitting.
*
* RETURNS: OK or ERROR
*/

static STATUS wdbPipeTx
    (
    void 	* pDev,
    struct mbuf * pMbuf
    )
    {
    WDB_PIPE_PKT_DEV * 	pPktDev = pDev;
    int 		lockKey;
    struct mbuf * 	pFirstMbuf = pMbuf;
    char 		data [WDB_PIPE_PKT_MTU + sizeof(short)];
    int           	len;
    int 		byteWritten = -1;

    /* if we are in polled mode, transmit the packet in a loop */

    if (pPktDev->mode == WDB_COMM_MODE_POLL)
	{
	DRIVER_POLL_TX(pPktDev, pMbuf);
	return (OK);
	}

    lockKey = intLock();

    /* if the txmitter isn't cranking, queue the packet and start txmitting */

    if (pPktDev->outputBusy == FALSE)
	{
	pPktDev->outputBusy = TRUE;
#if CPU==SIMNT	/* SIMNT doesn't need to prepend length */ 
	len = 0;
#else
        len = 2;
#endif

        while (pMbuf != NULL)
            {
            bcopy (mtod (pMbuf, char *), &data[len], pMbuf->m_len);
            len += pMbuf->m_len;
            pMbuf = pMbuf->m_next;
            }

        wdbMbufChainFree (pFirstMbuf);

#if CPU != SIMNT
        * (short *) &data [0]  = (short) len - 2; 
        byteWritten = WRITE (pPktDev->t2h_fd, data, len);
#else
        if(pPktDev->t2h_fd)
            {
            byteWritten = mailslotWrite (pPktDev->t2h_fd, data, len);
            }
        if(byteWritten == -2)
            {
            mailslotClose( pPktDev->t2h_fd );
            pPktDev->t2h_fd = mailslotConnect(TGTSVR_MAIL_SLOT,2);
            if (pPktDev->t2h_fd == -1 )
                {
	        pPktDev->outputBusy = FALSE;
                intUnlock (lockKey);
                return(ERROR);
                }
            else
                {
                byteWritten = mailslotWrite (pPktDev->t2h_fd, data, len);
                }
            }
#endif /* CPU != SIMNT */

        /* Write has failed, treat the error for all arch */

        if(byteWritten == -1)
            {
	    pPktDev->outputBusy = FALSE;
            intUnlock (lockKey);
            return(ERROR);
            }

	pPktDev->outputBusy = FALSE;
	}

    intUnlock (lockKey);
    return (OK);
    }

/******************************************************************************
*
* wdbPipeFree - free the input buffer
*
* This is the callback used to let us know the agent is done with the
* input buffer we loaded it.
*
* RETURNS: N/A
*/

static void wdbPipeFree
    (
    void *	pDev
    )
    {
    WDB_PIPE_PKT_DEV *	pPktDev = pDev;

    pPktDev->inputBusy = FALSE;
    }

/******************************************************************************
*
* wdbPipeModeSet - switch driver modes
*
* RETURNS: OK for a supported mode, else ERROR
*/

static STATUS wdbPipeModeSet
    (
    void *	pDev,
    uint_t	newMode
    )
    {
    WDB_PIPE_PKT_DEV * pPktDev = pDev;

    DRIVER_RESET_INPUT  (pPktDev);
    DRIVER_RESET_OUTPUT (pPktDev);

    if (newMode == WDB_COMM_MODE_INT) 
	DRIVER_MODE_SET (pPktDev, WDB_COMM_MODE_INT);
    else if (newMode == WDB_COMM_MODE_POLL)
	DRIVER_MODE_SET (pPktDev, WDB_COMM_MODE_POLL);
    else
	return (ERROR);

    return (OK);
    }

/******************************************************************************
*
* wdbPipePoll - poll for a packet
*
* This routine polls for a packet. If a packet has arrived it invokes
* the agents callback.
*
* RETURNS: OK if a packet has arrived, else ERROR.
*/ 

static STATUS wdbPipePoll
    (
    void *	pDev
    )
    {
    WDB_PIPE_PKT_DEV	* pPktDev = (WDB_PIPE_PKT_DEV *) pDev;
#if CPU == SIMNT
    if(pPktDev->inputBusy)
        {
        pollingInt = 1;
        polledPackets++;
	wdbPipeInt(pPktDev,simLastMailSize);
        pollingInt = 0;
        return( 0 );
        }

    simDelay(1);
    return(ERROR);
#else
    if (wdbPipeInt (pPktDev))
        return (OK);
    else 
        return (ERROR);
#endif /* CPU == SIMNT */
    }

#if CPU != SIMNT
#if CPU != SIMHPPA
/******************************************************************************
*
* wdbPipeDrvModeSet - switch driver modes
*
* RETURNS: N/A
*/

static void wdbPipeDrvModeSet
    (
    WDB_PIPE_PKT_DEV *	pPktDev,
    uint_t		newMode
    )
    {
    if (newMode == WDB_COMM_MODE_INT) 
	{
	s_fdint (pPktDev->h2t_fd, 1);

	/*
	 * XXX - DBT flush the pipe before entering interrupt mode. If the
	 * pipe is not empty, it won't generate any interrupt when a packet
	 * is received.
	 */

	wdbPipeFlush (pPktDev->h2t_fd);
	}
    else	/* poll mode */
	s_fdint (pPktDev->h2t_fd, 0);
    }
#endif	/* CPU != SIMHPPA */

/******************************************************************************
*
* wdbPipeFlush - flush pipe receive bufer
*/

static void wdbPipeFlush
    (
    int pipeFd		/* pipe file desc */
    )
    {
    char tmpBuf [WDB_PIPE_PKT_MTU];
    int  nBytes;

    do
	{
	nBytes = u_read (pipeFd, tmpBuf, WDB_PIPE_PKT_MTU);
	}
    while (nBytes > 0);
    }
#endif /* CPU != SIMNT */
#endif /* CPU_FAMILY==SIMHPPA || CPU_FAMILY==SIMSPARCSOLARIS || ... */

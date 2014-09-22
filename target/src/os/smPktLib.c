/* smPktLib.c - VxWorks shared packet protocol library */

/* Copyright 1984-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
02m,03may02,mas  cache flush and volatile fix (SPR 68334); bridge flush fix
		 (SPR 68844)
02l,24oct01,mas  doc update (SPR 71149)
02k,09oct01,mas  smPktSend: added error checking (SPR 6004), limited interrupt
		 retries (SPR 25341), removed all FAST modifiers, code cleanup
02j,17oct00,sn   replaced NULL with 0 so htonl works
02i,21feb99,jdi  doc: listed errnos.
02h,26aug93,kdl  smPktBroadcast() now checks smPktSllPut() status before 
		 interrupting destination CPU  (SPR #2441).
02g,13nov92,jcf  removed potential bus error during attach.
02f,02aug92,kdl  Uncommented include of "copyright_wrs.h".
02e,24jul92,elh  Moved heartbeat from anchor to header.
02d,23jun92,elh  general cleanup and documentation.
02c,02jun92,elh  the tree shuffle
02b,27may92,elh	 Made completely independant, changed to use
		 offsets, general cleanup.
02a,14may92,pme  Split to keep only packet passing routines.
01e,20feb92,elh  Added in ntohl, and htonl for 960, Added USE_OFFSET.
		 Extracted and renamed OS specific calls & moved to
		 smUtilLib.  Also removed function pointers
		 shMemIntGenFunc and shMemHwTasFunc.
		 made TAS_CHECKS 10 TAS_TRIES 5000.
		 modified parameters passed to intGen function.
01d,10feb92,kdl+ Changed shMemDetach to flush input queue.
	    elh  Removed references to OK_WAS{EMPTY, NT_EMPTY}.
		 Made shMemSend return an ERROR if packet too large.
		 Changed shMemSend to return silently (no
		 interrupts generated) if sending to self.
		 Misc code review changes.
01c,05feb92,elh  ansified.
01b,27jan92,elh	 added masterCpu to shared memory anchor.
		 changed shMemBeat to take pAnchor as argument.
		 changed arguments to shMemIsAlive.
		 changed shMemSetup to probe memory.
		 changed copyright.
01a,15aug90,kdl	 written.
*/

/*
DESCRIPTION

This library contains routines which allow multiple processors
to communicate over a backplane using shared memory.  All
services for initializing and managing the shared memory are
provided.

Data is sent between CPU's via shared memory "packets".  A packet
is simply a data buffer (of configurable size) with a header
that allows it to be properly identified and manipulated as a node
in a linked list of packets.


SHARED MEMORY MASTER CPU

One CPU node acts as the shared memory master.  This CPU initializes
the shared memory area and sets up the shared memory anchor.  These
steps are performed by the master calling the smPktSetup() routine.
This routine should be called only by the shared memory master CPU,
and it should only be called once for a given shared memory region.
(It is, however, possible to maintain and use multiple separate
shared memory regions.)

Once smPktSetup() has completed successfully, there is little functional
difference between the master CPU and other CPU's using shared memory,
except that the master is responsible for maintaining the heartbeat in
the shared memory packet region header.


SHARED MEMORY ANCHOR

The shared memory anchor is a small data structure which is at
a predetermined location, to allow all CPU's using shared memory to
find it.  The shared memory anchor contains the base offset to the
actual shared memory region which will be used for inter-processor
message passing.  (This allows the master CPU to dynamically allocate
the shared memory region.)

The shared memory anchor does not have to be defined as part of the
shared memory region which will be used for message passing.  However,
it must be located in a similar address space; the address translation
constants which CPU boards use to convert local addresses to bus addresses
must apply equally to the anchor and the regular shared memory region.


ATTACHING TO SHARED MEMORY

Each CPU, master or non-master, which will use the shared memory region
must attach itself to the shared memory.  The shared memory region must
have already been initialized by the master CPU calling smPktSetup().

The first step in attaching to shared memory is for each CPU to allocate and
initialize a shared memory packet descriptor (SM_PKT_DESC).  This structure 
describes the individual CPU's attachment to the shared memory region 
and is used in all subsequent shared memory calls to identify which 
shared memory region is being used.  Since the shared memory descriptor 
is used only by the local CPU, it is not necessary for the descriptor 
itself to be located in shared memory.  In fact, it is preferable for 
the descriptor to be allocated from the CPU's local memory, since local 
memory is usually more efficient to access.

The shared memory packet descriptor is initialized by calling the 
smPktInit() routine.  This routine takes a number of parameters which 
specify the characteristics of this CPU and its access to shared memory.

After the shared memory packet descriptor has been initialized, the CPU may
attach itself to the shared memory region.  This is done by calling the
smPktAttach() routine.

When smPktAttach() is called, it checks that the shared memory anchor 
contains the special ready value and that the heartbeat is incrementing.
If either of these conditions is not met, the routine will check 
periodically until either the ready value and incrementing heartbeat 
are recognized or a time limit is reached.  For non-master CPU's, 
this limit may be set by changing the global variable, smAliveTimeout.  
The limit is expressed in seconds, with 600 seconds (10 minutes) the 
default.  The master CPU will only wait 5 beat periods for a recognized 
heartbeat, since it is the CPU responsible for initializing shared memory 
and incrementing the heartbeat and therefore should not have to wait.  
If the time limit is reached before a valid ready value and heartbeat 
is seen, ERROR is returned and errno is set to S_smPktLib_DOWN .

Once the CPU has attached itself to the shared memory region, it may
send packets to other CPU's or receive packets which have been sent to it.
(Attempts to perform any shared memory operations without first attaching
successfully will return ERROR, with errno set to S_smPktLib_NOT_ATTACHED .)


SENDING PACKETS

To send a packet to another CPU, an application task must first obtain
a shared memory packet from the pool of free packets.  This is
done by calling smPktFreeGet(), as follows:
\cs
	status = smPktFreeGet (pSmPktDesc, &pPkt);
\ce
In this example, <pSmPktDesc> is the address of this CPU's shared memory packet
descriptor.  If status value returned by smPktFreeGet is OK, the address
of the obtained packet will be placed in <pPkt>.  If the packet address is
NULL, no free packets were available.

Once a packet has been obtained, the application task must copy the data
to be sent into the "data" field of the packet structure.  The maximum
number of bytes which may be copied to the packet's data buffer is
<maxPktBytes>, as specified by the master CPU during smPktSetup.  This
limit may be determined by reading a field in the shared memory packet 
descriptor.

The application task may set the "type" field in the packet header (SM_PKT_HDR)
to indicate the specific type of packet being sent.  This field is not used
by smPktLib and may therefore be given any value.

To send the completed packet, the application task calls smPktSend(),
as follows:
\cs
	status = smPktSend (pSmPktDesc, destCpu, pPkt);
\ce
Here, <destCpu> is the number of the destination CPU, and <pPkt> is the
address of the packet to be sent.  If smPktSend() returns a status of OK,
the packet was successfully queued for the destination CPU.

If the destination CPU did not previously have any input packets queued to
it smPktSend() will call the user-provided routine, smUtilIntGen(), to 
interrupt the destination CPU to notify it that a 
packet is available.  See "Interrupts," below, for more information.

If the destination CPU is not attached, ERROR is returned and errno is set
to S_smPktLib_DEST_NOT_ATTACHED .  If the specified destination cpu
number is out of range (i.e. less than zero or greater than the maximum
specified during smPktSetup), ERROR is returned and errno is set to
S_smPktLib_INVALID_CPU_NUMBER .

If the first interrupt fails, a delay of one tick is made and another
interrupt is attempted.  This is repeated up to smPktMaxIntRetries attempts or
until the interrupt is successful.  If not successful, ERROR is returned
and errno is set to S_smPktLib_INCOMPLETE_BROADCAST so that the caller
does not try to remove the packet and place it on the free list.  Doing
so would put the packet in both an input list and the free list!

BROADCAST PACKETS

In some circumstances, it may be desirable to send the same data to
all CPU nodes using the shared memory region.  This may be accomplished
by using a special "broadcast" mode of smPktSend().  This option sends
a copy of the same packet data to each attached CPU (except the sender).

To send a broadcast message, a CPU must first obtain a free shared memory
packet using smPktFreeGet() as usual.  The data area of the packet
is then filled with the message to be sent, again the same as for sending
to a single destination.

Broadcast mode is indicated during smPktSend() by a special value of the
<destCpu> (destination CPU) parameter.  This parameter is set to
SM_BROADCAST, rather than a particular CPU number.

When a broadcast message is sent, a separate packet is obtained for each
attached CPU, the data from the original packet is copied to it, and
the packet is queued to a particular CPU.  Therefore, there must be
sufficient free packets to send one to each CPU.  Broadcast packets are
sent to destination CPU's in cpu-number order.

If there are not enough free packets to send a copy to each CPU,
as many as possible will be sent, but ERROR is returned and errno is
set to S_smPktLib_INCOMPLETE_BROADCAST .  (If there are insufficient
free packets, the original packet passed during smPktSend() will be
sent to a destination CPU, to provide as complete a broadcast as
possible.)

Broadcast packets are received in the same manner as any other packets.


RECEIVING PACKETS

Packets are received by calling the smPktRecv() routine.  This routine
will normally be called by an interrupt handler, in response to a
an interrupt generated by a remote CPU sending a packet.  The smPktRecv()
routine may also be called periodically, in a polling fashion, to check
for received packets in systems which do not use interrupts to notify
the receiving CPU.

To receive a packet, smPktRecv is called as follows:
\cs
	status = smPktRecv (pSmPktDesc, &pPkt);
\ce
If the returned status is OK, <pPkt> will contain either the address of
a received packet, or NULL if no input packets were available.  A returned
status of ERROR indicates that an error occurred while attempting to
obtain received packets.

A sending CPU will interrupt the destination CPU only if there
were no packets previously queued for the destination CPU.  It is
therefore important that an interrupt handler which receives packets
call smPktRecv again after each packet is received, to check for
additional packets.

After a packet has been received, it must be explicitly freed by calling
smPktFreePut().


DETACHING FROM SHARED MEMORY

The attachment of a CPU to shared memory may be ended by calling
smPktDetach().  This routine will mark the calling CPU as no longer
attached.  After this, other CPU's may no longer send packets to it.
The CPU may re-attach itself to the shared memory region by later
calling smPktAttach().  When re-attaching, the original shared
memory descriptor may be re-used if the CPU's configuration remains
the same, or new values may be specified via smPktInit().


INTERRUPTS

When a packet is sent to a CPU, there must be some method for that CPU
to be informed that an input packet is available.  The preferred method
is for the sending CPU to be able to interrupt the receiving CPU.  This
will be highly dependent on the specific hardware being used.  

Two types of interrupts are supported, mailbox interrupts and vme
bus interrupts.  Mailbox interrupts are the first preferred
method (SM_INT_MAILBOX), followed by vme bus interrupts (SM_INT_BUS).
If interrupts cannot be used, a polling scheme may be employed
(SM_INT_NONE), but this is much less efficient.

When a CPU initailizes its shared memory packet descriptor via the
smPktInit() call, it passes in an interrupt type as well as three
interrupt arguments.  This describes how the cpu wishes to be notified
of incomming packets.  The interrupt types recognized by this library
are listed in smLib.h.  These values may be obtained for any attached CPU by
calling smPktCpuInfoGet().

The default interrupt method for a particular target is defined by the
configuration parameters: SM_INT_TYPE, SM_INT_ARG1, SM_INT_ARG2, SM_INT_ARG3 .

When a CPU sends a packet to a destination CPU which did not previously
have any input packets queued to it, the sending CPU will 
interrupt the destination CPU. 

A handler routine which executes in response to such an interrupt
must call smPktRecv() to obtain the input packet.  Since the interrupt
is generated only for the first packet queued when the input list was
previously empty, it is important that the handler routine then call
smPktRecv additional times until no more packets are available.

If it is not possible to use interrupts to notify receiving CPU's, a 
polling method may be used.  The simplest method is for the recieving CPU to
repeatedly call smPktRecv() to check for input packets.  In this case,
no notification routine is used.


OBTAINING STATUS INFORMATION

Two routines are provided to obtain current status information about
the shared memory region and individual CPU's:

The smPktInfoGet() routine gets status information which applies to the
the shared memory region as a whole.  It takes as a parameter a pointer
to a special information structure (SM_PKT_INFO), which it fills before
returning.  

The smPktCpuInfoGet() routine obtains status information for a single
CPU.  The number of the CPU must be specified during the call to
smPktCpuInfoGet().  A CPU number of NONE (-1) indicates that information
on the calling CPU should be returned.  The routine takes as a parameter
a pointer to a special CPU information structure (SM_PKT_CPU_INFO), 
which it fills before returning.  

INTERNAL

This file runs under only vxWorks.  SunOS is no longer supported.

Pointers into shared memory are declared 'volatile' so that compiler
optimization does not disrupt ordering of I/O operations.

Reading of locking semaphores after updates to shared memory but before
release of the semaphores is to cause the flushing of any external bus
bridge read and write FIFOs.  Failure to do so can result in erroneous
reads of shared memory and subsequent deadlock conditions.
*/

#ifdef UNIX
#undef INET
#endif

/* includes */

#include "vxWorks.h"
#include "sysLib.h"
#include "taskLib.h"
#include "cacheLib.h"
#include "smPktLib.h"
#include "smLib.h"
#include "smUtilLib.h"

/* defines */

#define	SM_LOCK_TAKE(lockLocalAdrs,tasRoutine,numTries,pOldLvl) \
		(smLockTake (lockLocalAdrs, tasRoutine, numTries, pOldLvl))

#define	SM_LOCK_GIVE(lockLocalAdrs,tasClearRoutine, oldLvl) \
		(smLockGive (lockLocalAdrs, tasClearRoutine, oldLvl))

#define DEFAULT_TAS_TRIES	5000	/* default tries for test-and-set */

#ifndef SM_PKT_MAX_INT_RETRIES
# define SM_PKT_MAX_INT_RETRIES 5   /* max #retries of interrupt per pkt */
#endif

/* Globals */

int	smPktMemSizeDefault 	= DEFAULT_MEM_SIZE;	/* memory size */
int 	smPktMaxBytesDefault 	= DEFAULT_PKT_SIZE; 	/* max pkt size */
int 	smPktMaxInputDefault	= DEFAULT_PKTS_MAX;	/* max input pkts */
int 	smPktMaxCpusDefault     = DEFAULT_CPUS_MAX;	/* max num CPU's */
int	smPktTasTries 	  	= DEFAULT_TAS_TRIES;
				/* times to try test-and-set */
int     smPktMaxIntRetries      = SM_PKT_MAX_INT_RETRIES;
                                /* max #int retries to attempt per packet */

/* Forward References */

#ifdef __STDC__

LOCAL STATUS smPktSllGet
    (
    SM_SLL_LIST volatile * listLocalAdrs,	/* local addr of packet list */
    int                    baseAddr,		/* addr conversion constant */
    FUNCPTR                tasRoutine,		/* test-and-set routine addr */
    FUNCPTR                tasClearRoutine,	/* test-and-set routine addr */
    SM_SLL_NODE **	   pNodeLocalAdrs	/* location to put node addr */
    );

LOCAL STATUS smPktSllPut
    (
    SM_SLL_LIST volatile * listLocalAdrs,	/* local addr of packet list */
    int			   base,		/* base address */
    FUNCPTR		   tasRoutine,		/* test-and-set routine addr */
    FUNCPTR		   tasClearRoutine,	/* test-and-set routine addr */
    SM_SLL_NODE volatile * nodeLocalAdrs,	/* local addr of node */
    BOOL *		   pListWasEmpty	/* set to true if adding to
                                                 * empty list */
    );

LOCAL STATUS smPktBroadcast
    (
    SM_PKT_DESC *	pSmPktDesc,	/* sh mem pkt descriptor */
    volatile SM_PKT *	pPktOrig	/* ptr to original packet */
    );

#else	/* __STDC__ */

LOCAL STATUS smPktSllGet ();
LOCAL STATUS smPktSllPut ();
LOCAL STATUS smPktBroadcast ();

#endif	/* __STDC__ */

/******************************************************************************
*
* smPktSetup - set up shared memory (master CPU only)
*
* This routine should be called only by the master CPU using shared
* memory.  It initializes the specified memory area for use by the
* shared memory protocol.
*
* After the shared memory has been initialized, this and other CPU's
* may initialize a shared memory packet descriptor to it, using smPktInit(),
* and then attach to the shared memory area, using smPktAttach().
*
* The <anchorLocalAdrs> parameter is the memory address by which the master
* CPU accesses the shared memory anchor.
*
* The <smLocalAdrs> parameter is the memory address by which the master
* CPU accesses the actual shared memory region to be initialized.
*
* The <smSize> parameter is the size, in bytes, of the shared memory
* region.
*
* The shared memory routines must be able to obtain exclusive access to
* shared data structures.  To allow this, a test-and-set operation is
* used.  It is preferable to use a genuine test-and-set instruction, if
* the hardware being used permits this.  If this is not possible, smUtilLib
* provides a software emulation of test-and-set.  The <tasType> parameter
* specifies what method of test-and-set is to be used.
*
* The <maxCpus> parameter specifies the maximum number of CPU's which may
* use the shared memory region.
*
* The <maxPktBytes> parameter specifies the size, in bytes, of the data
* buffer in shared memory packets.  This is the largest amount of data
* which may be sent in a single packet.  If this value is not an exact
* multiple of 4 bytes, it will be rounded up to the next multiple of 4.
*
* INTERNAL
* The first item in the shared memory area is the shared memory packet
* header (SM_PKT_MEM_HDR).  Following this is an array of CPU 
* descriptors (SM_PKT_CPU_DESC); this table contains one CPU descriptor 
* for each possible CPU, as specified by <maxCpus>.
* The shared memory area following the cpu table is allocated to the global
* list of free packets (SM_PKT), used for sending data between CPU's.  Note
* that the shared memory anchor is NOT part of the regular shared memory area.
*
* Since the size of each data packet is not pre-determined, the actual size
* is calculated based on the size of the (fixed) packet header and the
* data buffer size, specified by <maxPktBytes>.
*
* The standard smPktSllPut routine is used to build the free packet list.
* The software test-and-set routine is always used, regardless of the
* definition of <tasType>, because no hardware TAS routine has been supplied
* yet. (That is done during smPktAttach.)  Use of any TAS method at this
* stage is mainly a formality, since no other CPU's are able to attach to
* the shared memory region.
*
* RETURNS: OK, or ERROR.
*
* ERRNO: S_smPktLib_SHARED_MEM_TOO_SMALL, S_smPktLib_MEMORY_ERROR
*
* INTERNAL:  This routine and smObjSetup can not be called at the same time!!!
*/

STATUS smPktSetup
    (
    SM_ANCHOR * anchorLocalAdrs, 	/* local addr of anchor */
    char * 	smLocalAdrs,		/* local addr of sh mem area */
    int		smSize,			/* size of shared memory area */
    int		tasType, 		/* test and set type */
    int		maxCpus,		/* max number of cpus */
    int		maxPktBytes		/* max bytes of packet data */
    )
    {
    SM_ANCHOR volatile * pAnchorv = (SM_ANCHOR volatile *) anchorLocalAdrs;
    SM_PKT_MEM_HDR volatile *	smPktHdr;	/* local addr of sh mem hdr */
    int				pktSize;	/* actual size of pkt */
    STATUS			status;	 	/* return status */
    SM_PKT volatile *		pPkt;		/* local addr of free packet */
    int				memLeft;
    char			temp = 0;
    int				base = (int) anchorLocalAdrs;
    int 			bytesUsed = 0;
    int                         tmp;            /* temp storage */

    /* set up default values for parameters */

    smSize 	= (smSize == 0)  ?  smPktMemSizeDefault : smSize;
    maxCpus	= (maxCpus == 0) ?  smPktMaxCpusDefault : maxCpus;
    maxPktBytes = (maxPktBytes == 0) ?  smPktMaxBytesDefault : maxPktBytes;

    maxPktBytes = ((maxPktBytes + sizeof (int) - 1) / sizeof (int)) *
                  sizeof (int);                 /* round up buf to int mult */

    pktSize = (sizeof (SM_PKT_HDR) + maxPktBytes);
                                                /* pkt size incl data buffer */

    if (smSetup (anchorLocalAdrs, smLocalAdrs, tasType, maxCpus,
		 &bytesUsed) == ERROR)
       return (ERROR);

    smSize 	-= bytesUsed;
    smLocalAdrs += bytesUsed;

    /*
     * Check that shared mem size is big enough to contain:
     * the shared memory packet header, the shared memory packet
     * cpu descriptors and 1 pkt per cpu.
     */

    if (smSize < (sizeof (SM_PKT_MEM_HDR) +
	(maxCpus * sizeof (SM_PKT_CPU_DESC)) + (maxCpus * pktSize)))
        {
        errno = S_smPktLib_SHARED_MEM_TOO_SMALL;
        return (ERROR);                         /* not enough sh mem */
        }

    /* Probe beginning and end of shared memory */

    if ((smUtilMemProbe (smLocalAdrs , VX_WRITE, sizeof (char), &temp) != OK) ||
        (smUtilMemProbe (smLocalAdrs + smSize - 1, VX_WRITE, sizeof (char),
			 &temp) != OK))
        {
        errno = S_smPktLib_MEMORY_ERROR;
        return (ERROR);
        }

    /* Clear shared memory */

    bzero (smLocalAdrs, smSize);

    /* Fill in header */

    smPktHdr = (SM_PKT_MEM_HDR volatile *) smLocalAdrs;
    smPktHdr->maxPktBytes = htonl (maxPktBytes);/* max size of pkt data buf */
    smPktHdr->pktCpuTbl   = htonl (SM_LOCAL_TO_OFFSET ((char *) smPktHdr +
				   		       sizeof (SM_PKT_MEM_HDR),
						       base));

    pAnchorv->smPktHeader = htonl (SM_LOCAL_TO_OFFSET (smLocalAdrs, base));

    /* Set up list of free packets */

    pPkt = (SM_PKT *) (smLocalAdrs + sizeof (SM_PKT_MEM_HDR) +
		      (maxCpus * sizeof (SM_PKT_CPU_DESC)));

						/* calculate addr of 1st pkt */

    memLeft = smSize - ((int) ((char *) pPkt - smLocalAdrs));
						/* calculate remaining sh mem */

    while (memLeft >= pktSize)
	{
	/* allow one more in free list*/

	smPktHdr->freeList.limit = htonl (ntohl (smPktHdr->freeList.limit) +1);

	/* Add packet to list, always use software test-and-set emulation */

	status = smPktSllPut (&(smPktHdr->freeList), base, smUtilSoftTas, NULL,
			      &(pPkt->header.node), NULL);

	if (status == ERROR)
	    {
	    smPktHdr->freeList.limit = htonl (ntohl (smPktHdr->freeList.limit)
	                                      - 1);
	    return (ERROR);			/* error adding to free list */
	    }

	pPkt = (SM_PKT *) ((char *) pPkt + pktSize);/* advance pkt pointer */
	memLeft -= pktSize;			/* decrease mem remaining */
	}

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    tmp = smPktHdr->freeList.limit;             /* BRIDGE FLUSH  [SPR 68334] */

    return (OK);				/* shared mem init complete */
    }

/******************************************************************************
*
* smPktInit - initialize shared memory packet descriptor
*
* This routine initializes a shared memory packet descriptor.  The descriptor
* must have been previously allocated (generally in the CPU's local
* memory).  Once the descriptor has been initialized by this routine,
* the CPU may attach itself to the shared memory area by calling
* smPktAttach().
*
* Only the shared memory descriptor itself is modified by this routine.
* No structures in shared memory are affected.
*
* The <pSmPktDesc> paramter is the address of the shared memory packet 
* descriptor which is to be initialized; this structure must have 
* already been allocated before smPktInit is called.
*
* The <anchorLocalAdrs> parameter is the memory address by which the local
* CPU may access the shared memory anchor.  This address may vary for
* different CPU's because of address offsets (particularly if the anchor is
* located in one CPU's dual-ported memory).
*
* The <maxInputPkts> parameter specifies the maximum number of incoming
* shared memory packets which may be queued to this CPU at one time.  If
* a remote CPU attempts to send more packets after this limit is reached,
* an error will be returned to the remote CPU.
*
* The <ticksPerBeat> parameter specifies the frequency of the shared memory
* heartbeat.  The frequency is expressed in terms of how many CPU
* ticks on the local CPU correspond to one heartbeat period.
*
* The <intType>, <intArg1>, <intArg2>, and <intArg3> parameters allow a
* CPU to announce the method by which it is to be notified of input packets
* which have been queued to it.  Once this CPU has attached to the shared
* memory region, other CPU's will be able to determine these interrupt
* parameters by calling smPktCpuInfoGet().  The following interrupt 
* methods are currently recognized by this library: SM_INT_MAILBOX, 
* SM_INT_BUS, SM_INT_NONE, and SM_INT_USER .
*
* RETURNS: N/A
*/

void smPktInit
    (
    SM_PKT_DESC *	pSmPktDesc,         /* ptr to sh mem packet descr */
    SM_ANCHOR *		anchorLocalAdrs,    /* local addr of sh mem anchor*/
    int                 maxInputPkts,       /* max queued packets allowed */
    int                 ticksPerBeat,       /* cpu ticks per heartbeat */
    int                 intType,            /* interrupt method */
    int                 intArg1,            /* interrupt argument #1 */
    int                 intArg2,            /* interrupt argument #2 */
    int                 intArg3             /* interrupt argument #3 */
    )
    {
    if (pSmPktDesc == NULL)
        return;                                 /* don't use null ptr */

    bzero ((char *) pSmPktDesc, sizeof (SM_PKT_DESC));

    smInit (&pSmPktDesc->smDesc, anchorLocalAdrs, ticksPerBeat, intType,
	    intArg1, intArg2, intArg3);

    pSmPktDesc->maxInputPkts  = (maxInputPkts == 0) ? smPktMaxInputDefault :
				   		      maxInputPkts;
    pSmPktDesc->status        = SM_CPU_NOT_ATTACHED;    /* initial status */
    }

/******************************************************************************
*
* smPktAttach - attach to shared memory
*
* This routine "attaches" the local CPU to a shared memory area.  The
* shared memory area is identified by the shared memory packet descriptor
* whose address specified by <pSmPktDesc>.  The descriptor must have
* already been initialized by calling smPktInit().
*
* This routine will complete the attach process only if and when the
* shared memory has been initialized by the master CPU.  To determine
* this, the shared memory anchor is checked for the proper value
* (SM_READY_VALUE) in the anchor's ready-value field and a check is made for
* an active heartbeat.  This repeated checking continues until either 
* the ready-value and heartbeat have been verified or a timeout limit 
* is reached.  If the shared memory is not recognized as active within 
* the timeout period, this routine returns an error (S_smPktLib_DOWN).
*
* The attachment to shared memory may be ended by calling smPktDetach().
*
* RETURNS: OK, or ERROR.
*
* ERRNO: S_smPktLib_DOWN
*
* SEE ALSO: smPktInit()
*/

STATUS smPktAttach
    (
    SM_PKT_DESC	*	pSmPktDesc	/* packet descriptor */
    )
    {
    SM_PKT_MEM_HDR volatile *	pHdr;		/* sm pkt header */
    SM_PKT_CPU_DESC volatile *	pPktDesc;	/* pkt cpu descriptor */
    int				cpuNum;		/* this cpu's number */
    SM_ANCHOR volatile *	pAnchor;	/* anchor */
    int				beatsToWait;
    SM_DESC *			pSmDesc = (SM_DESC *) &pSmPktDesc->smDesc;

    cpuNum  = pSmDesc->cpuNum;
    pAnchor = pSmDesc->anchorLocalAdrs;

    /* Check that shared memory is initialized and running */

    /*
     * XXX master CPU should only wait DEFAULT_BEATS_TO_WAIT but we don't
     * know who the master is unless we look in the anchor.  The anchor may
     * not be mapped onto the bus, and we will blow up with a BERR if we
     * poke around.  So we just wait a long time, even if we are the master.
     * This could be fixed by listing the SM master and local processor
     * numbers in the packet descriptor so that the two could be compared.
     * If equal, we would be the master and no waiting would be necessary.
     */

    beatsToWait = DEFAULT_ALIVE_TIMEOUT;

    if (smIsAlive ((SM_ANCHOR *)pAnchor, (int *)&(pAnchor->smPktHeader),
                   pSmDesc->base, beatsToWait, pSmDesc->ticksPerBeat) == FALSE)
        {
        errno = S_smPktLib_DOWN;
        return (ERROR);                         /* sh memory not active */
        }

    if (smAttach (pSmDesc) == ERROR)
	return (ERROR);

    /* Get local address for shared mem packet header */

    pHdr = SM_OFFSET_TO_LOCAL (ntohl (pAnchor->smPktHeader), pSmDesc->base, 
			       SM_PKT_MEM_HDR volatile *);

    pSmPktDesc->hdrLocalAdrs = (SM_PKT_MEM_HDR *) pHdr;
    pSmPktDesc->maxPktBytes  = ntohl (pHdr->maxPktBytes);
    pSmPktDesc->cpuLocalAdrs = SM_OFFSET_TO_LOCAL (ntohl (pHdr->pktCpuTbl),
				 	    	   pSmDesc->base,
					    	   SM_PKT_CPU_DESC *);

    pPktDesc = &((pSmPktDesc->cpuLocalAdrs) [cpuNum]);
						/* calculate addr of cpu desc
						 * in global table.
						 */
    pPktDesc->inputList.limit = htonl (pSmPktDesc->maxInputPkts);
						/* max queued count */

    pPktDesc->status  = htonl (SM_CPU_ATTACHED);/* mark this cpu as attached */
    pSmPktDesc->status = SM_CPU_ATTACHED;	/* also mark sh mem descr */

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    cpuNum = pPktDesc->status;			/* BRIDGE FLUSH  [SPR 68334] */

    return (OK);				/* attach complete */
    }

/******************************************************************************
*
* smPktFreeGet - get a shared memory packet from free list
*
* This routine obtains a shared memory packet.  The packet is taken
* from the global free packet list.  No initialization of the packet
* contents is performed.
*
* The address of the obtained packet is placed in the location specified
* by <ppPkt>.  If there were no free packets available, this value will
* be NULL.
*
* RETURNS: OK, or ERROR.
*
* ERRNO: S_smPktLib_NOT_ATTACHED
*/

STATUS smPktFreeGet
    (
    SM_PKT_DESC *	pSmPktDesc,	/* ptr to shared memory descriptor */
    SM_PKT **		ppPkt 		/* location to put packet address */
    )
    {
    SM_DESC *	pSmDesc = &pSmPktDesc->smDesc;

    /* Check that this cpu is connected to shared memory */

    if (pSmPktDesc->status != SM_CPU_ATTACHED)
	{
	errno = S_smPktLib_NOT_ATTACHED;
	return (ERROR);				/* local cpu is not attached */
	}

    /* Get packet from free list */

    return (smPktSllGet (&(pSmPktDesc->hdrLocalAdrs->freeList),
			 pSmDesc->base, pSmDesc->tasRoutine,
			 pSmDesc->tasClearRoutine, (SM_SLL_NODE **) ppPkt));
    }

/******************************************************************************
*
* smPktSend - send a packet via shared memory
*
* This routine queues a packet (previously acquired via smPktFreeGet)
* to be received by another CPU.  If the input list for the destination
* CPU was previously empty and the destination has not specified polling 
* as its interrupt type, this routine will interrupt the destination CPU. 
*
* If the specified <destCpu> is SM_BROADCAST, a copy of the packet will
* be sent to each CPU which is attached to the shared memory area (except
* the sender CPU).  If there are not enough free packets to send a copy
* to each cpu, or if errors occur when sending to one or more destinations,
* an error (S_smPktLib_INCOMPLETE_BROADCAST) is returned.
*
* If ERROR is returned and errno equals S_smPktLib_INCOMPLETE_BROADCAST,
* the original packet, <pPkt>, was sent to an attached CPU and does not
* have to be freed.  A return code of ERROR combined with any other value
* of errno indicates that <pPkt> was NOT sent and should be explicitly
* freed using smPktFreePut().  (A return value of OK indicates that
* <pPkt> was either sent or freed before this routine returned, and no
* further action is required.)
*
* RETURNS: OK, or ERROR.
*
* ERRNO: S_smPktLib_NOT_ATTACHED, S_smPktLib_INVALID_PACKET,
* S_smPktLib_PACKET_TOO_BIG, S_smPktLib_INVALID_CPU_NUMBER,
* S_smPktLib_DEST_NOT_ATTACHED, S_smPktLib_INCOMPLETE_BROADCAST
* 
*/

STATUS smPktSend
    (
    SM_PKT_DESC *	pSmPktDesc,	/* ptr to shared memory descriptor */
    SM_PKT *		pPkt,		/* local addr of packet to be sent */
    int			destCpu		/* destination cpu number */
    )
    {
    SM_PKT volatile *		pPktv = (SM_PKT volatile *) pPkt;
    STATUS			status;		/* return status */
    SM_PKT_CPU_DESC volatile *	pPktCpuDesc;	/* destination cpu pkt descr*/
    SM_CPU_DESC volatile *	pCpuDesc;	/* destination cpu descr*/
    int				i;
    BOOL			listWasEmpty;	/* list empty */
    SM_DESC *			pSmDesc = (SM_DESC *) &pSmPktDesc->smDesc;
    int                         tmp;            /* temp storage */

    /* Check that this cpu is connected to shared memory */

    if (pSmPktDesc->status != SM_CPU_ATTACHED)
	{
	errno = S_smPktLib_NOT_ATTACHED;
	return (ERROR);				/* local cpu is not attached */
	}

    /* Check for null pointer */

    if (pPkt == NULL)
	{
	errno = S_smPktLib_INVALID_PACKET;
	return (ERROR);				/* null packet address */
	}

    /* Enforce max data length */

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    tmp = pPktv->header.nBytes;                 /* BRIDGE FLUSH  [SPR 68334] */

    if (pPktv->header.nBytes > pSmPktDesc->maxPktBytes)
	{
	errno = S_smPktLib_PACKET_TOO_BIG;
	return (ERROR);				/* too much data ! */
	}
						/* byte swap info */
    pPktv->header.nBytes = htonl (pPktv->header.nBytes);

    /* Call special routine if doing broadcast */

    if (destCpu == SM_BROADCAST)		/* if sending to all cpu's */
	return (smPktBroadcast (pSmPktDesc, pPkt));


    /* Make sure destination cpu is valid and attached */

    if (destCpu < 0  ||  destCpu >= pSmDesc->maxCpus)
        {
        errno = S_smPktLib_INVALID_CPU_NUMBER;
        return (ERROR);				/* cpu number out of range */
        }

    pPktCpuDesc = &((pSmPktDesc->cpuLocalAdrs) [destCpu]);
    pCpuDesc    = &((pSmDesc->cpuTblLocalAdrs) [destCpu]);
    						/* get addr of cpu descriptor */

    if (ntohl (pPktCpuDesc->status) != SM_CPU_ATTACHED)
        {
        errno = S_smPktLib_DEST_NOT_ATTACHED;
        return (ERROR);				/* dest cpu is not attached */
        }

    /* Complete packet header info */

    pPktv->header.srcCpu   = htonl (pSmDesc->cpuNum);	/* source cpu */

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    i = pPktv->header.srcCpu;			/* BRIDGE FLUSH  [SPR 68334] */

    /* Add this packet to destination cpu's input list */

    status = smPktSllPut (&(pPktCpuDesc->inputList), pSmDesc->base,
			  pSmDesc->tasRoutine, pSmDesc->tasClearRoutine, 
			  &(pPktv->header.node), &listWasEmpty);

    if ((status == OK) && (listWasEmpty) && (destCpu != pSmDesc->cpuNum))
        {
	/* Interrupt destination cpu */

	if ((status = smUtilIntGen ((SM_CPU_DESC *)pCpuDesc, destCpu)) != OK)
            {
            /*
             * Packet put in input list but interrupt failed.
             * This is not an error.  It means another interrupt will have
             * to be sent after a short delay.  This will be repeated until
             * the interrupt is successful or a timeout occurs.
             * (See SPR 25341, 33771)
             */

            for (i = smPktMaxIntRetries; (status != OK) && (i > 0); --i)
                {
                taskDelay (1);
                status = smUtilIntGen ((SM_CPU_DESC *)pCpuDesc, destCpu);
                }

            /* time out on interrupt generation */

            if (status != OK)
                {
                /* packet still in input list so do NOT delete it */

                errno = S_smPktLib_INCOMPLETE_BROADCAST;
                }
            }
        }

    return (status);
    }


/******************************************************************************
*
* smPktBroadcast - send packet data to all attached CPU's via shared memory
*
* This sends a copy of the packet specified by <pPkt> to all CPU's attached
* to the specified shared memory area (except the sending CPU).
*
* This routine attempts to obtain a new packet for each destination CPU,
* copying the packet contents to each newly-acquired packet.  If no new
* packets are available, this routine will go ahead and send the original
* packet to the next destination CPU.  If there are still more destinations
* after the original has been sent, an error (S_smPktLib_INCOMPLETE_BROADCAST)
* is returned.  This same error is returned if one or more destinations did
* not receive the broadcast due to other problems (e.g. the input queue for
* a destination CPU was full).
*
* RETURNS: OK, or ERROR.
*
* ERRNO: S_smPktLib_INCOMPLETE_BROADCAST
*/

LOCAL STATUS smPktBroadcast
    (
    SM_PKT_DESC *	pSmPktDesc,	/* sh mem pkt descriptor */
    SM_PKT volatile *	pPktOrig	/* ptr to original packet */
    )
    {
    STATUS			status;		/* return status */
    SM_PKT volatile *		pPkt;		/* ptr to packet being sent */
    SM_PKT volatile *		pPktNew;	/* ptr to newly obtained pkt */
    int				destCpu;	/* destination cpu number */
    SM_CPU_DESC volatile *	pCpuDesc;	/* destination cpu descriptor*/
    SM_PKT_CPU_DESC volatile *	pPktCpuDesc;	/* destination cpu descriptor*/
    BOOL			destMissed;	/* TRUE if a dest was missed */
    BOOL			listWasEmpty;	/* list added to was empty */
    SM_DESC *			pSmDesc = (SM_DESC *) &pSmPktDesc->smDesc;

    pCpuDesc = pSmDesc->cpuTblLocalAdrs;
    pPktCpuDesc = pSmPktDesc->cpuLocalAdrs;
    destMissed = FALSE;				/* no destinations missed yet */

    /* Send copy of packet to each attached cpu (except self) */

    for (destCpu = 0;  destCpu < pSmDesc->maxCpus;  destCpu++)
        {
        if ((ntohl (pPktCpuDesc->status) == SM_CPU_ATTACHED)  &&
	    (destCpu != pSmDesc->cpuNum))
    	    {
    	    if (pPktOrig == NULL)		/* if original already used */
    	    	{
    	    	destMissed = TRUE;		/* weren't enough free packets*/
    	    	break;				/* can't do any more */
    	    	}

	    /* Get new packet */

    	    if (smPktFreeGet (pSmPktDesc, (SM_PKT **)&pPktNew) != OK)
    	    	return (ERROR);			/* error getting packet */

    	    if (pPktNew != NULL)		/* if a new pkt was obtained */
    	    	{
		pPkt = pPktNew;			/* it is the packet to send */

    	    	pPkt->header.type   = pPktOrig->header.type;
						/* copy packet type */
    	    	pPkt->header.nBytes = pPktOrig->header.nBytes;
						/* copy byte count */
    	    	bcopy ((char *)pPktOrig->data, (char *)pPkt->data,
    	    	       ntohl (pPkt->header.nBytes));
    						/* copy packet data */
    	    	}
    	    else				/* if could not get new pkt */
    	    	{
    	    	pPkt = pPktOrig;		/*  set up to send original */
    	    	}

    	    pPkt->header.srcCpu  = htonl (pSmDesc->cpuNum);
						/* source cpu number */
    	    /* Send packet */

            status = smPktSllPut (&(pPktCpuDesc->inputList), pSmDesc->base, 
				  pSmDesc->tasRoutine, pSmDesc->tasClearRoutine,
    			          &(pPkt->header.node), &listWasEmpty);

            if ((status == OK) && listWasEmpty)	/* if list previously empty */
        	{
		/* Interrupt destination CPU */

        	if (smUtilIntGen ((SM_CPU_DESC *)pCpuDesc, destCpu) != OK)
		    {
		    destMissed = TRUE;	/* a destination was missed */
		    }
        	}
    	    else if (status == ERROR)		/* if error sending pkt */
		{
		/*
		 *  NOTE: We do not return just because an error occurred.
		 *        The destination cpu's input list may simply have been
		 *        full, or some other error may have occurred which
		 *	  will not affect the remaining destinations.  When
		 *	  this routine ultimately returns, the broadcast will
		 *	  be reported as having been incomplete.
		 */

		destMissed = TRUE;		/* a destination was missed */

		if (pPkt != pPktOrig)		/* if pkt was not the original*/
        	    {
    	    	    /* New packet not sent - return to free pool */

        	    if (smPktFreePut (pSmPktDesc, (SM_PKT *)pPkt) != OK)
    		    	return (ERROR);		/* could not return packet */
		    }
    	    	}


    	    if (pPkt == pPktOrig  &&  status != ERROR)
    	    	{				/* if original actually sent */
    	    	pPktOrig = NULL;		/*  clear pointer to orig pkt */
    	    	}

    	    }  /* end if (attached) */


        pCpuDesc++;				/* next cpu descriptor */
	pPktCpuDesc++;

        }  /* end for */


    /* Free original packet if it wasn't sent during broadcast */

    if (pPktOrig != NULL)
        {
        if (smPktFreePut (pSmPktDesc, (SM_PKT *)pPktOrig) != OK)
    	    return (ERROR);			/* could not free packet */
        }


    if (destMissed)				/* if a destination was missed*/
	{
    	errno = S_smPktLib_INCOMPLETE_BROADCAST;
    	return (ERROR);				/* broadcast not complete */
    	}
    else
	{
    	return (OK);
	}
    }

/******************************************************************************
*
* smPktFreePut - return a shared memory packet to free list
*
* This routine frees a shared memory packet.  The packet is placed
* on the global free packet list.  No modification of the packet
* contents is performed.
*
* The address of the packet to be returned is specified in <pPkt>.
*
* RETURNS: OK, or ERROR.
*
* ERRNO: S_smPktLib_NOT_ATTACHED, S_smPktLib_INVALID_PACKET
*/

STATUS smPktFreePut
    (
    SM_PKT_DESC	*	pSmPktDesc,	/* ptr to shared memory descriptor */
    SM_PKT *		pPkt		/* addr of packet to be returned */
    )
    {
    SM_DESC *	pSmDesc = &pSmPktDesc->smDesc;

    /* Check that this cpu is connected to shared memory */

    if (pSmPktDesc->status != SM_CPU_ATTACHED)
	{
	errno = S_smPktLib_NOT_ATTACHED;
	return (ERROR);				/* local cpu is not attached */
	}

    /* Check for null pointer */

    if (pPkt == NULL)
	{
	errno = S_smPktLib_INVALID_PACKET;
	return (ERROR);				/* null packet address */
	}

    /* Return packet to free list */

    return (smPktSllPut (&(pSmPktDesc->hdrLocalAdrs->freeList), pSmDesc->base, 
			 pSmDesc->tasRoutine, pSmDesc->tasClearRoutine, 
			 &(pPkt->header.node), NULL));
    }

/******************************************************************************
*
* smPktRecv - receive a packet sent via shared memory
*
* This routine receives a shared memory packet which was queued to this CPU.
* This routine should be called in response to an interrupt which notifies
* this CPU of the packet, or it can be called repeatedly in a polling
* fashion to check for input packets.  (NOTE: An interrupt will be generated
* only if there previously was not a packet queued to this CPU.  Therefore,
* after a packet is received, this routine should be called again to check
* for more packets.)
*
* Upon return, the location indicated by <ppPkt> will contain the address
* of the received packet, or NULL if there was none.
*
* RETURNS: OK, or ERROR.
*
* ERRNO: S_smPktLib_NOT_ATTACHED
*/

STATUS smPktRecv
    (
    SM_PKT_DESC	*	pSmPktDesc,	/* shared memory descriptor */
    SM_PKT **		ppPkt		/* location to put pkt addr */
    )
    {
    SM_PKT_CPU_DESC volatile *	pPktCpuDesc;
    STATUS			status;
    SM_DESC *			pSmDesc = (SM_DESC *) &pSmPktDesc->smDesc;
    int                         tmp;         /* temp storage */

    /* Check that this cpu is connected to shared memory */

    if (pSmPktDesc->status != SM_CPU_ATTACHED)
	{
	errno = S_smPktLib_NOT_ATTACHED;
	return (ERROR);				/* local cpu is not attached */
	}

    /* Get packet (if any) from input list */

    pPktCpuDesc = &((pSmPktDesc->cpuLocalAdrs) [pSmDesc->cpuNum]);
    						/* get addr of cpu descriptor */

    status = smPktSllGet (&(pPktCpuDesc->inputList), pSmDesc->base,
    		      	  pSmDesc->tasRoutine, pSmDesc->tasClearRoutine, 
			  (SM_SLL_NODE **) ppPkt);

    if ((status == OK) && (*ppPkt != NULL))
	{
	(*ppPkt)->header.nBytes = ntohl ((*ppPkt)->header.nBytes);
	(*ppPkt)->header.srcCpu = ntohl ((*ppPkt)->header.srcCpu);

        CACHE_PIPE_FLUSH ();                    /* CACHE FLUSH   [SPR 68334] */
        tmp = (*ppPkt)->header.srcCpu;		/* BRIDGE FLUSH  [SPR 68334] */
	}

    return (status);
    }

/******************************************************************************
*
* smPktSllGet - get a shared memory node from a singly-linked list
*
* This routine attempts to obtain a shared memory node from the
* specified singly-linked list of nodes.  If a node is available,
* the local address (for this CPU) of the node is placed in the
* location specified by <pNodeLocalAdrs>.  If no node was available
* from the list, a NULL is placed in this location.
*
* This routine uses the specified test-and-set function, <tasRoutine>, to
* obtain exclusive access to the linked list lock.  If the lock cannot
* be acquired within the number of tries specified by the smPktTasTries
* global variable, this routine will return ERROR.
*
* RETURNS: OK, or ERROR if could not acquire list lock.
*
* ERRNO: S_smPktLib_LOCK_TIMEOUT
*/

LOCAL STATUS smPktSllGet
    (
    SM_SLL_LIST volatile * listLocalAdrs,	/* local addr of packet list */
    int                    baseAddr,		/* addr conversion constant */
    FUNCPTR                tasRoutine,		/* test-and-set routine addr */
    FUNCPTR                tasClearRoutine,	/* test-and-set routine addr */
    SM_SLL_NODE **	   pNodeLocalAdrs	/* location to put node addr */
    )
    {
    int                    oldLvl;		/* saved interrupt level */
    int			   count;		/* temp value */
    SM_SLL_NODE volatile * pNode;		/* pointer to node */

    /* Take list lock */

    if (SM_LOCK_TAKE ((int *)&(listLocalAdrs->lock), tasRoutine, smPktTasTries,
                      &oldLvl) != OK)
        {
    	errno = S_smPktLib_LOCK_TIMEOUT;
        return (ERROR);                         /* can't take lock */
        }

    /* Get first node from list, if any */

    if ((pNode = (SM_SLL_NODE volatile *) ntohl (listLocalAdrs->head)) != NULL)
        {
        pNode = SM_OFFSET_TO_LOCAL ((int) pNode, baseAddr, SM_SLL_NODE *);
                                                /* convert to local address */

        listLocalAdrs->head = pNode->next;      /* next node (if any) is head */

        count = ntohl (listLocalAdrs->count) - 1;
        if (count <= 0)                      /* if list now empty */
            {
            listLocalAdrs->tail = htonl (0); /*  clear tail pointer */
            count = 0;
            }

        listLocalAdrs->count = htonl (count);
        }

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    count = listLocalAdrs->limit;		/* BRIDGE FLUSH  [SPR 68334] */

    /* Release list lock */

    SM_LOCK_GIVE ((int *)&(listLocalAdrs->lock), tasClearRoutine, oldLvl);

    /* Pass node address back to caller */

    *pNodeLocalAdrs = (SM_SLL_NODE *) pNode;    /* local node addr, or NULL */

    return (OK);
    }

/******************************************************************************
*
* smPktSllPut - add a shared memory node to a singly-linked list
*
* This routine attempts to add a shared memory node to the specified
* singly-linked list.  The node to be added is identified by <nodeLocalAdrs>.
* Unless the list has reached its maximum number of entries, the node
* will be added at the tail of the list.  The bus address of the node is
* used in the list entry.
*
* This routine uses the specified test-and-set function, <tasRoutine>, to
* obtain exclusive access to the linked list lock.  If the lock cannot
* be acquired within the number of tries specified by the smPktTasTries
* global variable, this routine will return ERROR.
*
* If the list already contains its maximum number of node entries, this
* routine will return ERROR.
*
* If <pListWasEmpty> is non-null and the node is successfully added
* to the list, this routine will set the BOOL pointed to by
* <pListWasEmpty> to TRUE if there were no previous nodes in the list.
*
* RETURNS: OK, or ERROR if packet ptr is NULL, list full, or could
* not acquire list lock.
*
* ERRNO: S_smPktLib_LOCK_TIMEOUT, S_smPktLib_LIST_FULL
*/

LOCAL STATUS smPktSllPut
    (
    SM_SLL_LIST volatile * listLocalAdrs,	/* local addr of packet list */
    int			   base,		/* base address */
    FUNCPTR		   tasRoutine,		/* test-and-set routine addr */
    FUNCPTR		   tasClearRoutine,	/* test-and-set routine addr */
    SM_SLL_NODE volatile * nodeLocalAdrs,	/* local addr of node */
    BOOL *		   pListWasEmpty	/* set to true if adding to
                                                 * empty list */
    )
    {
    int			   oldLvl;		/* saved interrupt level */
    int			   nodeOff;		/* pointer to node */
    SM_SLL_NODE volatile * tailLocalAdrs;	/* pointer to list tail node */


    nodeLocalAdrs->next = 0;         /* mark node as end of list */
    nodeOff = htonl (SM_LOCAL_TO_OFFSET (nodeLocalAdrs, base));

    /* Take list lock */

    if (SM_LOCK_TAKE ((int *)&(listLocalAdrs->lock), tasRoutine, smPktTasTries,
                      &oldLvl) != OK)
        {
    	errno = S_smPktLib_LOCK_TIMEOUT;
        return (ERROR);                         /* can't take lock */
        }

    /* Add node to list */

    if (ntohl (listLocalAdrs->count) >= ntohl (listLocalAdrs->limit))
        {
        SM_LOCK_GIVE ((int *)&(listLocalAdrs->lock), tasClearRoutine, oldLvl);
        errno = S_smPktLib_LIST_FULL;
        return (ERROR);                         /* list has max nodes already */
        }
                                                /* if list previously empty */
    if (ntohl (listLocalAdrs->tail) == 0)
        {
        if (pListWasEmpty != NULL)
            *pListWasEmpty = TRUE;

        /* node is new head also */

        listLocalAdrs->head = nodeOff;
        }
    else
        {
        if (pListWasEmpty != NULL)
            *pListWasEmpty = FALSE;

        tailLocalAdrs = SM_OFFSET_TO_LOCAL (ntohl (listLocalAdrs->tail),
                                            base, SM_SLL_NODE volatile *);
                                                /* get addr of tail node */

        tailLocalAdrs->next = nodeOff;
                                             /* link new node to old tail */
        }

    listLocalAdrs->tail  = nodeOff;
                                                /* node is new tail */
    listLocalAdrs->count = ntohl (htonl (listLocalAdrs->count) + 1);
                                                /* one more node in list */

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    nodeOff = listLocalAdrs->limit;		/* BRIDGE FLUSH  [SPR 68334] */

    /* Release list lock */

    SM_LOCK_GIVE ((int *)&(listLocalAdrs->lock), tasClearRoutine, oldLvl);

    return (OK);
    }

/******************************************************************************
*
* smPktDetach - detach CPU from shared memory
*
* This routine ends the "attachment" between the calling CPU and
* the shared memory area specified by <pSmPktDesc>.  No further shared
* memory operations may be performed until a subsequent smPktAttach()
* is completed.
*
* RETURNS: OK, or ERROR.
*
* ERRNO: S_smPktLib_NOT_ATTACHED
*/

STATUS smPktDetach
    (
    SM_PKT_DESC *	pSmPktDesc,	/* ptr to shared mem descriptor */
    BOOL		noFlush		/* true if input queue should not
					 * be flushed */
    )
    {
    SM_PKT_CPU_DESC volatile *	pPktCpuDesc;	/* cpu descriptor */
    SM_PKT volatile *		pPkt;		/* pointer to input packet */
    SM_DESC *			pSmDesc = (SM_DESC *) &pSmPktDesc->smDesc;
    int                         tmp;            /* temp storage */

    /* Check that this cpu is connected to shared memory */

    if (pSmPktDesc->status != SM_CPU_ATTACHED)
	{
	errno = S_smPktLib_NOT_ATTACHED;
	return (ERROR);				/* local cpu is not attached */
	}

    if (smDetach (pSmDesc) == ERROR)
	return (ERROR);

    /* get addr of cpu descriptor */

    pPktCpuDesc = (SM_PKT_CPU_DESC volatile *) &((pSmPktDesc->cpuLocalAdrs) \
                                                  [pSmDesc->cpuNum]);

    pPktCpuDesc->status = htonl (SM_CPU_NOT_ATTACHED);/* mark as not attached */
    pSmPktDesc->status  = SM_CPU_NOT_ATTACHED;	  /* also mark sh mem descr */

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    tmp = pPktCpuDesc->status;                  /* BRIDGE FLUSH  [SPR 68334] */

    if (noFlush == FALSE)			/* flush input queue */
	{
	FOREVER
	    {
	    if (smPktSllGet (&(pPktCpuDesc->inputList), pSmDesc->base,
			     pSmDesc->tasRoutine, pSmDesc->tasClearRoutine,
			     (SM_SLL_NODE **) &pPkt) != OK)
		return (ERROR);			/* error getting input packet */

	    if (pPkt == NULL)
		break;				/* no more input packets */

	    if (smPktSllPut (&(pSmPktDesc->hdrLocalAdrs->freeList),
			     pSmDesc->base, pSmDesc->tasRoutine,
			     pSmDesc->tasClearRoutine,
		      	     &(pPkt->header.node), NULL) != OK)
		return (ERROR);			/* error adding to free list */
	   }
	}

    return (OK);
    }

/******************************************************************************
*
* smPktInfoGet - get current status information about shared memory
*
* This routine obtains various pieces of information which describe the
* current state of the shared memory area specified by <pSmPktDesc>.  The
* current information is returned in a special data structure, SM_PKT_INFO,
* whose address is specified by <pInfo>.  The structure must have been
* allocated before this routine is called.
*
* RETURNS: OK, or ERROR if this CPU is not attached to shared memory area.
*
* ERRNO: S_smPktLib_NOT_ATTACHED
*/

STATUS smPktInfoGet
    (
    SM_PKT_DESC *	pSmPktDesc,	/* shared memory descriptor */
    SM_PKT_INFO *	pInfo		/* info structure to fill */
    )
    {
    SM_PKT_MEM_HDR volatile *	pHdr;		/* shared memory header */
    SM_PKT_CPU_DESC volatile *	pCpuDesc;	/* cpu descriptor */
    int				ix;		/* index variable */
    int                         tmp;            /* temp storage */

    /* Check that this cpu is connected to shared memory */

    if (pSmPktDesc->status != SM_CPU_ATTACHED)
	{
	errno = S_smPktLib_NOT_ATTACHED;
	return (ERROR);				/* local cpu is not attached */
	}

    if (smInfoGet (&(pSmPktDesc->smDesc), &(pInfo->smInfo)) == ERROR)
	return (ERROR);

    /* Use local address to get info from shared memory packet header */

    pHdr = (SM_PKT_MEM_HDR volatile *) pSmPktDesc->hdrLocalAdrs;

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    tmp = pHdr->maxPktBytes;                    /* PCI bridge bug [SPR 68844]*/

    pInfo->maxPktBytes = ntohl (pHdr->maxPktBytes);/* max data bytes per pkt */
    pInfo->totalPkts   = ntohl (pHdr->freeList.limit);/* total number */
    pInfo->freePkts    = ntohl (pHdr->freeList.count);/* number free */

    /* Get count of attached cpu's starting with first cpu table entry */

    pCpuDesc = (SM_PKT_CPU_DESC volatile *) pSmPktDesc->cpuLocalAdrs;
    pInfo->attachedCpus = 0;

    for (ix = 0;  ix < pInfo->smInfo.maxCpus;  ix++)
	{
	if (ntohl (pCpuDesc->status) == SM_CPU_ATTACHED)
	    pInfo->attachedCpus++;

	pCpuDesc++;				/* next entry */
	}

    return (OK);
    }

/******************************************************************************
*
* smPktCpuInfoGet - get information about a single CPU using shared memory
*
* This routine obtains a variety of information describing the CPU specified
* by <cpuNum>.  If <cpuNum> is NONE (-1), this routine returns information
* about the local (calling) CPU.  The information is returned in a special
* structure, SM_PKT_CPU_INFO, whose address is specified by <pCpuInfo>.  (The
* structure must have been allocated before this routine is called.)
*
* RETURNS: OK, or ERROR.
*
* ERRNO: S_smPktLib_NOT_ATTACHED
*/

STATUS smPktCpuInfoGet
    (
    SM_PKT_DESC *	pSmPktDesc,	/* pkt descriptor */
    int			cpuNum,		/* cpu to get info about */
    SM_PKT_CPU_INFO *	pCpuInfo	/* structure to fill */
    )
    {
    SM_PKT_CPU_DESC volatile * pCpuDesc;	/* ptr to cpu descriptor */
    int                        tmp;             /* temp storage */

    /* Check that the local cpu is connected to shared memory */

    if (pSmPktDesc->status != SM_CPU_ATTACHED)
	{
	errno = S_smPktLib_NOT_ATTACHED;
	return (ERROR);				/* local cpu is not attached */
	}

    if (cpuNum == NONE)
	cpuNum = pSmPktDesc->smDesc.cpuNum;

    if (smCpuInfoGet (&(pSmPktDesc->smDesc), cpuNum,
		      &(pCpuInfo->smCpuInfo)) == ERROR)
	return (ERROR);

    /* get address of cpu descr */

    pCpuDesc = (SM_PKT_CPU_DESC volatile *)&(pSmPktDesc->cpuLocalAdrs[cpuNum]);

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    tmp = pCpuDesc->status;                     /* PCI bridge bug [SPR 68844]*/

    pCpuInfo->status  = ntohl (pCpuDesc->status);/* attached or not attached */

    /* Get packet counts */

    pCpuInfo->inputPkts    = ntohl (pCpuDesc->inputList.count);
						/* current pkts in input queue*/
    pCpuInfo->maxInputPkts = ntohl (pCpuDesc->inputList.limit);
						/* max pkts allowed in queue */

    /*
     * XXX - free list counts - for future use when cpu's have free lists...
     *
     * pCpuInfo->freePkts  = ntohl (pCpuDesc->freeList.count);
     * pCpuInfo->totalPkts = ntohl (pCpuDesc->freeList.limit);
     *
     */

    pCpuInfo->freePkts  = 0;			/* (future use) */

    return (OK);
    }

/******************************************************************************
*
* smPktBeat - increment packet heartbeat in shared memory anchor
*
* This routine increments the shared memory packet "heartbeat".  The heartbeat
* is used by processors using the shared memory packet library to
* confirm that the shared memory network is active.  The shared memory
* area whose heartbeat is to be incremented is specifed by <pSmPktHdr>, which
* is the address of a shared memory packet header region in which the heartbeat
* count is maintained.
*
* This routine should be called periodically by the master CPU.
*
* RETURNS: N/A
*/

void smPktBeat 
    (
    SM_PKT_MEM_HDR *	pSmPktHdr 	/* ptr to sm packet header */
    )
    {
    SM_PKT_MEM_HDR volatile * pPktHdrv = (SM_PKT_MEM_HDR volatile *) pSmPktHdr;
    int                       tmp;              /* temp storage */

    /* Increment heartbeat */

    tmp = pPktHdrv->heartBeat;                  /* PCI bridge bug [SPR 68844]*/

    pPktHdrv->heartBeat = htonl (ntohl (pPktHdrv->heartBeat) + 1);

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    tmp = pPktHdrv->heartBeat;                  /* BRIDGE FLUSH  [SPR 68334] */
    }


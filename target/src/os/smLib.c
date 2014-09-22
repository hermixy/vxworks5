/* smLib.c - VxWorks common shared memory library */

/* Copyright 1984-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
02s,03may02,mas  cache flush and volatile fix (SPR 68334); bridge flush fix
		 (SPR 68844)
02r,14nov01,mas  added smLibInit() for future common code base upgrade
02q,24oct01,mas  doc update (SPR 71149)
02p,20sep01,jws  eliminate min 5000 tries in smLockTake() (SPR68418)
02o,17mar99,dat  added docs about SM_INT_USER_1/2, SPR 25804
02n,21feb99,jdi  doc: listed errnos.
02m,18mar98,dat  added exponential backoff with limits, (more SPR 9312)
02l,05dec97,dat  rework SPR 9312, add global variable smLockTakeDelayCnt.
02k,29sep97,dat  SPR 9312, fixed smLockTake retry loop.
02j,14jul93,wmd  Removed ntohl for anchorLocalAddr in smSetup() (SPR #2282).
02i,10feb93,pme  Changed smLockTake retry time calculation.
02h,29sep92,pme  Suppressed warning message in smLockTake.
		 Added write pipe flush calls where needed.
02g,02aug92,kdl  Uncommented include of "copyright_wrs.h".
02f,24jul92,elh  Moved heartbeat from anchor to header.
02e,13jul92,elh  Added tasClear functionality. Added pme's changes to
		 smLockTake.
02d,02jun92,elh  the tree shuffle
02c,27may92,elh	 Further split up the library.  Changed to use offsets,
		 Made completely independent.  General cleanup.
02b,20may92,pme+ Removed pkt specific data structures (and routines
	    elh	 that use them to their corrosponding files.
02a,14may92,pme  Splitted to keep only routines common to backplane driver
		 and shared memory objects.
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

This library contains the shared memory routines which are common
to the shared memory libraries: smPktLib and smObjLib.  
It provides basic shared memory functionality such as shared 
memory attachment, detachment and lock synchronization.


SHARED MEMORY MASTER CPU

One CPU node acts as the shared memory master.  This CPU initializes
the shared memory header and sets up the shared memory anchor.  These
steps are performed by the master calling the smSetup() routine.
This routine should be called only by the shared memory master CPU,
and it should only be called once for a given shared memory region.
(It is, however, possible to maintain and use multiple separate
shared memory regions.)

Once smSetup() has completed successfully, there is little functional
difference between the master CPU and other CPU's using shared memory,
except that the master is responsible for maintaining the shared memory
packet and shared memory object heartbeats in
the shared memory header regions.


SHARED MEMORY ANCHOR

The shared memory anchor is a small data structure which is at
a predetermined location, to allow all CPU's using shared memory to
find it.  The shared memory anchor contains the base offset to the
actual shared memory header.  (This allows the master CPU to 
dynamically allocate the shared memory region.)  Another field in 
the anchor contains a "ready value" which is set to a specific 
value when the master CPU has completed initializing the shared memory 
region.

The shared memory anchor does not have to be defined as part of the
shared memory region.  However, it must be located in a similar 
address space; the address translation constants which CPU boards use 
to convert local addresses to bus addresses must apply equally to the 
anchor and the regular shared memory region.


ATTACHING TO SHARED MEMORY

Each CPU, master or non-master, which will use the shared memory region
must attach itself to the shared memory.  The shared memory region must
have already been initialized by the master CPU calling smSetup().

The first step in attaching to shared memory is for each CPU to allocate and
initialize a shared memory descriptor (SM_DESC).  This structure describes
the individual CPU's attachment to the shared memory region and is used
in all subsequent shared memory calls to identify which shared memory
region is being used.  Since the shared memory descriptor is used only
by the local CPU, it is not necessary for the descriptor itself to be
located in shared memory.  In fact, it is preferable for the descriptor
to be allocated from the CPU's local memory, since local memory is usually
more efficient to access.

The shared memory descriptor is initialized by calling the smInit()
routine.  This routine takes a number of parameters which specify the
characteristics of this CPU and its access to shared memory.

After the shared memory descriptor has been initialized, the CPU may
attach itself to the shared memory region.  This is done by calling the
smAttach() routine. 


DETACHING FROM SHARED MEMORY

The attachment of a CPU to shared memory may be ended by calling
smDetach().  This routine will mark the calling CPU as no longer
attached.  The CPU may re-attach itself to the shared memory region by 
later calling smAttach().  (When re-attaching, the original shared 
memory descriptor may be re-used if the CPU's configuration remains 
the same, or new values may be specified via smInit().)


INTERRUPTS

There must be some method for a CPU to be notified of an occuring event.
The preferred method is for the CPU initiating the event to interrupt the 
affected CPU.  This will be highly dependent on the specific hardware being 
used.  If interrupts cannot be used, a polling scheme may be employed, but 
this is much less efficient.

Three types of interrupts are supported, mailbox interrupts, vme
bus interrupts, and user defined interrupts.  Mailbox interrupts are the
first preferred method (SM_INT_MAILBOX), followed by bus (backplane) interrupts
(SM_INT_BUS).  If interrupts cannot be used, a polling scheme may be employed 
(SM_INT_NONE), but this is much less efficient. 

The user interrupt types (SM_INT_USER_1 and SM_INT_USER_2) allow the BSP
writer to specify a routine to be used
to generate an interrupt in another CPU.  The three interrupt arguments
are passed to the
user's routine.  When using this method, the user is responsible for calling
the smUtilIntRoutine(), when an incoming signal (interrupt) is detected.
The routine smUtilIntRoutine() will process any data packets in the list
for the local cpu.  The global variable <smUtilUser1Rtn> is a pointer to the
user routine for SM_INT_USER_1 .  The variable <smUtilUser2Rtn> is the pointer
for the SM_INT_USER_2 routine.

When a CPU initailizes its shared memory descriptor via the smInit() call, 
it passes in an interrupt type as well as three interrupt arguments.  This 
describes how the cpu wishes to be notified of events.  The interrupt 
types recognized by this library are listed in smLib.h. These values may 
be obtained for any attached CPU by calling smCpuInfoGet().  

The default interrupt method for a particular target is defined by 
the configuration parameters: SM_INT_TYPE, SM_INT_ARG1, SM_INT_ARG2, 
and SM_INT_ARG3 .

When a CPU wishs to notify a destination CPU of an event, the sending 
CPU will call smUtilIntGen(), to interrupt the destination CPU.
smUtilIntGen() should return OK if the destination CPU was
successfully notified, or ERROR if it was not.

If it is not possible to use interrupts to notify receiving CPU's, a
polling method may be used.  


INTERNAL

This file runs under only vxWorks.  SunOS is no longer supported.

The shared memory libaries are split into mulitple files.
Below is the internal layout of the shared memory libraries:

		              		    ___________
shared memory object         		    | smObjLib|
library		             	    	   / __________ 
		              		  /	
shared memory		____________     /
packet library		| smPktLib |    / 
			____________   /
			     |	      /			
			     v       /
			____________v
common shared memory	| smLib	    |
library			_____________
			     |
			     v
shared memory           _____________   
support routines	| smUtilLib |
			_____________

\is
\i `smObjLib'
Contains the share memory object routines.

\i `smPktLib'
Contains the shared memory packet routines.  It provides 
packet passing functionality to send and retreive data packets over 
the backplane using shared memory.  It is used by the backplane driver.

\i `smLib'
Contains routines the shared memory routines that are common
to backplane driver and the shared memory objects:  
		
\i `smUtilLib'
Contains the OS specific routines needed by the shared 
memory libraries.  This module should be provided for each OS 
for which shared memory is ported and must 
contain at a minimum the following routines:

\cs
	smUtilMemProbe	- probe memory to see if it exists 
	smUtilSofTas	- software test-and-set routine 
	smUtilTas	- hardware test-and-set routine
	smUtilTasClear	- clear hardware test-and-set routine
	smUtilProcNumGet- get processor number
	smUtilDelay	- delay for specified number of time 
	smUtilRateGet	- get number of ticks per second 
	intLock		- lock out interrupts
	intUnlock	- unlock interrupts
	smUtilIntGen	- interrupt generation routine
	smUtilIntConnect- connect one or more routine to shared memory 
		          interrupt.
	smUtilIntRoutine- shared memory interrupt routine
	smUtilPollTask  - shared memory polling task
\ce
\ie

SHARED MEMORY LOCK

The routines smLockTake() and smLockGive() are used to achieve the necessary
inter-processor locks.  The routine that is called to perform a Test and
Set (TAS) primitive is one of the calling arguments.  The delay algorithm
between TAS attempts is an exponential one.  The delay between retries is
twice as great as the previous delay, until the delay reaches a maximum
value and is then reset to its starting value.  The following variables
can be tuned to give best performance for any specific CPU and clock
combination.

\cs
   int smLockTakeDelayMin = 256;
   int smLockTakeDelayMax = 1*1024*1024;
\ce

The minimum number of retries (>= 1) to be attempted is specified as an
argument to smLockTake().  (One retry is always made if the first attempt
fails, even if the minimum number of retries is specified as zero.)

INTERNAL
There needs to be better documentation discussing the interfaces
and functionality of the OS specific library, smUtilLib.

SEE ALSO: \tb VxWorks Programmer's Guide: Shared-Memory Objects,  
\tb VxWorks Network Programmer's Guide: Data Link Layer Network Components.
*/

/* Includes */

#include "vxWorks.h"
#include "cacheLib.h"
#include "smLib.h"
#include "smUtilLib.h"

/* Defines */

#define	SM_VERSION		1	/* protocol version for sh mem anchor */

/* Globals */

int		smAliveTimeout   = DEFAULT_ALIVE_TIMEOUT;

/* current maximum # of tries to get lock used for statistics */

int 		smCurMaxTries = 0;

int		smLockTakeDelayMin = 256;
int		smLockTakeDelayMax = 1*1024*1024;



/* Locals */

LOCAL BOOL	smLibInitialized = FALSE;
LOCAL int	smRegionsMax 	 = SM_REGIONS_MAX;
LOCAL SM_REGION smRegions [SM_REGIONS_MAX];

/* Forward declarations */

LOCAL int smRegionGet (SM_ANCHOR * anchorLocalAdrs);
LOCAL int smRegionFind (SM_ANCHOR * anchorLocalAdrs);


/******************************************************************************
*
* smLibInit - initialize the shared memory library and shared memory region
*
* This routine is normally called only by the system startup code during
* kernel initialization.  It prepares the library and locates the shared
* memory region via either static lookup or dynamic allocation.  A static
* lookup is first attempted if <pRgnCfgTbl> is not NULL.  If the shared
* memory region is not found or <pRgnCfgTbl> is NULL, an uncached shared
* memory region of size <objSize> + <netSize> is allocated from the kernel
* heap.
*
* INTERNAL
* This routine is a place holder until a common code base version of smLib is
* created.  The input arguments are identical with those used in AE.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void smLibInit
    (
    void * pRgnCfgTbl,		/* address of memory configuration table */
    UINT   objSize,		/* shared object region size */
    UINT   netSize		/* shared memory network region size */
    )
    {
    /* NOT YET IMPLEMENTED */

    if (pRgnCfgTbl == NULL)
        ;

    if (objSize == netSize)
        ;
    }


/******************************************************************************
*
* smSetup - set up shared memory (master CPU only)
*
* This routine should be called only by the master CPU using shared
* memory.  It initializes the specified memory area for use by the
* shared memory protocol.  This includes initializing the shared memory
* anchor and the shared memory header.
*
* After the shared memory has been initialized, this and other CPU's
* may initialize a shared memory descriptor to it, using smInit(),
* and then attach to the shared memory area, using smAttach().
*
* The <anchorLocalAdrs> parameter is the memory address by which the master
* CPU accesses the actual shared memory anchor region to be initialized.
*
* The <smLocalAdrs> parameter is the memory address by which the master
* CPU accesses the actual shared memory region to be initialized.
*
* The shared memory routines must be able to obtain exclusive access to
* shared data structures.  To allow this, a test-and-set operation is
* used.  It is preferable to use a genuine test-and-set instruction, if
* the hardware being used permits this.  If this is not possible, smLib
* provides a software emulation of test-and-set.  The <tasType> parameter
* specifies what method of test-and-set is to be used.
*
* The <maxCpus> parameter specifies the maximum number of CPU's which may
* use the shared memory region.
*
* The amount of shared memory taken as a result of the call, gets returned
* in <pMemUsed>
*
* INTERNAL
* The first item in the shared memory area is the shared memory header (SM_HDR).
* Following this is an array of CPU descriptors (SM_CPU_DESC); this table
* contains one CPU descriptor for each possible CPU, as specified by <maxCpus>.
*
* RETURNS: OK, or ERROR.
*
* ERRNO: S_smLib_MEMORY_ERROR
*/

STATUS smSetup
    (
    SM_ANCHOR * anchorLocalAdrs,	/* local addr of anchor */
    char *	smLocalAdrs,		/* local addr of sh mem area */
    int		tasType,		/* method of test-and-set */
    int		maxCpus,		/* max number of cpu's */
    int	*	pMemUsed		/* amount of memory used */
    )
    {
    SM_ANCHOR volatile * pAnchorv = (SM_ANCHOR volatile *) anchorLocalAdrs;
    SM_HDR volatile *	 pHdr;			/* local addr of sh mem hdr */
    char		 temp = 0;		/* temp location */

    /* Initialize shared memory region table */

    if (!smLibInitialized)
	{
	bzero ((char *) smRegions, sizeof (smRegions));
	smLibInitialized = TRUE;
	}

    /* Probe shared memory */

    if (smUtilMemProbe (smLocalAdrs, VX_WRITE, sizeof (char), &temp) != OK)
        {
       	errno = S_smLib_MEMORY_ERROR;
	return (ERROR);
	}

    /* Find the Region */

    if (smRegionFind (anchorLocalAdrs) != ERROR)
	{
	pHdr = SM_OFFSET_TO_LOCAL (ntohl (pAnchorv->smHeader), (int) pAnchorv,
                                   SM_HDR volatile *);

	if ((ntohl (pHdr->maxCpus) != maxCpus) ||
	    (ntohl (pHdr->tasType) != tasType))
	    printf ("smSetup:Warning! previous parameters differ!\n");

	*pMemUsed = 0;
	return (OK);
	}

    if (smRegionGet (anchorLocalAdrs) == ERROR)
	return (ERROR);

    *pMemUsed = sizeof (SM_HDR) + (maxCpus * sizeof (SM_CPU_DESC));

    /* Partially initialize shared memory header */

    bzero (smLocalAdrs, *pMemUsed);
    pHdr 	   = (SM_HDR volatile *) smLocalAdrs; /* header = SM start */
    pHdr->tasType  = htonl (tasType);		/* test-and-set method */
    pHdr->maxCpus  = htonl (maxCpus);		/* max number of cpu's */
    pHdr->cpuTable = htonl (SM_LOCAL_TO_OFFSET (pHdr + sizeof (SM_HDR),
					        (int) anchorLocalAdrs));

    /* Set up the anchor */

    bzero ((char *) pAnchorv, sizeof (SM_ANCHOR));
    pAnchorv->version    = htonl (SM_VERSION);  /* sh mem version */
    pAnchorv->smHeader   = htonl (SM_LOCAL_TO_OFFSET (pHdr, (int) pAnchorv));
    pAnchorv->masterCpu  = htonl (SM_MASTER); /* sh mem  master */
    pAnchorv->readyValue = htonl (SM_READY);  /* sh mem available */

    CACHE_PIPE_FLUSH ();                /* CACHE FLUSH   [SPR 68334] */
    maxCpus = pHdr->maxCpus;		/* BRIDGE FLUSH  [SPR 68334] */

    return (OK);				/* shared mem init complete */
    }

/******************************************************************************
*
* smInit - initialize shared memory descriptor
*
* This routine initializes a shared memory descriptor.  The descriptor
* must have been previously allocated (generally in the CPU's local
* memory).  Once the descriptor has been initialized by this routine,
* the CPU may attach itself to the shared memory area by calling
* smAttach().
*
* Only the shared memory descriptor itself is modified by this routine.
* No structures in shared memory are affected.
*
* The <pSmDesc> paramter is the address of the shared memory descriptor
* which is to be initialized; this structure must have already been
* allocated before smInit() is called.
*
* The <anchorLocalAdrs> parameter is the memory address by which the local
* CPU may access the shared memory anchor.  This address may vary for
* different CPU's because of address offsets (particularly if the anchor is
* located in one CPU's dual-ported memory).
*
* The <ticksPerBeat> parameter specifies the frequency of the shared memory
* anchor's heartbeat.  The frequency is expressed in terms of how many CPU
* ticks on the local CPU correspond to one heartbeat period.
*
* The <intType>, <intArg1>, <intArg2>, and <intArg3> parameters allow a
* CPU to announce the method by which it is to be notified of input packets
* which have been queued to it.  Once this CPU has attached to the shared
* memory region, other CPU's will be able to determine these interrupt
* parameters by calling smCpuInfoGet().  
*
* RETURNS: N/A
*/

void smInit
    (
    SM_DESC *	pSmDesc,	 /* sh mem descr to init*/
    SM_ANCHOR *	anchorLocalAdrs, /* local addr of sh mem anchor*/
    int		ticksPerBeat,	 /* cpu ticks per heartbeat */
    int		intType,	 /* interrupt method */
    int		intArg1,	 /* interrupt argument #1 */
    int		intArg2,	 /* interrupt argument #2 */
    int		intArg3		 /* interrupt argument #3 */
    )
    {
    if (pSmDesc == NULL)
	return;					/* don't use null ptr */

    bzero ((char *) pSmDesc, sizeof (SM_DESC));

    /* Copy input parameters */

    pSmDesc->anchorLocalAdrs  = anchorLocalAdrs;
    pSmDesc->base	      = (int) anchorLocalAdrs;
    pSmDesc->cpuNum	      = smUtilProcNumGet ();
    pSmDesc->ticksPerBeat     = (ticksPerBeat == 0) ? smUtilRateGet () :
						      ticksPerBeat;
    pSmDesc->intType	      = intType;
    pSmDesc->intArg1	      = intArg1;
    pSmDesc->intArg2	      = intArg2;
    pSmDesc->intArg3	      = intArg3;
    pSmDesc->status	      = SM_CPU_NOT_ATTACHED;	/* initial status */
    }

/******************************************************************************
*
* smIsAlive - determine if shared memory is initialized and active
*
* This routine examines the shared memory anchor and heartbeat to
* determine whether the shared memory has been initialized and that
* it is running.  The shared memory is specified by <pAnchor>, the
* address of the shared memory anchor.  If the shared
* memory network is active, TRUE is returned; otherwise, FALSE is
* returned.  <pHeader> points to the location of the header containing
* the heartbeat.  <base> is the base address from which to calculate the
* address of the heartbeat counter in the header in shared memory.
*
* The anchor ready-value must contain the special value SM_READY to
* indicate that the master CPU has successfully initialized the shared
* memory region.  The heartbeat must increment at least twice
* within the timeout period to be recognized as active.
*
* <heartBeats> specifies the number of beats to wait for a correct
* ready-value and active heartbeat before timing out.  A zero value
* indicates the use of a default value, specified by the global variable 
* 'smAliveTimeout'.
*
* <ticksPerBeat> specifies the number of ticks per heartbeat.
*
* This routine safely probes the anchor addresses to avoid bus errors
* (in case the anchor address is not yet accessable).  Occasional
* messages are written to standard output if a bus error occurs but is
* handled.  Occasional messages will also appear to indicate the most
* recently obtained values for the ready-value and heartbeat.
*
* This routine may not be called from interrupt level.
*
* This routine does not set errno.
*
* RETURNS: TRUE or FALSE.
*/

BOOL smIsAlive
    (
    SM_ANCHOR *	pAnchor,	/* ptr to shared mem anchor */
    int *	pHeader,	/* {smObj/smPkt} header */
    int		base,		/* base */
    int		heartBeats,	/* heartbeat periods */
    int 	ticksPerBeat   	/* ticks per beat */
    )
    {
    UINT	readyValue;        /* ready value from anchor */
    UINT	newHeartBeat;      /* beat value from anchor */
    UINT	oldHeartBeat;      /* saved beat value */
    int		incCnt;            /* number of beat increments observed*/
    int *	pHeartBeat = NULL; /* ptr to heartbeat */
    int		temp;              /* temp storage */

    newHeartBeat = 0;				/* init beat values */
    oldHeartBeat = 0;
    incCnt = 0;

    /* Determine number of heartbeat periods to wait */

    if (ticksPerBeat == 0)
        {
	ticksPerBeat = smUtilRateGet ();
        }

    if (heartBeats == 0)
        {
	heartBeats = smAliveTimeout * (smUtilRateGet ()) / ticksPerBeat;
        }

    /* Check anchor and heartbeat until ready or countdown expires */

    while (heartBeats-- > 0)
	{
	/* XXX - invalidate data cache here ?? */

	/* Probe and get anchor ready-value */

	if (smUtilMemProbe ((char *) &pAnchor->readyValue, VX_READ, 
			    sizeof (int), (char *) &readyValue) != OK)
	    {
	    if ((heartBeats % 10) == 8)		/* print error occasionally */
                {
                printf ("\nsmIsAlive: bus error checking anchor\n");
                }
	    }

	readyValue = ntohl (readyValue);

	if (readyValue == SM_READY)	/* if anchor appears ready */
	    {
	    /*
	     * Calculate the location of the heartbeat.
	     * The heartbeat must be the first location in {smPkt/smObj}
	     * header. 
	     */

            CACHE_PIPE_FLUSH ();		/* FLUSH BUFFER  [SPR 68334] */
            temp = *(int *)pHeader;		/* PCI bridge bug [SPR 68844]*/

	    pHeartBeat = SM_OFFSET_TO_LOCAL (ntohl (*pHeader), base, int *);

	    /* Probe and get heartbeat value */

	    if (smUtilMemProbe ((char *) pHeartBeat, VX_READ, sizeof (int),
				(char *) &newHeartBeat) == OK)
		{
		newHeartBeat = ntohl (newHeartBeat);

	    	/* Check if heartbeat incremented */

	    	if ((newHeartBeat <= oldHeartBeat) || (oldHeartBeat == 0))
	    	    {
	    	    oldHeartBeat = newHeartBeat; /* no beat - try again */
	    	    }
	    	else				/* beat incremented! */
	    	    {
		    if (++incCnt >= 2)		/* if 2 incr's have been seen*/
		        {
	    	    	return (TRUE);		/* SHARED MEM IS ALIVE & WELL*/
		        }
	    	    }
		}
	    else				/* could not probe heartbeat */
	    	{
		if ((heartBeats % 10) == 8)	/* print error occasionally */
		    {
	    	    printf ("\nsmIsAlive: bus error getting heartbeat\n");
		    }
	    	}

	    }  /* end if (readyValue) */


	/* Occasionally display last obtained ready-value and heartbeat */

	if ((heartBeats % 10) == 6)
	    {
	    printf ("smIsAlive:  readyValue = 0x%x, heartbeat = 0x%x\n",
		    readyValue, newHeartBeat);
	    }

	/* Wait before checking heartbeat again */

	if (smUtilDelay ((char *) pHeartBeat, ticksPerBeat) != OK)
	    {					/* wait enough for 1 beat */
	    return (FALSE);			/* called from int level?? */
	    }

	}  /* end while */

    return (FALSE);				/* timed out */
    }


/******************************************************************************
*
* smAttach - attach a node to shared memory
*
* This routine "attaches" the local CPU to a shared memory area.  The
* shared memory area is identified by the shared memory descriptor
* whose address specified by <pSmDesc>.  The descriptor must have
* already been initialized by calling smInit().
*
* This routine should get called only after and when the shared memory has
* been initialized by the master CPU.  To determine this, smIsAlive() must
* have been called to check the shared memory anchor for both the proper 
* value (SM_READY_VALUE) in the anchor's ready-value field and an 
* active heartbeat.
*
* The attachment to shared memory may be ended by calling smDetach().
*
* RETURNS: OK, or ERROR.
*
* ERRNO: S_smLib_INVALID_CPU_NUMBER
*
* SEE ALSO: smIsAlive(), smInit()
*/

STATUS smAttach
    (
    SM_DESC *	pSmDesc		/* ptr to shared mem descriptor */
    )
    {
    SM_HDR volatile *	   pHdr;        /* ptr to shared mem header */
    SM_CPU_DESC volatile * pCpuDesc;    /* ptr to shared mem cpu descriptor */
    int			   cpuNum;      /* this cpu's number */
    int			   temp;        /* temp storage */

    cpuNum = pSmDesc->cpuNum;

    /* Get local address for shared mem header */

    CACHE_PIPE_FLUSH ();			/* FLUSH BUFFER  [SPR 68334] */
    temp = pSmDesc->anchorLocalAdrs->smHeader;	/* PCI bridge bug [SPR 68844]*/

    pHdr = SM_OFFSET_TO_LOCAL (ntohl (pSmDesc->anchorLocalAdrs->smHeader),
                               pSmDesc->base, SM_HDR volatile *);

    pSmDesc->headerLocalAdrs = (SM_HDR *) pHdr;

    /* Copy data from shared mem header */

    pSmDesc->maxCpus         = ntohl (pHdr->maxCpus);
    pSmDesc->cpuTblLocalAdrs = SM_OFFSET_TO_LOCAL (ntohl (pHdr->cpuTable),
                                                   pSmDesc->base,
                                                   SM_CPU_DESC *);

    /* Determine test-and-set routine to use */

    pSmDesc->tasClearRoutine = NULL;

    if (ntohl (pHdr->tasType) == SM_TAS_HARD)   /* if using hardware tas */
	{
        pSmDesc->tasRoutine = smUtilTas;
        pSmDesc->tasClearRoutine = (FUNCPTR) smUtilTasClear; 
	}
    else
        pSmDesc->tasRoutine = smUtilSoftTas;    /* use software tas routine */


    /* Init entry in cpu table */

    if (cpuNum < 0  ||  cpuNum >= pSmDesc->maxCpus)
        {
        errno = S_smLib_INVALID_CPU_NUMBER;
        return (ERROR);                         /* cpu number out of range */
        }

    /* calculate addr of cpu desc in global table.  */

    pCpuDesc = (SM_CPU_DESC volatile *) &((pSmDesc->cpuTblLocalAdrs) [cpuNum]);

    pCpuDesc->intType = htonl (pSmDesc->intType); /* interrupt method */
    pCpuDesc->intArg1 = htonl (pSmDesc->intArg1); /* interrupt argument #1 */
    pCpuDesc->intArg2 = htonl (pSmDesc->intArg2); /* interrupt argument #2 */
    pCpuDesc->intArg3 = htonl (pSmDesc->intArg3); /* interrupt argument #3 */
    pCpuDesc->status  = htonl (SM_CPU_ATTACHED);  /* mark cpu as attached */

    pSmDesc->status = SM_CPU_ATTACHED;          /* also mark sh mem descr */

    CACHE_PIPE_FLUSH ();			/* FLUSH BUFFER  [SPR 68334] */
    cpuNum = pCpuDesc->intArg1;			/* BRIDGE FLUSH  [SPR 68334] */

    return (OK);                                /* attach complete */
    }


/******************************************************************************
*
* smDetach - detach CPU from shared memory
*
* This routine ends the "attachment" between the calling CPU and
* the shared memory area specified by <pSmDesc>.  No further shared
* memory operations may be performed until a subsequent smAttach()
* is completed.
*
* RETURNS: OK, or ERROR.
*
* ERRNO: S_smLib_NOT_ATTACHED
*/

STATUS smDetach
    (
    SM_DESC *	pSmDesc       /* ptr to shared mem descriptor */
    )
    {
    SM_CPU_DESC volatile * pCpuDesc;  /* ptr to cpu descriptor in sh mem hdr*/
    int			   temp;      /* temp value */

    /* Check that this cpu is connected to shared memory */

    if (pSmDesc->status != SM_CPU_ATTACHED)
        {
        errno = S_smLib_NOT_ATTACHED;
        return (ERROR);                         /* local cpu is not attached */
        }

    /* get addr of cpu descriptor */

    pCpuDesc = (SM_CPU_DESC volatile *) &((pSmDesc->cpuTblLocalAdrs) \
                                           [pSmDesc->cpuNum]);

    pCpuDesc->status = htonl (SM_CPU_NOT_ATTACHED);/* mark as not attached */
    pSmDesc->status  = SM_CPU_NOT_ATTACHED;       /* also mark sh mem descr */

    CACHE_PIPE_FLUSH ();			/* FLUSH BUFFER  [SPR 68334] */
    temp = pCpuDesc->status;			/* BRIDGE FLUSH  [SPR 68334] */

    return (OK);
    }


/******************************************************************************
*
* smInfoGet - get current status information about shared memory
*
* This routine obtains various pieces of information which describe the
* current state of the shared memory area specified by <pSmDesc>.  The
* current information is returned in a special data structure, SM_INFO,
* whose address is specified by <pInfo>.  The structure must have been
* allocated before this routine is called.
*
* RETURNS: OK, or ERROR if this CPU is not attached to shared memory area.
*
* ERRNO: S_smLib_NOT_ATTACHED
*/

STATUS smInfoGet
    (
    SM_DESC *	pSmDesc,	/* ptr to shared memory descriptor */
    SM_INFO *	pInfo		/* ptr to info structure to fill */
    )
    {
    SM_HDR volatile *	   pHdr;	/* ptr to shared memory header */
    SM_CPU_DESC volatile * pCpuDesc;	/* ptr to cpu descriptor */
    int			   ix;		/* index variable */
    int			   temp;        /* temp value */

    /* Check that this cpu is connected to shared memory */

    if (pSmDesc->status != SM_CPU_ATTACHED)
	{
	errno = S_smLib_NOT_ATTACHED;
	return (ERROR);				/* local cpu is not attached */
	}

    /* Get protocol version number */

    CACHE_PIPE_FLUSH ();			/* FLUSH BUFFER  [SPR 68334] */
    temp = pSmDesc->anchorLocalAdrs->version;	/* PCI bridge bug [SPR 68844]*/

    pInfo->version = ntohl (pSmDesc->anchorLocalAdrs->version);


    /* Get info from shared memory header */

    pHdr = (SM_HDR volatile *) pSmDesc->headerLocalAdrs;

    pInfo->tasType     = ntohl (pHdr->tasType);	   /* test-and-set method */
    pInfo->maxCpus     = ntohl (pHdr->maxCpus);	   /* max number of cpu's */

    /* Get count of attached cpu's starting with first cpu table entry */

    pCpuDesc = (SM_CPU_DESC volatile *) pSmDesc->cpuTblLocalAdrs;
    pInfo->attachedCpus = 0;

    for (ix = 0;  ix < pInfo->maxCpus;  ix++)
	{
	if (ntohl (pCpuDesc->status) == SM_CPU_ATTACHED)
	    pInfo->attachedCpus++;

	pCpuDesc++;				/* next entry */
	}

    return (OK);
    }


/******************************************************************************
*
* smCpuInfoGet - get information about a single CPU using shared memory
*
* This routine obtains a variety of information describing the CPU specified
* by <cpuNum>.  If <cpuNum> is NONE (-1), this routine returns information
* about the local (calling) CPU.  The information is returned in a special
* structure, SM_CPU_INFO, whose address is specified by <pCpuInfo>.  (The
* structure must have been allocated before this routine is called.)
*
* RETURNS: OK, or ERROR.
*
* ERRNO: S_smLib_NOT_ATTACHED, S_smLib_INVALID_CPU_NUMBER
*/

STATUS smCpuInfoGet
    (
    SM_DESC *		pSmDesc,	/* ptr to shared memory descriptor */
    int			cpuNum,		/* number of cpu to get info about */
    SM_CPU_INFO *	pCpuInfo	/* cpu info structure to fill */
    )
    {
    SM_CPU_DESC volatile * pCpuDesc;	/* ptr to cpu descriptor */
    int			   temp;        /* temp value */

    /* Check that the local cpu is connected to shared memory */

    if (pSmDesc->status != SM_CPU_ATTACHED)
	{
	errno = S_smLib_NOT_ATTACHED;
	return (ERROR);				/* local cpu is not attached */
	}

    /* Report on local cpu if NONE specified */

    if (cpuNum == NONE)
        {
	cpuNum = pSmDesc->cpuNum;
        }

    /* Get info from cpu descriptor */

    if (cpuNum < 0  ||  cpuNum >= pSmDesc->maxCpus)
	{
	errno = S_smLib_INVALID_CPU_NUMBER;
	return (ERROR);				/* cpu number out of range */
	}

    /* get address of cpu descr */

    pCpuDesc = (SM_CPU_DESC volatile *) &(pSmDesc->cpuTblLocalAdrs [cpuNum]);

    pCpuInfo->cpuNum  = cpuNum;			   /* actual cpu number */

    CACHE_PIPE_FLUSH ();			/* FLUSH BUFFER  [SPR 68334] */
    temp = pCpuDesc->status;			/* PCI bridge bug [SPR 68844]*/

    pCpuInfo->status  = ntohl (pCpuDesc->status);  /* attached/not attached */
    pCpuInfo->intType = ntohl (pCpuDesc->intType); /* interrupt type */
    pCpuInfo->intArg1 = ntohl (pCpuDesc->intArg1); /* interrupt argument #1 */
    pCpuInfo->intArg2 = ntohl (pCpuDesc->intArg2); /* interrupt argument #2 */
    pCpuInfo->intArg3 = ntohl (pCpuDesc->intArg3); /* interrupt argument #3 */

    return (OK);
    }


/******************************************************************************
*
* smLockTake - take a mutual exclusion lock
*
* This routine repeatedly attempts to obtain a mutual exclusion lock
* by using the specified test-and-set routine.  If the lock is not
* obtained within the specified number of attempts, an error is returned.
*
* If the lock was successfully taken, interrupts will be disabled when
* this routine returns and the old interrupt mask level will be copied
* to the location pointed to by <pOldLvl>.
*
* If the lock was not taken, interrupts will be enabled upon return (at
* the interrupt level which was in effect prior to calling this routine).
*
* This routine does not set errno.
*
* INTERNAL 
*
* RETURNS: OK, or ERROR if lock not taken.
*/

STATUS smLockTake
    (
    int *	lockLocalAdrs,	/* local addr of lock */
    FUNCPTR	tasRoutine,	/* test-and-set routine to use */
    int		numTries,	/* number of times to try to take lock */
    int	*	pOldLvl		/* where to put old int lvl if success */
    )
    {
    int         oldLvl;         /* previous interrupt level */
    int         delay;          /* time between tries to get lock */
    int         ix;		/* index */
    int		dummy = 0;	/* dummy counter for delay */
    volatile int dummy2;	/* dummy to avoid warning in delay */
    int         curNumTries;    /* current number of tries */
 
    /* First try to get lock. */
 
    oldLvl = intLock ();                /* lock out interrupts */
    if ((*tasRoutine) (lockLocalAdrs) == TRUE)
        {
        *pOldLvl = oldLvl;              /* pass back old int level */

	/*
	 * XXXdat These PIPE_FLUSH operations should not be needed.
	 * The tasRoutine should be taking care of the cache issues
	 * related to tas activity.
	 */

	CACHE_PIPE_FLUSH ();		/* flush write buffer */
        dummy2 = *(int *)lockLocalAdrs; /* BRIDGE FLUSH  [SPR 68334] */
        return (OK);            	/* done! */
        }
    intUnlock (oldLvl);         /* unlock interrupts before retry */
 
    /* 
     * The first try has failed so now we insert a decrementing
     * delay between each tries to reduce bus contention and deadlock.
     *
     * Set maximum delay to a different value for each processor. 
     * Note that this scheme gives a lowest priority to CPUs with
     * high processor number. A better version would implement
     * a random delay.
     */
 

    delay = smLockTakeDelayMin;
    curNumTries = 1;
 
    do
        {
        for (ix = 0; ix <= delay; ix++) /* delay loop */
	    dummy2 = dummy++;           /* volatile!! */
 
        oldLvl = intLock ();            /* lock out interrupts */
 
        if ((*tasRoutine) (lockLocalAdrs) == TRUE)
            {
            *pOldLvl = oldLvl;          /* pass back old int level */
	    CACHE_PIPE_FLUSH ();	/* flush write buffer */
            dummy2 = *(int *)lockLocalAdrs; /* BRIDGE FLUSH  [SPR 68334] */
            return (OK);                /* done! */
            }
 
        intUnlock (oldLvl);             /* unlock interrupts before retry */
 
	/* Exponential delay, with a limit */

	delay <<= 1;
	if (delay > smLockTakeDelayMax)
	    {
	    delay = smLockTakeDelayMin;
	    }

        curNumTries++;
 
        /* keep track of maximum number of attempts */

        if (curNumTries > smCurMaxTries)
            {
            smCurMaxTries = curNumTries;
            }
 
        } while (--numTries > 0);       /* do for spec'd number of tries */
 
    return (ERROR);                     /* cannot take lock */
    }


/******************************************************************************
*
* smLockGive - give a mutual exclusion lock
*
* This routine gives up a mutual exclusion lock taken previously by smLockTake.
*
* RETURNS: N/A
*/

void smLockGive
    (
    int *	lockLocalAdrs,		/* local address of lock */
    FUNCPTR	tasClearRoutine,	/* test and set clear routine to use */
    int		oldLvl			/* old interrupt level */
    )
    {
    int volatile * pLockv = (int volatile *) lockLocalAdrs;
    int            temp;			/* temp value */

    if (tasClearRoutine != NULL)	/* hardware test-and-set */
        {
	(*tasClearRoutine) (lockLocalAdrs);
        }
    else
        {
    	*pLockv = 0;
        }

    CACHE_PIPE_FLUSH ();		/* FLUSH BUFFER  [SPR 68334] */
    temp = *pLockv;			/* BRIDGE FLUSH  [SPR 68334] */

    intUnlock (oldLvl);
    }


/******************************************************************************
*
* smRegionFind - find a shared memory region.
*
* This routine finds the shared memory region associated with
* <anchorLocalAdrs>.
*
* RETURNS:  index of the region, or ERROR if not found
*/

LOCAL int smRegionFind
    (
    SM_ANCHOR *	anchorLocalAdrs		/* local address of anchor */
    )
    {
    int		ix;			/* index */

    for (ix = 0; (smRegions [ix].anchor != NULL) && (ix < smRegionsMax); ix++)
	{
	if (smRegions [ix].anchor == anchorLocalAdrs)
	    {
	    return (ix);			/* region already initialized*/
	    }
	}

    return (ERROR);
    }


/******************************************************************************
*
* smRegionGet - get a shared memory region
*
* smRegionGet reserves a space in the smRegion table for a shared memory
* region specified by <anchorLocalAdrs>.
*
* RETURNS: the index of the region, or ERROR if out of space.
*/

LOCAL int smRegionGet
    (
    SM_ANCHOR *	anchorLocalAdrs		/* anchor address */
    )
    {
    int		ix;			/* index */

    for (ix = 0; (smRegions [ix].anchor != NULL) && (ix < smRegionsMax); ix++)
	;

    if (ix == smRegionsMax)
	{
	printf ("Out of shared memory regions!!\n");
	errno = S_smLib_NO_REGIONS;
	return (ERROR);				/* no space in table */
	}

    smRegions [ix].anchor = anchorLocalAdrs;
    return (ix);
    }


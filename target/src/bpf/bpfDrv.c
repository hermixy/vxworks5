/* bpfDrv.c - Berkeley Packet Filter (BPF) I/O driver library */

/* Copyright 1999 - 2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*-
 * Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995, 1996
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from the Stanford/CMU enet packet filter,
 * (net/enet.c) distributed as part of 4.3BSD, and code contributed
 * to Berkeley by Steven McCanne and Van Jacobson both of Lawrence
 * Berkeley Laboratory.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)bpf.c	7.5 (Berkeley) 7/15/91
 *
 * static char rcsid[] =
 * "$Header: bpf.c,v 1.67 96/09/26 22:00:52 leres Exp $";
 */

/*
modification history
--------------------
01f,07may02,wap  Correctly initialize filter length (SPR #74358)
01e,26mar02,wap  remember to call _bpfDevUnlock() when bpfIfSet() exits with
                 error (SPR #74039)
01d,17nov00,spm  added support for BSD Ethernet devices
01c,24apr00,spm  updated to use NPT version of MUX cookie
01b,05apr00,spm  added interfaces to suspend filtering and remove I/O device
01a,24nov99,spm  created from BSD bpf.c,v 1.67 96/09/26 22:00:52
*/

/*
DESCRIPTION
This library provides a driver which supports the customized retrieval of
incoming network data that meets the criteria imposed by a user-specified
filter.

USER-CALLABLE ROUTINES
The bpfDrv() routine initializes the driver and the bpfDevCreate() routine
creates a packet filter device. Each BPF device allows direct access to the
incoming data from one or more network interfaces.

CREATING BPF DEVICES
In order to retrieve incoming network data, a BPF device must be created by
calling the bpfDevCreate() routine:
.CS
    STATUS bpfDevCreate
        (
        char *  pDevName,       /@ I/O system device name @/
        int     numUnits,       /@ number of device units @/
        int     bufSize         /@ block size for the BPF device @/
        )
.CE

The <numUnits> parameter specifies the maximum number of BPF units for
the device. Each unit is accessed through a separate file descriptor
for use with a unique filter and/or a different network interface. For
example, the following call creates the "/bpf0" and "/bpf1" units:
.CS
    bpfDevCreate ("/bpf", 2, 4096);
.CE

CONFIGURING BPF DEVICES
After opening a device unit, the associated file descriptor must be bound to
a specific network interface with the BIOCSETIF ioctl() option. The BIOCSETF
ioctl() option adds any filter instructions. Each file descriptor receives
a copy of any data which matches the filter. Different file descriptors may
share the same interface. The underlying filters will receive an identical
data stream.

IOCTL FUNCTIONS
The BPF driver supports the following ioctl() functions:

NOTE
When reading data from BPF units, the supplied buffer must be able to
accept an entire block of data as defined by the <bufSize> parameter to
the bpfDevCreate() routine. That value is also available with the BIOCGBLEN
ioctl() option described above.

INCLUDE FILES: bpfDrv.h

SEE ALSO: ioLib
*/

/* includes */

#include "vxWorks.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "ctype.h" 	/* isdigit() declaration */

#include "iosLib.h"
#include "taskLib.h"
#include "semLib.h"
#include "tickLib.h"
#include "errnoLib.h"
#include "objLib.h"
#include "sysLib.h"

#include "net/if.h"

#include "netLib.h"

#include "net/bpf.h"
#include "private/bpfLibP.h"

#include "private/muxLibP.h"    /* PCOOKIE_TO_ENDOBJ conversion macro */

/* defines */

#define DEBUG

#define ROTATE_BUFFERS(pBpfDev) \
     (pBpfDev)->readHoldBuf = (pBpfDev)->recvStoreBuf; \
     (pBpfDev)->readHoldLen = (pBpfDev)->recvStoreLen; \
     (pBpfDev)->recvStoreBuf = (pBpfDev)->freeBuf;     \
     (pBpfDev)->recvStoreLen = 0;                      \
     (pBpfDev)->freeBuf = NULL;

#define BPF_DEV_NAME "/dev/bpf"

#ifdef DEBUG

#undef LOCAL
#define LOCAL

#endif /* DEBUG */

#define BPF_BUFSIZE 4096         /* default BPF buffer size */   

/* globals */

/* locals */
LOCAL int 	bpfDrvNum = 0; 	 /* BPF driver number */
LOCAL int 	bpfDrvCount = 0; /* Number of BPF driver users */
LOCAL int bpfLockTaskId = 0;     /* ID of task holding global BPF lock */
LOCAL SEM_ID bpfLockSemId;       /* Semaphore ID for global BPF lock */

LOCAL int          numBpfDevs;   /* The number of allocated BPF devices */
LOCAL BPF_DEV_CTRL * pBpfDevTbl; /* BPF device descriptor table */

LOCAL UINT         bpfBufSize = BPF_BUFSIZE;   /* default BPF buffer size */

/* forward declarations */

LOCAL int bpfOpen (BPF_DEV_HDR * pBpfDevHdr, char * minorDevStr, int flags);
LOCAL int bpfClose (BPF_DEV_CTRL * pBpfDev);
LOCAL int bpfRead (BPF_DEV_CTRL * pBpfDev, char * pBuf, int nBytes);
LOCAL int bpfWrite (BPF_DEV_CTRL * pBpfDev, char * pBuf, int nBytes);
LOCAL STATUS bpfIoctl (BPF_DEV_CTRL * pBpfDev, int request, void * addr);
LOCAL void bpfWakeup (BPF_DEV_CTRL * pBpfDev);
LOCAL int bpfSleep (BPF_DEV_CTRL * pBpfDev, int numTicks);
LOCAL void bpfTimeStamp (struct timeval * pTimeval);
LOCAL STATUS bpfBufsAlloc (BPF_DEV_CTRL * pBpfDev);
LOCAL void bpfDevReset (BPF_DEV_CTRL * pBpfDev);
LOCAL STATUS bpfIfSet (BPF_DEV_CTRL * pBpfDev, struct ifreq * pIfReq, 
                       BOOL restartFlag);
LOCAL void bpfSelectAdd (BPF_DEV_CTRL *, SEL_WAKEUP_NODE *);
LOCAL void bpfSelectDelete (BPF_DEV_CTRL *, SEL_WAKEUP_NODE *);
LOCAL int bpfSetFilter (BPF_DEV_CTRL * pBpfDev, struct bpf_program * pBpfProg);
LOCAL void bpfDevReset (BPF_DEV_CTRL * pBpfDev);
LOCAL void bpfDevFree (FAST BPF_DEV_CTRL * pBpfDev);
LOCAL void bpfPacketCatch (FAST BPF_DEV_CTRL * pBpfDev, FAST M_BLK_ID pMBlk, 
                           FAST int snapLen, FAST int bpfHdrLen, 
                           struct timeval * pTimeVal, FAST int linkHdrLen);

/*******************************************************************************
*
* bpfDrv - initialize the BPF driver
* 
* This routine installs the Berkeley Packet Filter driver for access through
* the I/O system. It is required before performing any I/O operations and is
* executed automatically if INCLUDE_BPF is defined at the time the system
* is built. Subsequent calls to the routine just count the number of users
* with BPF access.
*
* RETURNS: OK, or ERROR if initialization fails.
*
* ERRNO: N/A
*/

STATUS bpfDrv (void)
    {
    /*
     * For additional calls to this installation routine,
     * just increase the number of users.
     */

    if (bpfDrvNum)
        {
        bpfDrvCount++;
        return (OK);
        }

    bpfLockSemId = semMCreate (SEM_Q_PRIORITY | SEM_DELETE_SAFE 
                               | SEM_INVERSION_SAFE);
    if (bpfLockSemId == NULL)
        {
        return (ERROR);
        }
    
    bpfDrvNum = iosDrvInstall (NULL, NULL, bpfOpen, bpfClose,
                               bpfRead, bpfWrite, bpfIoctl);
    if (bpfDrvNum == ERROR)
        {
        bpfDrvNum = 0;
        return (ERROR);
        }

    bpfDrvCount++;

    return (OK);
    }

/*******************************************************************************
*
* bpfDrvRemove - remove BPF driver support
* 
* This routine uninstalls the Berkeley Packet Filter driver, preventing
* further access through the I/O system. It may only be called once to
* reverse a particular installation with the bpfDrv initialization routine.
* It has no effect if any other users retain BPF access through a separate
* installation. The driver is also not removed if any BPF file descriptors
* are currently open.
*
* RETURNS: OK, or ERROR if removal fails.
*
* ERRNO: S_ioLib_NO_DRIVER
*
* NOMANUAL
*/

STATUS bpfDrvRemove (void)
    {
    STATUS result;

    if (bpfDrvNum == 0)
        {
        errnoSet (S_ioLib_NO_DRIVER);
        return (ERROR);
        }

    /* Decrease the number of users. Attempt removal if zero. */

    bpfDrvCount--;

    if (bpfDrvCount > 0)     /* Other users have access - do not remove. */
        return (OK);

    if (bpfDrvCount < 0)     /* Mismatched calls to remove routine. */
        {
        bpfDrvCount = 0;
        return (ERROR);
        }

    /* Remove driver if no open files exist. */

    result = iosDrvRemove (bpfDrvNum, FALSE);
    if (result == ERROR) 	/* Error: open files found.*/
        {
        bpfDrvCount = 1;     /* Allows removal attempt after closing files. */
        return (ERROR);
        }

    /* Driver removed from I/O system. Remove global lock and reset number. */

    semDelete (bpfLockSemId);

    bpfDrvNum = 0;

    return (OK);
    }

/*******************************************************************************
*
* bpfDevCreate - create Berkeley Packet Filter device
*
* This routine creates a Berkeley Packet Filter device. Each of the
* <numUnits> units corresponds to a single available file descriptor for
* monitoring a network device. The <pDevName> parameter provides the name
* of the BPF device to the I/O system. The default name of "/bpf" (assigned
* if <pDevName> is NULL) produces units named "/bpf0", "/bpf1", etc., up to
* the <numUnits> limit.
*
* RETURNS: OK, or ERROR if device creation failed.
*
* ERRNO: S_ioLib_NO_DRIVER
*/

STATUS bpfDevCreate
    (
    char * 	pDevName, 	/* I/O system device name */
    int 	numUnits,       /* number of device units */
    int 	bufSize 	/* BPF device block size (0 for default) */
    )
    {
    BPF_DEV_HDR * pBpfDevHdr;   /* I/O system device header for BPF units */

    if (bpfDrvNum == 0)
        {
        errnoSet (S_ioLib_NO_DRIVER);
        return (ERROR);
        }

    if (numUnits <= 0)
        {
        return (ERROR);
        }

    pBpfDevHdr = (BPF_DEV_HDR *)malloc (sizeof (BPF_DEV_HDR));
    if (pBpfDevHdr == NULL)
        {
        return (ERROR);
        }

    pBpfDevHdr->lockSem = semBCreate (SEM_Q_FIFO, SEM_FULL);
    if (pBpfDevHdr->lockSem == NULL)
        {
        free (pBpfDevHdr);
        return (ERROR);
        }

    pBpfDevHdr->pBpfDevTbl = calloc (numUnits, sizeof (BPF_DEV_CTRL));
    if (pBpfDevHdr->pBpfDevTbl == NULL)
        {
        free (pBpfDevHdr);
        semDelete (pBpfDevHdr->lockSem);
        return (ERROR);
        }

    pBpfDevHdr->pBpfNetIfTbl = calloc (numUnits, sizeof (BPF_PROTO_CTRL));
    if (pBpfDevHdr->pBpfDevTbl == NULL)
        {
        free (pBpfDevHdr->pBpfDevTbl); 
        free (pBpfDevHdr);
        semDelete (pBpfDevHdr->lockSem);
        return (ERROR);
        }

    if (pDevName == NULL)
        pDevName = BPF_DEV_NAME;

    if (iosDevAdd ( (DEV_HDR *) pBpfDevHdr, pDevName, bpfDrvNum) != OK)
        {
        free (pBpfDevHdr->pBpfNetIfTbl);
        free (pBpfDevHdr->pBpfDevTbl);
        free (pBpfDevHdr);
        semDelete (pBpfDevHdr->lockSem);
        return (ERROR);
        }

    pBpfDevHdr->numDevs = numUnits;
    pBpfDevHdr->bufSize = (bufSize <= 0) ? bpfBufSize : bufSize;

    /* Initialize attachment to network interfaces. */

    _bpfProtoInit ();

    return (OK);
    }

/*******************************************************************************
*
* bpfDevDelete - destroy Berkeley Packet Filter device
*
* This routine removes a Berkeley Packet Filter device and releases all
* allocated memory. It will close any open files using the device.
*
* RETURNS: OK, or ERROR if device not found
*
* ERRNO: S_ioLib_NO_DRIVER
*
* INTERNAL
* This routine reverses the bpfDevCreate() routine operations.
*/

STATUS bpfDevDelete
    (
    char * 	pDevName 	/* name of BPF device to remove */
    )
    {
    char * 	pTail = NULL; 	/* Pointer to tail of device name. */
    BPF_DEV_HDR * pBpfDev; 	/* I/O system device header for BPF units */

    if (bpfDrvNum == 0)
        {
        errnoSet (S_ioLib_NO_DRIVER);
        return (ERROR);
        }

    pBpfDev = (BPF_DEV_HDR *)iosDevFind (pDevName, &pTail);
    if (pBpfDev == NULL || pTail == pDevName) 	/* Device not found? */
        return (ERROR);

    /* Close any open files and remove the BPF device. */

    iosDevDelete ( (DEV_HDR *)pBpfDev);

    /*
     * The I/O system device header is the first element in the overall
     * BPF device header. Remove all other data structures for the device.
     */

    free (pBpfDev->pBpfNetIfTbl);

    free (pBpfDev->pBpfDevTbl);

    semDelete (pBpfDev->lockSem);

    free (pBpfDev);

    return (OK);
    }

/*******************************************************************************
*
* _bpfLock - provides mutual exclusion for the BPF driver
*
* This routine protects critical sections of the BPF code by obtaining
* a global lock which prevents additional BPF operations by any BPF units
* through their associated file descriptors.
*
* RETURNS:
* BPF_LOCK_KEY  - if the BPF lock is newly acquired
* BPF_DUMMY_KEY - if the calling task already owns the BPF_LOCK_KEY
*
* NOMANUAL
*/

int _bpfLock (void)
    {
    int taskIdCurrent = taskIdSelf ();  /* ID of current task */
    
    /* If the task already owns the lock return a dummy key */

    if (bpfLockTaskId == taskIdCurrent)
	{
	return (BPF_DUMMY_KEY);
	}

    if (semTake (bpfLockSemId, WAIT_FOREVER) != OK)
	{
	panic ("bpfLock error");
	}
    
    bpfLockTaskId = taskIdCurrent;
    return (BPF_LOCK_KEY);
    }

/*******************************************************************************
*
* _bpfUnlock - release the mutual exclusion for the BPF driver
*
* This routine permits parallel BPF operations by any BPF units through
* their associated file descriptors. It releases the lock previously acquired
* by _bpfLock.  The routine has no effect if <lockKey> equals BPF_DUMMY_KEY
* because of redundant lock attempts. If <lockKey> equals BPF_LOCK_KEY then
* the lock will be released if the calling task currently owns the BPF lock.
*
* RETURNS:
* OK or ERROR if the calling task does not own the BPF lock
*
* NOMANUAL
*/
 
STATUS _bpfUnlock
    (
    int lockKey         /* BPF_LOCK_KEY or BPF_DUMMY_KEY */
    )
    {
    int taskIdCurrent;  /* ID of current task */

    if (lockKey == BPF_DUMMY_KEY)
	{
	return (OK);
	}

    taskIdCurrent = taskIdSelf ();

    /* Calling task must own the BPF lock in order to release it */

    if (taskIdCurrent != bpfLockTaskId)
	{
	return (ERROR);
	}

    bpfLockTaskId = 0;
    semGive (bpfLockSemId);
    return (OK);
    }

/*******************************************************************************
*
* _bpfDevLock - provides mutual exclusion for a BPF device
*
* This routine protects shared data structures in the BPF code by obtaining
* a lock which prevents BPF operations by any BPF units associated
* with the same I/O device. This lock primarily supports stable "snapshots"
* of the list of BPF units from a device attached to a particular network
* interface. It also guarantees that a particular BPF unit will only be
* available through a single file descriptor at a time.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void _bpfDevLock
    (
    UINT32 devId 	/* Unique identifier for BPF I/O device. */
    )
    {
    BPF_DEV_HDR * pBpfDevHdr;     /* BPF device header */

    pBpfDevHdr = (BPF_DEV_HDR *)devId;

    if (semTake (pBpfDevHdr->lockSem, WAIT_FOREVER) != OK)
        {
        panic ("bpfDevLock error");
        }

    return;
    }

/*******************************************************************************
*
* _bpfDevUnlock - release the mutual exclusion for a BPF device
*
* This routine permits parallel BPF operations by any BPF units within
* a particular BPF I/O device. It releases the lock previously acquired
* by the _bpfDevLock routine.
*
* RETURNS: N/A
*
* NOMANUAL
*/
 
void _bpfDevUnlock
    (
    UINT32 devId 	/* Unique identifier for BPF I/O device. */
    )
    {
    BPF_DEV_HDR * pBpfDevHdr;     /* BPF device header */

    pBpfDevHdr = (BPF_DEV_HDR *)devId;

    semGive (pBpfDevHdr->lockSem);

    return;
    }

/*******************************************************************************
*
* _bpfUnitLock - provides mutual exclusion for a BPF file descriptor
*
* This routine protects shared data structures in the BPF code by obtaining
* a lock which prevents BPF operations by a specific BPF unit through its
* associated file descriptor. This lock primarily prevents alterations to
* settings (such as the buffer size) which are currently in use by a task.
* It also guarantees that a particular BPF unit will only be available 
* through a single file descriptor at a time.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void _bpfUnitLock
    (
    BPF_DEV_CTRL * pBpfDev 	/* Control structure for open BPF unit */
    )
    {
    if (semTake (pBpfDev->lockSem, WAIT_FOREVER) != OK)
        {
        panic ("bpfUnitLock error");
        }

    return;
    }

/*******************************************************************************
*
* _bpfUnitUnlock - release the mutual exclusion for a BPF file descriptor
*
* This routine permits parallel BPF operations for a BPF unit associated
* with a single file descriptor. It releases the lock previously acquired
* by the _bpfUnitLock routine.
*
* RETURNS: N/A
*
* NOMANUAL
*/
 
void _bpfUnitUnlock
    (
    BPF_DEV_CTRL * pBpfDev 	/* Control structure for open BPF unit */
    )
    {
    semGive (pBpfDev->lockSem);

    return;
    }

/*******************************************************************************
*
* _bpfDevDetach - detach a BPF device from the network interface
*
* This routine detaches the device from the attached network interface.  If the
* BPF device requested promiscous mode it will disable promiscuous mode for the
* interface.  If no units of the BPF device remain attached to the network 
* interface, then this routine will remove the BPF MUX protocol as well.
*
* RETURNS:
* OK or ERROR
*
* CAVEATS:
* Must be called by a task which holds the device-specific _bpfDevLock
* and the unit-specific _bpfUnitLock.
*
* NOMANUAL
*/

STATUS _bpfDevDetach
    (
    BPF_DEV_CTRL * pBpfDev   /* Device control structure for a BPF unit */
    )
    {
    int lockKey; 	/* BPF global lock */

    LIST * pBpfUnitList;      /* list of BPF units attached to the interface */

    pBpfUnitList = & (pBpfDev->pNetDev->bpfUnitList);

    /* Disable promiscuous mode if established for this unit. */

    if (pBpfDev->promiscMode)
        {
        pBpfDev->promiscMode = FALSE;
        lockKey = _bpfLock ();
        _bpfProtoPromisc (pBpfDev->pNetDev, FALSE);
        _bpfUnlock (lockKey);
        }

    /* Remove the unit from the attachment list for the BPF device. */

    lstDelete (pBpfUnitList, (NODE *) pBpfDev);

    /* 
     * Detach this instance of the BPF MUX protocol if no more
     * units from this device are using the network interface.
     * Remove the Ethernet input hook if no more units from any
     * BPF device are using any BSD network interface.
     */

    if (lstCount (pBpfUnitList) == 0)
        {
        lockKey = _bpfLock ();
        _bpfProtoDetach (pBpfDev);
        _bpfUnlock (lockKey);
        }

    pBpfDev->pNetDev = NULL;

    return (OK);
    }

/*******************************************************************************
*
* _bpfPacketTap - process packets
*
* This routine processes packets for BPF devices.  For each BPF device on the
* list <pListBpfDev>, the device specific packet filter is applied.  If the
* filter accepts the packet then it is copied to the device's buffer. That
* list has at least one entry or the BPF MUX protocol would never execute 
* this routine.
*
* RETURNS:
* Not Applicable
*
* NOMANUAL
*/

void _bpfPacketTap
    (
    LIST * 	pListBpfDev, 	/* BPF units attached to network interface */
    long 	type, 		/* MUX encoding of frame type */
    M_BLK_ID 	pMBlk, 		/* M_BLK chain containing the packet */
    int 	netDataOffset 	/* offset to the start of frame data */
    )
    {
    struct timeval   timeStamp;   /* Timestamp of packet arrival */
    int snapLen;                  /* length of packet data to store */ 
    int bpfHdrLen;                /* size of the BPF hdr + padding */
    BPF_DEV_CTRL * pBpfDev = NULL;    /* BPF device */
    BPF_DEV_CTRL * pStart = NULL;

    bpfTimeStamp (&timeStamp);
    
    /* 
     * Determine the size of the BPF header plus padding so that the
     * network header is long word aligned 
     */

    bpfHdrLen = BPF_WORDALIGN (netDataOffset + sizeof (struct bpf_hdr))
                    - netDataOffset;

    pBpfDev = pStart = (BPF_DEV_CTRL *)lstFirst (pListBpfDev);
    if (pBpfDev == NULL)
        {
        /*
         * Last attached unit for this BPF device was just removed
         * from the network interface.
         */

        return;
        }

    _bpfDevLock (pBpfDev->bpfDevId);    /*
                                         * Protect list of attached BPF units
                                         * while applying filters.
                                         */

    for ( ; pBpfDev != NULL;
            pBpfDev = (BPF_DEV_CTRL *) lstNext ((NODE *) pBpfDev))
        {
        _bpfUnitLock (pBpfDev);  /* Prevent changing filter or buffer size. */
        pBpfDev->pktRecvCount++;
        snapLen = bpf_filter (pBpfDev->pBpfInsnFilter, (u_char *) pMBlk,
                              pMBlk->mBlkPktHdr.len, 0, type, netDataOffset);
        if (snapLen != 0)
            {
            /* Filter accepted the frame. Copy it to the BPF unit's buffer. */

            bpfPacketCatch (pBpfDev, pMBlk, snapLen, bpfHdrLen, 
                            &timeStamp, netDataOffset);
            }
        _bpfUnitUnlock (pBpfDev);
        }

    _bpfDevUnlock (pStart->bpfDevId);

    return;
    }

/*******************************************************************************
*
* bpfOpen - BPF specific open routine 
*
* This routine opens a unit for a BPF device. It initializes a BPF_DEV_CTRL
* structure. All subsequent I/O operations will supply a pointer to that
* control structure through the file descriptor.
*
* RETURNS:
* BPF_DEV_CTRL * - if open is successful
* ERROR          - if open fails
*
* ERRNO:
* S_errno_ENXIO  - if minor device number is invalid
* S_errno_EBUSY  - if minor device is already in use
* S_errno_ENOMEM - insufficient memory for device
*
* NOMANUAL
*/ 

LOCAL int bpfOpen
    (
    BPF_DEV_HDR * pBpfDevHdr,    /* BPF device hdr (unused) */
    char        * minorDevStr,   /* string containing device unit number */
    int         flags            /* user open flags */
    )
    {
    FAST BPF_DEV_CTRL *	pBpfDev = NULL; 	/* BPF device to open */
    SEM_ID 			readSem; 	/* Trigger for incoming data. */
    int 			minorDevNum;    /* unit number for device */

    /* Convert string to int and verify it contains a valid unit number */

    if ( (minorDevStr == NULL) || 
        ( (minorDevNum = atoi (minorDevStr)) < 0)
         || (minorDevNum >= pBpfDevHdr->numDevs) )
        {
        netErrnoSet (ENXIO);
        return (ERROR);
        }

    readSem = semBCreate (SEM_Q_FIFO, SEM_EMPTY);
    if (readSem == NULL)
        {
        netErrnoSet (ENOMEM);
        return (ERROR);
        }

    pBpfDev = pBpfDevHdr->pBpfDevTbl + minorDevNum;
    if (pBpfDev == NULL)     /* Device control header vanished!! */
        {
        semDelete (readSem);
        netErrnoSet (ENOMEM);
        return (ERROR);
        }

    /*
     * Check if this BPF unit is already in use. A device lock is required
     * for this test since the semaphore which provides unit locks will not
     * exist if it is currently unused.
     */

    _bpfDevLock ( (UINT32)pBpfDevHdr);

    if (pBpfDev->inUse)
        {
        _bpfDevUnlock ( (UINT32)pBpfDevHdr);
        semDelete (readSem);
        netErrnoSet (EBUSY);
        return (ERROR);
        }

    /* Initialize device structure. */

    bzero ( (char *)pBpfDev, sizeof (BPF_DEV_CTRL));

    pBpfDev->lockSem = semBCreate (SEM_Q_FIFO, SEM_FULL);
    if (pBpfDev->lockSem == NULL)
        {
        _bpfDevUnlock ( (UINT32)pBpfDevHdr);
        semDelete (readSem);
        netErrnoSet (ENOMEM);
        return (ERROR);
        }

    pBpfDev->inUse = TRUE;

    _bpfDevUnlock ( (UINT32)pBpfDevHdr);

    pBpfDev->usrFlags = flags;
    pBpfDev->bufSize = pBpfDevHdr->bufSize;
    pBpfDev->readWkupSem = readSem;
    pBpfDev->readTimeout = WAIT_FOREVER;

    /* XXX - this uses semMInit() routine internally!  PD Violation??? */

    selWakeupListInit (& (pBpfDev->selWkupList));

    /*
     * Set the element for attaching to a network interface. It is only
     * used if this BPF unit is the first attachment to the network
     * interface from the parent BPF device.
     */

    pBpfDev->pNetIfCtrl = pBpfDevHdr->pBpfNetIfTbl + minorDevNum;

    /*
     * Attaching a BPF unit to a network interface requires a value which
     * uniquely identifies the parent device. This field also provides
     * mutual exclusion on a device-specific basis when necessary.
     */

    pBpfDev->bpfDevId = (UINT32)pBpfDevHdr;

    return ((int) pBpfDev);
    }

/*******************************************************************************
*
* bpfClose - BPF specific close routine 
*
* This routine closes a BPF device.  The steps are the following:
* 1) Detach from the attached network interface if any. 
* 2) Free any allocated memory for store buffers and the filter.
* 3) Mark the device as being unused.
*
* RETURNS:
* OK or ERROR
*
* ERRNO:
*
* NOMANUAL
*/ 

LOCAL STATUS bpfClose
    (
    BPF_DEV_CTRL * pBpfDev   /* BPF device control structure */
    )
    {
    _bpfUnitLock (pBpfDev);

    if (pBpfDev->pNetDev != NULL)
        {
        /* Detach from the network interface */

        _bpfDevLock (pBpfDev->bpfDevId);
        (void) _bpfDevDetach (pBpfDev);
        _bpfDevUnlock (pBpfDev->bpfDevId);
        }

    /* Release the buffers and mark the unit available. */

    bpfDevFree (pBpfDev);

    /* Remove the mutex semaphore for the BPF unit instead of unlocking. */

    semDelete (pBpfDev->lockSem);

    return (OK);
    }

/*******************************************************************************
*
* bpfRead - read next block of packets from device buffers 
*
* This routine reads the next block of packets from the BPF device store 
* buffers.  The call will return immeadiately if using non-Blocking mode.  If
* in immediate mode bpfRead will return when the timeout expires or when a
* packet arrives.  In normal mode, the routine returns when the timeout
* expires or when the storage buffer is filled.
*
* RETURNS: 
* ERROR or the number of bytes read
*
* ERRNOS:
* S_errno_EINVAL      - <nBytes> is not equal to the size of the BPF buffer
* S_errno_EWOULDBLOCK - no packets are available and non-Blocking mode is on
*
* NOMANUAL
*/ 

LOCAL int bpfRead
    (
    BPF_DEV_CTRL * pBpfDev,   /* BPF device control structure */
    char         * pBuf,      /* user data buffer */
    int          nBytes       /* number of bytes to read */
    )
    {
    int error = OK;           /* return value errorno code */

    _bpfUnitLock (pBpfDev);

    /*
     * Restrict application to use a buffer the same size as
     * the internal BPF buffers.
     */

    if (nBytes != pBpfDev->bufSize)
        {
        _bpfUnitUnlock (pBpfDev);
        netErrnoSet (EINVAL);
        return (ERROR);
        }

    /*
     * If the hold buffer is empty, then do a timed sleep, which
     * ends when the timeout expires or when enough packets
     * have arrived to fill the store buffer.
     */

    while (pBpfDev->readHoldBuf == NULL)
        {
        if (pBpfDev->nonBlockMode)
            {
            if (pBpfDev->recvStoreLen == 0)
                {
                _bpfUnitUnlock (pBpfDev);
                netErrnoSet (EWOULDBLOCK);
                return (ERROR);
                }
            ROTATE_BUFFERS(pBpfDev);
            break;
            }
        if (pBpfDev->immediateMode && pBpfDev->recvStoreLen != 0)
            {
            /*
             * A packet(s) either arrived since the previous
             * read or arrived while we were asleep.
             * Rotate the buffers and return what's here.
             */

            ROTATE_BUFFERS(pBpfDev);
            break;
            }

        _bpfUnitUnlock (pBpfDev);

        error = bpfSleep (pBpfDev, pBpfDev->readTimeout);

        _bpfUnitLock (pBpfDev);

        if (error != OK) 
            {
            if (error == EWOULDBLOCK) 
                {
                /*
                 * On a timeout, return what's in the buffer,
                 * which may be empty. If new data is available,
                 * swap the buffers to retrieve it.
                 */

                if (pBpfDev->readHoldBuf != NULL)
                    {
                    /*
                     * The read buffer is now available. The receive buffer
                     * must have filled while sleeping and the buffers were
                     * rotated then.
                     */

                    break;
                    }

                if (pBpfDev->recvStoreLen == 0)
                    {
                    /* No data arrived while sleeping. */

                    _bpfUnitUnlock (pBpfDev);
                    return (0);
                    }
                ROTATE_BUFFERS(pBpfDev);
                break;
                }

            /* Fatal error occurred while sleeping. */

            _bpfUnitUnlock (pBpfDev);
            netErrnoSet (error);
            return (ERROR);
            }
        }
    
    /*
     * Move data from the read buffer into user space. The entire buffer
     * is transferred since the target buffer is <pBpfDev>->bufSize bytes,
     * so rotate the buffers appropriately.
     */

    nBytes = pBpfDev->readHoldLen;
    bcopy (pBpfDev->readHoldBuf, pBuf, nBytes);

    pBpfDev->freeBuf = pBpfDev->readHoldBuf;
    pBpfDev->readHoldBuf = NULL;
    pBpfDev->readHoldLen = 0;

    _bpfUnitUnlock (pBpfDev);
    
    return (nBytes);
    }

/*******************************************************************************
*
* bpfWrite - outputs a packet directly to a network interface
*
* This routine takes a fully formed packet, including any data link layer
* header required by the device, and outputs it on the attached network
* interface.
*
* RETURNS:
* ERROR or the number of bytes written to the interface 
*
* ERRNOS:
* S_errno_ENXIO  - if the BPF device is not attached to a network interface
*
* NOMANUAL
*/

LOCAL int bpfWrite
    (
    BPF_DEV_CTRL * pBpfDev,   /* BPF device control structure */
    char         * pBuf,      /* user data buffer */
    int          nBytes       /* number of bytes to read */
    )
    {

    /* Must be attached to a network interface */

    if (pBpfDev->pNetIfCtrl == NULL)
        {
        netErrnoSet (ENXIO);
        return (ERROR);
        }

    if (_bpfProtoSend (pBpfDev->pNetIfCtrl, pBuf, nBytes,
                       pBpfDev->nonBlockMode) != OK)
        {
        return (ERROR);
        }

    return (nBytes);
    }

/*******************************************************************************
*
* bpfIoctl - implements BPF specific I/O control commands 
*
* This routine implements the following I/O control commands for BPF devices:
*
*  FIONREAD		Check for read packet available.
*  BIOCGBLEN		Get buffer len [for read()].
*  BIOCSETF		Set ethernet read filter.
*  BIOCFLUSH		Flush read packet buffer.
*  BIOCPROMISC		Put interface into promiscuous mode.
*  BIOCGDLT		Get link layer type.
*  BIOCGETIF		Get interface name.
*  BIOCSETIF		Set interface.
*  BIOCSRTIMEOUT	Set read timeout.
*  BIOCGRTIMEOUT	Get read timeout.
*  BIOCGSTATS		Get packet stats.
*  BIOCIMMEDIATE	Set immediate mode.
*  BIOCVERSION		Get filter language version.
*
* RETURNS:
* OK or ERROR
*
* ERRNO:
* S_errno_EINVAL - invalid command for BPF device
* S_errno_ENOMEM - insufficient system memory
* S_errno_ENOBUFS - expected buffer not found [from bpfProtoAttach() routine]
* 
* NOMANUAL
*/

LOCAL STATUS bpfIoctl
    (
    BPF_DEV_CTRL * pBpfDev,   /* BPF device control structure */
    int		 request,     /* ioctl () command */
    void         * addr       /* ioctl () command argument */
    )
    {
    int                error = OK;    /* errno value to set */
    int                lockKey;       /* BPF lock key */
    FAST int           nBytes;        /* byte count */
    struct timeval     * pTimeVal;    /* timeout value */
    UINT               mSec;          /* timeout in milliseconds */
    struct bpf_stat    * pBpfStat;    /* BPF device statistics */
    struct bpf_version * pBpfVersion; /* BPF filter version info */

    switch (request)
        {
        default:
            error = EINVAL;
            break;

        case FIONBIO:
            _bpfUnitLock (pBpfDev);
            pBpfDev->nonBlockMode = * ((BOOL *) addr);
            _bpfUnitUnlock (pBpfDev);
            break;

        case FIONREAD:
            _bpfUnitLock (pBpfDev);

            /* Retrieve amount of data in the read and immediate buffers. */

            nBytes = pBpfDev->recvStoreLen;
            if (pBpfDev->readHoldBuf != NULL)
                nBytes += pBpfDev->readHoldLen;

            _bpfUnitUnlock (pBpfDev);

            *(int *)addr = nBytes;
            break;

        case FIOSELECT:
            /* Add BPF unit to wakeup list. */

            _bpfUnitLock (pBpfDev);

            bpfSelectAdd (pBpfDev, (SEL_WAKEUP_NODE *)addr);

            _bpfUnitUnlock (pBpfDev);

            break;

        case FIOUNSELECT:
            /* Remove BPF unit from wakeup list. */

            _bpfUnitLock (pBpfDev);

            bpfSelectDelete (pBpfDev, (SEL_WAKEUP_NODE *)addr);

            _bpfUnitUnlock (pBpfDev);

            break;

        case BIOCGBLEN:
            _bpfUnitLock (pBpfDev);
     	    *(u_int *)addr = pBpfDev->bufSize;
            _bpfUnitUnlock (pBpfDev);
            break;

        case BIOCSBLEN:
            _bpfUnitLock (pBpfDev);

            /* Cannot set buffer len after attaching to network interface. */

            if (pBpfDev->pNetDev != NULL)
                {
                _bpfUnitUnlock (pBpfDev);
                error = EINVAL;
                break;
                }

            nBytes = *(u_int *)addr;

            /* New buffer size must be between maximum and minimum limits. */

            if (nBytes > BPF_MAXBUFSIZE)
                nBytes = BPF_MAXBUFSIZE;

            if (nBytes < BPF_MINBUFSIZE)
                {
                nBytes = BPF_MINBUFSIZE;
                }

            /* Determine if enough memory remains to allocate buffer. */

            if ( (nBytes = min (nBytes, memFindMax ())) < BPF_MINBUFSIZE)
                {
                _bpfUnitUnlock (pBpfDev);
                error = ENOMEM;
                break;
                }

            *(u_int *)addr = nBytes;
            pBpfDev->bufSize = nBytes;
            (void) _bpfUnitUnlock (pBpfDev);

            break;

        case BIOCSETF:
            _bpfUnitLock (pBpfDev);
            error = bpfSetFilter (pBpfDev, (struct bpf_program *)addr);
            _bpfUnitUnlock (pBpfDev);
            break;

        case BIOCFLUSH:
            _bpfUnitLock (pBpfDev);
            bpfDevReset (pBpfDev);
            _bpfUnitUnlock (pBpfDev);
            break;

        case BIOCPROMISC:
            _bpfUnitLock (pBpfDev);
            if (pBpfDev->pNetDev == NULL)
                {
                /* No interface attached yet. */

                error = EINVAL;
                }
            else if (!pBpfDev->promiscMode)
                {
                lockKey = _bpfLock ();
                error = _bpfProtoPromisc (pBpfDev->pNetDev, TRUE);
                (void) _bpfUnlock (lockKey);
                pBpfDev->promiscMode = (error == OK);
                }
            _bpfUnitUnlock (pBpfDev);
            break;

        case BIOCGDLT:
            _bpfUnitLock (pBpfDev);
            if (pBpfDev->pNetDev == NULL)
                error = EINVAL;
            else
                {
                *(u_int *)addr = pBpfDev->pNetDev->dataLinkType;
                }
            _bpfUnitUnlock (pBpfDev);

            break;

        case BIOCGETIF:
            _bpfUnitLock (pBpfDev);

            if (pBpfDev->pNetDev == NULL)
                error = EINVAL;
            else 
                {
                sprintf ( ( (struct ifreq *)addr)->ifr_name, "%s%d",
                         pBpfDev->pNetDev->devName,
                         pBpfDev->pNetDev->devUnit);
                }
            _bpfUnitUnlock (pBpfDev);
            break;

        case BIOCSETIF:
            _bpfUnitLock (pBpfDev);
            error = bpfIfSet (pBpfDev, (struct ifreq *)addr, FALSE);
            _bpfUnitUnlock (pBpfDev);
            break;

        case BIOCSTOP: 		/* Detach device from input stream. */
            _bpfUnitLock (pBpfDev);

            /* Do nothing if device is not attached. */

            if (pBpfDev->pNetDev == NULL)
                {
                _bpfUnitUnlock (pBpfDev);
                break;
                }

            _bpfDevLock (pBpfDev->bpfDevId);
            _bpfDevDetach (pBpfDev);
            _bpfDevUnlock (pBpfDev->bpfDevId);

            _bpfUnitUnlock (pBpfDev);
            break;

        case BIOCSTART:		/* Reattach device to input stream. */
            _bpfUnitLock (pBpfDev);

            /* Do nothing if device is already attached. */

            if (pBpfDev->pNetDev != NULL)
                {
                _bpfUnitUnlock (pBpfDev);
                break;
                }

            error = bpfIfSet (pBpfDev, (struct ifreq *)addr, TRUE);
            _bpfUnitUnlock (pBpfDev);
            break;

        case BIOCSRTIMEOUT:
            pTimeVal = (struct timeval *)addr;

            mSec = pTimeVal->tv_sec * 1000; 
            mSec += pTimeVal->tv_usec / 1000;

            _bpfUnitLock (pBpfDev);

            /* scale milliseconds to ticks */

            pBpfDev->readTimeout = (mSec * sysClkRateGet ()) / 1000;

            if (pBpfDev->readTimeout == 0)
                {
                if (pTimeVal->tv_usec == 0)
                    {
                    pBpfDev->readTimeout = WAIT_FOREVER;
                    }
                else
                    {
                    pBpfDev->readTimeout = 1;
                    }
                }

            _bpfUnitUnlock (pBpfDev);
            break;

        case BIOCGRTIMEOUT:
            pTimeVal = (struct timeval *)addr;
            _bpfUnitLock (pBpfDev);

            if (pBpfDev->readTimeout == WAIT_FOREVER)
                {
                pTimeVal->tv_sec = 0;
                pTimeVal->tv_usec = 0;
                _bpfUnitUnlock (pBpfDev);
                }
            else
                {
                mSec = (pBpfDev->readTimeout * sysClkRateGet ()) / 1000;

                pTimeVal->tv_sec = mSec / 1000;
                pTimeVal->tv_usec = mSec % 1000;
                }

            _bpfUnitUnlock (pBpfDev);
            break;

        case BIOCGSTATS:
            pBpfStat = (struct bpf_stat *)addr;
            _bpfUnitLock (pBpfDev);

            pBpfStat->bs_recv = pBpfDev->pktRecvCount;
            pBpfStat->bs_drop = pBpfDev->pktDropCount;

            _bpfUnitUnlock (pBpfDev);
            break;

        case BIOCIMMEDIATE:
            _bpfUnitLock (pBpfDev);
            pBpfDev->immediateMode = *(u_int *)addr;
            _bpfUnitUnlock (pBpfDev);
            break;

        case BIOCVERSION:
            pBpfVersion = (struct bpf_version *)addr;
            pBpfVersion->bv_major = BPF_MAJOR_VERSION;
            pBpfVersion->bv_minor = BPF_MINOR_VERSION;
            break;
        }

    if (error != OK)
        {
        netErrnoSet (error);
        return (ERROR);
        }

    return (OK);
    }

/*******************************************************************************
*
* bpfDevReset - reset BPF device
*
* This routine resets a BPF device by flushing its packet buffer and clearing 
* the receive and drop counts.  
*
* RETURNS:
* Not Applicable
*
* CAVEATS:
* Must be called by a task which holds the unit-specific _bpfUnitLock.
*
* NOMANUAL
*/

LOCAL void bpfDevReset 
    (
    BPF_DEV_CTRL * pBpfDev   /* BPF device control structure */
    )
    {
    if (pBpfDev->readHoldBuf != 0)
        {
        pBpfDev->freeBuf = pBpfDev->readHoldBuf;
        pBpfDev->readHoldBuf = 0;
        pBpfDev->readHoldLen = 0;
        }
    pBpfDev->recvStoreLen = 0;

    pBpfDev->pktRecvCount = 0;
    pBpfDev->pktDropCount = 0;
    }

/*******************************************************************************
*
* bpfSelectAdd - joins the wakeup list for select operation
*
* This routine implements the FIOSELECT operation. It adds a BPF device
* to the list of I/O devices waiting for activity through the select()
* routine. The add operation allows the bpfWakeup() routine to complete
* the selection process once sufficient data arrives.
*
* RETURNS: N/A
*
* CAVEATS:
* Must be called by a task which holds the unit-specific _bpfUnitLock.
*
* NOMANUAL
*/

LOCAL void bpfSelectAdd
    (
    BPF_DEV_CTRL * 	pBpfDev, 	/* BPF device control structure */ 
    SEL_WAKEUP_NODE * 	pWakeupNode 	/* Control information from select */
    )
    {
    /* Only pend when selecting for read requests. */

    if (selWakeupType (pWakeupNode) == SELREAD)
        {
        if (selNodeAdd (& (pBpfDev->selWkupList), pWakeupNode) == ERROR)
            return;

        pBpfDev->selectFlag = TRUE;

        /* Wake the calling task immediately if data is already available. */

        if ( (pBpfDev->readHoldBuf != NULL) ||
            (pBpfDev->immediateMode && pBpfDev->recvStoreLen != 0))
            {
            selWakeup (pWakeupNode);
            }
        }

    return;
    }

/*******************************************************************************
*
* bpfSelectDelete - leaves the wakeup list for select operation
*
* This routine implements the FIOUNSELECT control operation. It removes a
* BPF device from the list of I/O devices waiting for activity through the
* select() routine. The remove operation completes the selection process
* started with the FIOSELECT operation.
*
* RETURNS: N/A
*
* CAVEATS:
* Must be called by a task which holds the unit-specific _bpfUnitLock.
*
* NOMANUAL
*/

LOCAL void bpfSelectDelete
    (
    BPF_DEV_CTRL * 	pBpfDev, 	/* BPF device control structure */ 
    SEL_WAKEUP_NODE * 	pWakeupNode 	/* Control information from select */
    )
    {
    /* Only handle cancellations for read requests. */

    if (selWakeupType (pWakeupNode) == SELREAD)
        {
        selNodeDelete (& (pBpfDev->selWkupList), pWakeupNode);

        /*
         * A BPF unit is only accessible through a single file descriptor,
         * so the list length will usually be zero at this point. However,
         * a task may have shared that file descriptor with others. No
         * race condition occurs because this routine executes while the
         * BPF unit is locked.
         */

        if (selWakeupListLen (& (pBpfDev->selWkupList)) == 0)
            {
            pBpfDev->selectFlag = FALSE;
            }
        }

    return;
    }

/*******************************************************************************
*
* bpfSetFilter - sets the filter program used by a BPF device
*
* This routine sets the filter program to be used with a BPF device.  Before
* the filter progam is set it will be validated using the bpf_validate () 
* routine.
*
* RETURNS:
* OK
* EINVAL if the filter program is invalid
* ENOMEM if there is insufficient system memory to store the filter program
*
* CAVEATS:
* Must be called by a task which holds the unit-specific _bpfUnitLock.
*
* INTERNAL
* This routine returns errno values if a failure occurs so that the ioctl
* mechanism can set the error number appropriately.
*
* NOMANUAL
*/

LOCAL int bpfSetFilter
    (
    BPF_DEV_CTRL       * pBpfDev,   /* BPF device control structure */ 
    struct bpf_program * pBpfProg   /* BPF filter program */
    )
    {
    struct bpf_insn * pBpfInsnNew;  /* New BPF filter instructions */
    struct bpf_insn * pBpfInsnOld;  /* Previous BPF filter instructions */ 
    UINT filterLen;                 /* size in instructions of new filter */
    UINT filterSize;                /* size in bytes of new filter */
    BOOL newFlag = FALSE;           /* allocate new storage for filter? */

    pBpfInsnOld = pBpfDev->pBpfInsnFilter;

    /* If filter program contains no instructions set filter to NULL */

    if (pBpfProg->bf_insns == NULL)
        {
	/* If no filter instructions then filter instuction length must be 0 */

        if (pBpfProg->bf_len != 0)
            {
            return (EINVAL);
            }

        /* Set filter to NULL and reset device buffers */

        pBpfDev->pBpfInsnFilter = NULL;
        pBpfDev->filterLen = 0;
        bpfDevReset (pBpfDev);

        if (pBpfInsnOld != NULL)
            {
            free (pBpfInsnOld);
            return (OK);
            }
        }

    filterLen = pBpfProg->bf_len;
    if (filterLen > BPF_MAXINSNS)
        return (EINVAL);

    if (filterLen != pBpfDev->filterLen)
        newFlag = TRUE;

    pBpfDev->filterLen = filterLen;
    filterSize = filterLen * sizeof (*pBpfProg->bf_insns);

    if (newFlag)
        {
        pBpfInsnNew = (BPF_INSN *)malloc (filterSize);
        if (pBpfInsnNew == NULL)
            {
            return (ENOMEM);
            }
        }
    else
        pBpfInsnNew = pBpfInsnOld;

    /* Copy instructions from given program to internal storage */

    bcopy ((char *) pBpfProg->bf_insns, (char *) pBpfInsnNew, filterSize);

    /* 
     * Validate the filter.  Since the filter is executed as part of the
     * critical receive path, validation prevents crashing the network stack.
     */

    if (bpf_validate (pBpfInsnNew, filterLen))
        {
        pBpfDev->pBpfInsnFilter = pBpfInsnNew;

        bpfDevReset (pBpfDev);
        if (newFlag && pBpfInsnOld != NULL)
            {
            free (pBpfInsnOld);
            }
        return (OK);
        }

    /* New filter is invalid.  Free filter if necessary and return EINVAL */

    if (newFlag)
        free (pBpfInsnNew);

    return (EINVAL);
    }

/*******************************************************************************
*
* bpfIfSet - attaches BPF device to a network interface
*
* When <restartFlag> is TRUE, this routine reattaches a BPF device to a
* network interface (for the BIOCSTART ioctl). Otherwise, it handles
* the BIOCSETIF ioctl by detaching a BPF device from its current network
* interface, if any, and then attaching to a new network interface.
*
* RETURNS:
* OK, EINVAL, or ENOBUFS
*
* CAVEATS:
* Must be called by a task which holds the unit-specific _bpfUnitLock.
*
* INTERNAL
* This routine returns errno values if a failure occurs so that the ioctl
* mechanism can set the error number appropriately.
* 
* NOMANUAL
*/

LOCAL STATUS bpfIfSet
    (
    BPF_DEV_CTRL * pBpfDev,   /* BPF device control structure */
    struct ifreq * pIfReq,    /* Name of the network interface to attach to */
    BOOL 	restartFlag 	/* Create a new attachment or renew one? */
    )
    {
    char * pDevName;
    char devName [END_NAME_MAX + 1];
    int unit;
    int lockKey;     /* Blocks any other BPF operations. */
    STATUS result;

    BOOL attachFlag = FALSE; 	/* New attachment to a network interface? */

    END_OBJ * pEnd = NULL; 	/* Handle for END device. */
    struct ifnet * pIf = NULL; 	/* Handle for BSD device. */

    /* Validate the name of the network interface. */

    if (pIfReq == NULL)
        return (EINVAL);

    pDevName = pIfReq->ifr_name;

    if (*pDevName == EOS)
        return (EINVAL);    /* No device name given. */

    while (*pDevName != EOS && !isdigit ( (int) (*pDevName)))
        pDevName++;

    if (*pDevName != EOS)
        unit = atoi (pDevName);
    else
        return (EINVAL);    /* No unit number included in device name. */

    if (unit < 0)
        return (EINVAL);    /* Invalid unit number. */

    bzero (devName, END_NAME_MAX + 1);
    strncpy (devName, pIfReq->ifr_name,
             min (END_NAME_MAX, pDevName - pIfReq->ifr_name));

    pEnd = endFindByName (devName, unit);
    if (pEnd == NULL)
        {
        /* Check for BSD device. */

        pIf = ifunit (pIfReq->ifr_name);
        if (pIf == NULL)
            return (EINVAL);    /* No such device. */
        }

    /* Allocate buffers for BPF unit if it hasn't been done yet. */

    if (!restartFlag && pBpfDev->recvStoreBuf == NULL)
        {
        if (bpfBufsAlloc (pBpfDev) != OK)
            {
            return (ENOMEM);
            }
        }

    /*
     * Bind BPF unit to network interface if not already attached to it.
     * Prevent any other BPF units in the device from changing the list
     * of network interface attachments.
     */

    _bpfDevLock (pBpfDev->bpfDevId);    /* Preserve attachments list. */

    if (pBpfDev->pNetDev == NULL)
        attachFlag = TRUE;    /* New attachment for BPF device. */
    else if (pEnd)
        {
        /* Check if changing attachments to new END device. */

        if ( (pBpfDev->pNetDev->bsdFlag == TRUE) ||
            (PCOOKIE_TO_ENDOBJ(pBpfDev->pNetDev->pCookie) != pEnd))
            attachFlag = TRUE;
        }
    else
        {
        /* Check if changing attachments to new BSD device. */

        if ( (pBpfDev->pNetDev->bsdFlag == FALSE) ||
            (pBpfDev->pNetDev->pCookie != pIf))
            attachFlag = TRUE;
        }

    if (attachFlag)
        {
        /* 
         * Detach BPF unit if attached to another network interface.
         * Only possible for BIOCSETIF (when restartFlag is FALSE). 
         */

        if (pBpfDev->pNetDev != NULL)
            {
            _bpfDevDetach (pBpfDev);
            }

        /* Attach to the new network interface. */

        lockKey = _bpfLock ();
        result = _bpfProtoAttach (pBpfDev, devName, unit, pIf);
        _bpfUnlock (lockKey);

        if (result != OK)
            {
            _bpfDevUnlock (pBpfDev->bpfDevId);
            return (result);
            }

        /* Add the BPF unit to the network device's list. */

        lstAdd ( & (pBpfDev->pNetDev->bpfUnitList), (NODE *) pBpfDev);
        }

    _bpfDevUnlock (pBpfDev->bpfDevId);

    /*
     * For BIOCSETIF operation, reset buffers even if re-attaching to
     * the same network interface.
     */

    if (!restartFlag)
        bpfDevReset (pBpfDev);

    return (OK);
    }

/*******************************************************************************
*
* bpfPacketCatch - copy a packet to the BPF device store buffer
*
* This routine copies a packet to the BPF device store buffer. If required, it
* will also wakeup any pended read tasks.
*
* RETURNS:
* Not Applicable
*
* CAVEATS:
* Must be called by a task which holds the unit-specific _bpfUnitLock.
*
* NOMANUAL
*/

LOCAL void bpfPacketCatch
    (
    FAST BPF_DEV_CTRL * pBpfDev,    /* BPF device */
    FAST M_BLK_ID     pMBlk,        /* packet to copy to BPF device */
    FAST int          snapLen,      /* bytes of packet to copy */
    FAST int          bpfHdrLen,    /* len of bpf hdr + padding */
    struct timeval    * pTimeVal,   /* timestamp when packet was received */   
    FAST int          linkHdrLen    /* size of packet's link level header */
    )
    {
    FAST struct bpf_hdr * pBpfHdr;  /* BPF packet header */
    FAST int totLen;                /* total len of header + packet */
    FAST int curLen = 0;            /* current len of buffer */

    /* snapLen of -1 means copy entire packet */

    if (snapLen == -1) 
	snapLen = pMBlk->mBlkPktHdr.len;

    /*
     * Figure out how many bytes to move.  If the packet is
     * greater or equal to the snapshot length, transfer that
     * much.  Otherwise, transfer the whole packet (unless
     * we hit the buffer size limit).
     */

    totLen = bpfHdrLen + min (snapLen, pMBlk->mBlkPktHdr.len);
    if (totLen > pBpfDev->bufSize)
	{
	totLen = pBpfDev->bufSize;
	}

    /* Round up the end of the previous packet to the next longword. */
    
    curLen = BPF_WORDALIGN (pBpfDev->recvStoreLen);
    if (curLen + totLen > pBpfDev->bufSize)
	{

	/*
	 * This packet will overflow the storage buffer.
	 * Rotate the buffers if we can, then wakeup any
	 * pending reads.
	 */
	
	if (pBpfDev->freeBuf == NULL)
	    {

	    /*
	     * We haven't completed the previous read yet, 
	     * so drop the packet. 
	     */
	    
	    ++pBpfDev->pktDropCount;
	    return;
	    }

	ROTATE_BUFFERS (pBpfDev);
	bpfWakeup (pBpfDev);
	curLen = 0;
	}
    else if (pBpfDev->immediateMode)
	{

	/*
	 * Immediate mode is set.  A packet arrived so any
	 * pending reads should be triggered.
	 */

	bpfWakeup (pBpfDev);
	}
    
    /* Fill bpf header */

    pBpfHdr = (struct bpf_hdr *) (pBpfDev->recvStoreBuf + curLen);

    pBpfHdr->bh_tstamp = * pTimeVal;
    pBpfHdr->bh_caplen = totLen - bpfHdrLen;
    pBpfHdr->bh_datalen = pMBlk->mBlkPktHdr.len;
    pBpfHdr->bh_hdrlen = bpfHdrLen;
    pBpfHdr->bh_linklen = linkHdrLen;

    /* Copy the packet data into the store buffer and update its length. */
    
    netMblkOffsetToBufCopy (pMBlk, 0, (char *) pBpfHdr + bpfHdrLen, 
			    totLen - bpfHdrLen, NULL);
    pBpfDev->recvStoreLen = curLen + totLen;
    return;
    }

/*******************************************************************************
*
* bpfBufsAlloc - allocate space for BPF device buffers
*
* This routine allocates space for BPF device buffers.  Two buffers of size 
* <pBpfDev>->bufSize will be allocated from system memory. 
* 
* RETURNS:
* OK or ERROR
*
* ERRNO:
* None
*
* CAVEATS:
* Must be called by a task which holds the unit-specific _bpfUnitLock.
*
* INTERNAL
* This routine only executes within the ioctl mechanism, which sets
* the error number to ENOMEM if it fails.
*
* NOMANUAL
*/

LOCAL STATUS bpfBufsAlloc 
    (
    BPF_DEV_CTRL * pBpfDev   /* BPF device */
    )
    {
    pBpfDev->freeBuf = malloc (pBpfDev->bufSize);
    if (pBpfDev->freeBuf == NULL)
        return (ERROR);

    pBpfDev->recvStoreBuf = malloc (pBpfDev->bufSize);
    if (pBpfDev->recvStoreBuf == NULL)
        {
        free (pBpfDev->freeBuf);
        return (ERROR);
        }
    pBpfDev->recvStoreLen = 0;
    pBpfDev->readHoldLen = 0;
    return (OK);
    }

/*******************************************************************************
*
* bpfDevFree - free buffers in use by a BPF device
*
* This routine frees the buffers in use by a BPF device.  It then marks the
* BPF device as being no longer in use.
*
* RETURNS:
* Not Applicable
*
* CAVEATS:
* Must be called by a task which holds the unit-specific _bpfUnitLock.
*
* NOMANUAL
*/

LOCAL void bpfDevFree
    (
    FAST BPF_DEV_CTRL * pBpfDev
    )
    {
    if (pBpfDev->recvStoreBuf != NULL)
        {
        free (pBpfDev->recvStoreBuf);
        }

    if (pBpfDev->readHoldBuf != NULL)
        {
        free (pBpfDev->readHoldBuf);
        }

    if (pBpfDev->freeBuf != NULL)
        {
        free (pBpfDev->freeBuf);
        }

    if (pBpfDev->pBpfInsnFilter != NULL)
        {
        free (pBpfDev->pBpfInsnFilter);
        }

    semDelete (pBpfDev->readWkupSem);

    /* Protect status change against conflicts with bpfOpen routine. */

    _bpfDevLock (pBpfDev->bpfDevId);

    pBpfDev->inUse = FALSE;

    _bpfDevUnlock (pBpfDev->bpfDevId);

    return;
    }

/*******************************************************************************
*
* bpfWakeup - wakes up all tasks pended on a BPF device
*
* This routine wakes up all tasks waiting for a BPF device to receive
* data. Tasks pended within an active read are triggered by the internal
* semaphore. Any tasks waiting with the select operation are started
* with the appropriate method.
*
* RETURNS:
* Not Applicable
*
* CAVEATS:
* Must be called by a task which holds the unit-specific _bpfUnitLock.
*
* NOMANUAL
*/

LOCAL void bpfWakeup 
    (
    BPF_DEV_CTRL * pBpfDev   /* BPF device control structure */
    )
    {
    /* Wake all tasks pending in a read operation. */

    semFlush (pBpfDev->readWkupSem);

    /* Wake any tasks pending within a select operation. */

    if (pBpfDev->selectFlag)
        selWakeupAll (& (pBpfDev->selWkupList), SELREAD);

    return;
    }

/*******************************************************************************
*
* bpfSleep - block on a BPF device waiting for packets to arrive
*
* This routine will block on a BPF device to wait for packets to arrive.  When
* a packet arrives the pended task will be signalled.  If <numTicks> is 
* WAIT_FOREVER the task will block until packet arrival.  Otherwise the task
* will unblock when <numTicks> expires.
*
* RETURNS:
* OK
* EWOULDBLOCK - if <numTicks> expires before packets arrive
* EPIPE       - if the attached network interface is shut down or the
*               BPF unit is closed.
*
* CAVEATS:
* Must be called by a task which holds the unit-specific _bpfUnitLock.
*
* NOMANUAL
*/

LOCAL int bpfSleep
    (
    BPF_DEV_CTRL * pBpfDev,
    int numTicks
    )
    {
    int error = OK;

    if (semTake (pBpfDev->readWkupSem, numTicks) != OK)
        {
        if (errnoOfTaskGet (taskIdSelf ()) == S_objLib_OBJ_TIMEOUT)
            error = EWOULDBLOCK;
        else
            error = EPIPE;
        }

    return (error);
    }

/*******************************************************************************
*
* bpfTimeStamp - returns a timestamp value
*
* This routine returns a timestamp value in <pTimeval>.
*
* RETURNS:
* Not Applicable
*
* NOMANUAL
*/

LOCAL void bpfTimeStamp 
    (
    struct timeval * pTimeval   /* holds returned timestamp value */
    )
    {
    static int timeStamp = 0;

    pTimeval->tv_sec = ++timeStamp;
    pTimeval->tv_usec = 0;
    return;
    }

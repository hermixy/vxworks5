/* sntpsLib.c - Simple Network Time Protocol (SNTP) server library */

/* Copyright 1984 - 2000 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history 
--------------------
01l,07may02,kbw  man page edits
01k,25oct00,ham  doc: cleanup for vxWorks AE 1.0.
01j,16mar99,spm  removed references to configAll.h (SPR #25663)
01i,16mar99,spm  recovered orphaned code from tor2_0_x branch (SPR #25770)
01h,01dec98,spm  corrected man page references for clock hook (SPR #22860)
01g,14dec97,jdi  doc: cleanup.
01f,10dec97,kbw  made minor man page changes
01e,04dec97,spm  added minor changes to man pages; changed parameter names to 
                 comply with coding standards
01d,02sep97,spm  corrected return value and typecast for sntpsConfigSet routine
01c,27aug97,spm  corrected header file name in library description
01b,15jul97,spm  code cleanup, documentation, and integration; entered in
                 source code control
01a,24may97,kyc  written
*/

/* 
DESCRIPTION
This library implements the server side of the Simple Network Time Protocol
(SNTP), a protocol that allows a system to maintain the accuracy of its 
internal clock based on time values reported by one or more remote sources. 
The library is included in the VxWorks image if INCLUDE_SNTPS is defined 
at the time the image is built.

USER INTERFACE
The routine sntpsInit() is called automatically during system startup when 
the SNTP server library is included in the VxWorks image. Depending on the 
value of SNTPS_MODE, the server executes in either a passive or an active 
mode.  When SNTPS_MODE is set to SNTP_PASSIVE (0x2), the server waits for
requests from clients, and sends replies containing an NTP timestamp. When
the mode is set to SNTP_ACTIVE (0x1), the server transmits NTP timestamp
information at fixed intervals. 

When executing in active mode, the SNTP server uses the SNTPS_DSTADDR and 
SNTPS_INTERVAL definitions to determine the target IP address and broadcast
interval.  By default, the server will transmit the timestamp information to
the local subnet broadcast address every 64 seconds.  These settings can be
changed with a call to the sntpsConfigSet() routine.  The SNTP server operating
in active mode will still respond to client requests.

The SNTP_PORT definition in assigns the source and destination UDP port.  The 
default port setting is 123 as specified by the relevant RFC.  Finally, the
SNTP server requires access to a reliable external time source.  The
SNTPS_TIME_HOOK constant specifies the name of a routine with the following
interface:

.CS
    STATUS sntpsTimeHook (int request, void *pBuffer);
.CE

This routine can be assigned directly by altering the value of SNTPS_TIME_HOOK
or can be installed by a call to the sntpsClockSet() routine. The manual pages
for sntpsClockSet() describe the parameters and required operation of the
timestamp retrieval routine.  Until this routine is specified, the SNTP server
will not provide timestamp information.

VXWORKS AE PROTECTION DOMAINS
Under VxWorks AE, the SNPT server can run in the kernel protection domain only. 
The SNTPS_TIME_HOOK MUST, if used, must reference a function in the kernel 
protection domain.  This restriction does not apply under non-AE versions of 
VxWorks.  

INCLUDE FILES: sntpsLib.h

SEE ALSO: sntpcLib, RFC 1769
*/

/* includes */

#include "vxWorks.h"
#include "sysLib.h"
#include "inetLib.h"
#include "sockLib.h"
#include "netLib.h"
#include "ioLib.h"
#include "wdLib.h"
#include "usrLib.h"
#include "errnoLib.h"
#include "sntpsLib.h"

#include <sys/ioctl.h>

/* defines */

#define NSEC_BASE2 	30 	/* Integral log2 for nanosecond conversion. */

/* forward declarations */

LOCAL void sntpsMsgSend (void);
LOCAL void sntpsStart (void);

LOCAL int sntpsTaskPriority  = 56;    /* Priority level of SNTP server. */
LOCAL int sntpsTaskOptions = 0;       /* Option settings for SNTP server. */
LOCAL int sntpsTaskStackSize = 5000;  /* Stack size for SNTP server task. */
LOCAL WDOG_ID   sntpsTimer;           /* Timer for periodic broadcasts. */
LOCAL SEM_ID    sntpsMutexSem;        /* Protection for clock changes. */

LOCAL BOOL 	sntpsInitialized;     /* Ready to send timestamps? */
LOCAL BOOL 	sntpsClockReady;      /* Clock hook assigned? */

LOCAL ULONG 	sntpsClockId;         /* SNTP clock identifier. */
LOCAL ULONG     sntpsRefId;           /* Default reference identifier. */
LOCAL ULONG     sntpsResolution;      /* Clock resolution, in nanoseconds. */
LOCAL INT8      sntpsPrecision;       /* Clock precision, in RFC format. */
LOCAL u_char 	sntpsMode;            /* Active or passive operation. */
LOCAL short 	sntpsInterval;        /* Broadcast interval, in seconds. */
LOCAL u_short 	sntpsPort;            /* UDP source and destination port. */
LOCAL struct in_addr 	sntpsDstAddr; /* Broadcast or multicast destination. */
LOCAL FUNCPTR   sntpsClockHookRtn;    /* Access to timestamp information. */

/*******************************************************************************
*
* sntpsInit - set up the SNTP server
*
* This routine is called from usrNetwork.c to link the SNTP server module into
* the VxWorks image.  It creates all necessary internal data structures and
* initializes the SNTP server according to the assigned settings.
*
* RETURNS: OK or ERROR.
*
* ERRNO:
*  S_sntpsLib_INVALID_PARAMETER
*
* NOMANUAL
*/

STATUS sntpsInit 
    (
    char * 	pDevName, 	/* boot device name */
    u_char 	mode, 		/* broadcast or unicast mode */
    char * 	pDstAddr, 	/* destination IP address for broadcasts */
    short 	interval, 	/* broadcast interval */
    u_short 	port, 		/* UDP source/destination port */
    FUNCPTR 	pTimeHookRtn 	/* timestamp retrieval routine */
    )
    {
    STATUS result = OK;
    int retVal;
    int sockNum = 0;
    struct ifreq ifr;
    struct in_addr ifAddr;
    struct in_addr ifMask;

    sntpsInitialized = FALSE;
    sntpsClockReady = FALSE;

    sntpsRefId = 0;    /* Default ref. ID if hook routine returns ERROR */

    /* Set mode to specified value. */

    if (mode != SNTP_ACTIVE && mode != SNTP_PASSIVE)
        {
        errnoSet (S_sntpsLib_INVALID_PARAMETER);
        return (ERROR);
        }
    else
        sntpsMode = mode;
       
    /* For active (broadcast) mode, set message interval and target address. */

    if (sntpsMode == SNTP_ACTIVE)
        {
        /* Sanity check broadcast interval and assign target address. */

        if (interval < 0)
            interval = 0;
        sntpsInterval = interval;

        /* Use subnet local broadcast address if none specified. */

        if (pDstAddr == NULL)
            {
            /* Combine IP address of interface and current netmask. */

            sockNum = socket (AF_INET, SOCK_RAW, 0);
            if (sockNum == -1)
                return (ERROR);

            bzero ( (char *)&ifr, sizeof (struct ifreq));
            strcpy (ifr.ifr_name, pDevName);
            ifr.ifr_addr.sa_len = sizeof (struct sockaddr_in);

            retVal = ioctl (sockNum, SIOCGIFADDR, (int)&ifr);
            if (retVal != 0)
                {
                close (sockNum);
                return (ERROR);
                }
            ifAddr.s_addr = 
                       ( (struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;

            retVal = ioctl (sockNum, SIOCGIFNETMASK, (int)&ifr);
            if (retVal != 0)
                {
                close (sockNum);
                return (ERROR);
                }
            ifMask.s_addr = 
                       ( (struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;

            close (sockNum);

            /* Extract network address and fill host portion with ones. */

            sntpsDstAddr.s_addr = (ifAddr.s_addr & ifMask.s_addr) 
                                    | ~ifMask.s_addr;
            }
        else        /* Use specified destination address. */
            {
            sntpsDstAddr.s_addr = inet_addr (pDstAddr);
            if (sntpsDstAddr.s_addr == ERROR)
                {
                errnoSet (S_sntpsLib_INVALID_PARAMETER);
                return (ERROR);
                }
            }

        /* Create timer for periodic transmissions. */

        sntpsTimer = wdCreate ();
        if (sntpsTimer == NULL)
            return (ERROR);
        }

    /* Create synchronization semaphore for changing clock. */

    sntpsMutexSem = semBCreate (SEM_Q_FIFO, SEM_FULL);
    if (sntpsMutexSem == NULL)
        {
        if (sntpsMode == SNTP_ACTIVE)
            wdDelete (sntpsTimer);
        return (ERROR);
        }

    sntpsPort = htons (port);

    sntpsInitialized = TRUE;

    /* Enable transmission if timestamp retrieval routine is provided. */

    result = sntpsClockSet (pTimeHookRtn);
    if (result == OK)
        sntpsClockReady = TRUE;
 
    result = taskSpawn ("tSntpsTask", sntpsTaskPriority, sntpsTaskOptions,
                        sntpsTaskStackSize, (FUNCPTR) sntpsStart,
                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    return (result);
    }

/*******************************************************************************
*
* sntpsLog2Get - find approximate power of two
*
* This routine determines the nearest integral power of two for the given 
* value, without using floating point arithmetic.  It is used to convert 
* 32-bit nanosecond values into signed integers for assignment to the poll 
* and precision fields of NTP messages.
*
* RETURNS: Nearest integral log base two value, in host byte order.
*
* ERRNO: N/A
*
* INTERNAL
* Floating-point calculations can't be used because some boards (notably
* the SPARC architectures) disable software floating point by default to
* speed up context switching. These boards abort with an exception when
* floating point operations are encountered.
*
* NOMANUAL
*/

INT8 sntpsLog2Get
    (
    ULONG inval 	/* input value for calculation */
    )
    {
    int loop;
    int floor; 		/* Nearest power of two for smaller value */
    int limit; 		/* Nearest power of two for larger value */
    int result;
    ULONG mask; 	/* Bitmask for log2 calculation */

    if (inval == 0)
        result = 0;
    else
        {
        /*
         * Set increasing numbers of the low-order bits of the input value
         * to zero until all bits have been cleared. The current and previous
         * values of the loop counter indicate the adjacent integral powers
         * of two.
         */

        for (loop = 0; loop < 32; loop ++)
            {
            mask = ~0 << loop;     /* Mask out the rightmost "loop" bits. */
            if ( (inval & mask) == 0)
                break;
            }
        floor = 1 << (loop - 1);
        limit = 1 << loop;
        if (inval - floor < limit - inval)
            result = loop - 1;
        else
            result = loop;
        }
   
    return (result);
    }

/*******************************************************************************
*
* sntpsClockSet - assign a routine to access the reference clock
*
* This routine installs a hook routine that is called to access the 
* reference clock used by the SNTP server. This hook routine must use the 
* following interface:
* .CS
*     STATUS sntpsClockHook (int request, void *pBuffer);
* .CE 
* The hook routine should copy one of three settings used by the server to
* construct outgoing NTP messages into <pBuffer> according to the value of 
* the <request> parameter.  If the requested setting is available, the 
* installed routine should return OK (or ERROR otherwise).
*
* This routine calls the given hook routine with the <request> parameter 
* set to SNTPS_ID to get the 32-bit reference identifier in the format
* specified in RFC 1769.  It also calls the hook routine with <request> 
* set to SNTPS_RESOLUTION to retrieve a 32-bit value containing the clock 
* resolution in nanoseconds.  That value will be used to determine the 8-bit 
* signed integer indicating the clock precision (according to the format 
* specified in RFC 1769).  Other library routines will set the <request> 
* parameter to SNTPS_TIME to retrieve the current 64-bit NTP timestamp 
* from <pBuffer> in host byte order.  The routine sntpsNsecToFraction() will 
* convert a value in nanoseconds to the format required for the NTP 
* fractional part.
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  In addition, all arguments to this function can  
* reference only that data which is valid in the kernel protection domain. 
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS: OK or ERROR.
*
* ERRNO: N/A
*/

STATUS sntpsClockSet
    (
    FUNCPTR 	pClockHookRtn 	/* new interface to reference clock */
    )
    {
    STATUS result;
    INT8 basetwo;

    if (pClockHookRtn == NULL)
        return (ERROR);

    /* Don't change clock setting if current routine is in use. */

    semTake (sntpsMutexSem, WAIT_FOREVER);

    sntpsClockHookRtn = pClockHookRtn;

    /* Get clock precision and clock identifier. */

    result = (* sntpsClockHookRtn) (SNTPS_ID, &sntpsClockId);
    if (result == ERROR)  /* Clock ID not available. Use default value. */
        sntpsClockId = sntpsRefId;

    result = (* sntpsClockHookRtn) (SNTPS_RESOLUTION, &sntpsResolution);
    if (result == ERROR)
        sntpsPrecision = 0;
    else
        {
        /* Find nearest power of two to clock resolution. */

        basetwo = sntpsLog2Get (sntpsResolution);

        /*
         * Convert to seconds required for NTP message. Subtract nearest 
         * integer to log base two of 1E9 (corresponds to division of clock 
         * resolution by 1E9).
         */

        sntpsPrecision = basetwo - NSEC_BASE2;
        }

    if (!sntpsClockReady)
        sntpsClockReady = TRUE;     /* Enable transmission of messages. */

    semGive (sntpsMutexSem);

    return (OK);
    }

/*******************************************************************************
*
* sntpsNsecToFraction - convert portions of a second to NTP format
*
* This routine is provided for convenience in fulfilling an SNTPS_TIME request
* to the clock hook.  It converts a value in nanoseconds to the fractional part 
* of the NTP timestamp format.  The routine is not designed to convert 
* non-normalized values greater than or equal to one second.  Although the NTP 
* time format provides a precision of about 200 pico-seconds, rounding errors 
* in the conversion process decrease the accuracy as the input value increases.
* In the worst case, only the 24 most significant bits are valid, which reduces
* the precision to tenths of a micro-second.
*
* RETURNS: Value for NTP fractional part in host-byte order.
*
* ERRNO: N/A
*
* INTERNAL
* Floating-point calculations can't be used because some boards (notably
* the SPARC architectures) disable software floating point by default to
* speed up context switching. These boards abort with an exception when
* floating point operations are encountered.
*/

ULONG sntpsNsecToFraction
    (
    ULONG nsecs 	/* nanoseconds to convert to binary fraction */
    )
    {
    ULONG factor = 294967296;  /* Partial conversion factor from base 10 */
    ULONG divisor = 10;        /* Initial exponent for mantissa. */
    ULONG mask = 100000000;    /* Pulls digits of factor from left to right. */
    int loop;
    ULONG fraction = 0;
    BOOL shift = FALSE;        /* Shifted to avoid overflow? */


    /*
     * Adjust large values so that no intermediate calculation exceeds
     * 32 bits. (This test is overkill, since the fourth MSB can be set
     * sometimes, but it's fast).
     */

    if (nsecs & 0xF0000000)
        {
        nsecs >>= 4;     /* Exclude rightmost hex digit. */
        shift = TRUE;
        }

    /*
     * In order to increase portability, the following conversion avoids
     * floating point operations, so it is somewhat obscure.
     *
     * A one nanosecond increase corresponds to increasing the NTP fractional 
     * part by (2^32)/1E9. Multiplying the number of nanoseconds by that value
     * (4.294967286) produces the NTP fractional part.
     *
     * The above constant is separated into integer and decimal parts to avoid
     * overflow. The mask variable selects each digit from the decimal part
     * sequentially, and the divisor shifts the digit the appropriate number
     * of decimal places.
     */

    fraction += nsecs * 4;              /* Handle integer part of conversion */

    for (loop = 0; loop < 9; loop++)    /* Nine digits in mantissa */
        {
        fraction += nsecs * (factor/mask)/divisor;
        factor %= mask;    /* Remove most significant digit from the factor. */
        mask /= 10;        /* Reduce length of mask by one. */
        divisor *= 10;     /* Increase shift by one decimal place. */
        }

    /* Scale result upwards if value was adjusted before processing. */

    if (shift)
        fraction <<= 4;

    return (fraction);
    }

/*******************************************************************************
*
* sntpsConfigSet - change SNTP server broadcast settings
*
* This routine alters the configuration of the SNTP server when operating
* in broadcast mode.  A <setting> value of SNTPS_DELAY interprets the contents
* of <pValue> as the new 16-bit broadcast interval.  When <setting> equals
* SNTPS_ADDRESS, <pValue> should provide the string representation of an
* IP broadcast or multicast address (for example, "224.0.1.1").  Any changed 
* settings will take effect after the current broadcast interval is 
* completed and the corresponding NTP message is sent.
*
* RETURNS: OK or ERROR.
*
* ERRNO:
*  S_sntpsLib_INVALID_PARAMETER
*/

STATUS sntpsConfigSet
    (
    int 	setting,     /* configuration option to change */
    void * 	pValue       /* new value for parameter */
    )
    {
    struct in_addr target;
    short interval;
    int result = OK;

    /* Don't change settings if message transmission in progress. */

    semTake (sntpsMutexSem, WAIT_FOREVER);

    if (setting == SNTPS_ADDRESS)
        {
        target.s_addr = inet_addr ( (char *)pValue);
        if (target.s_addr == ERROR)
            {
            errnoSet (S_sntpsLib_INVALID_PARAMETER);
            result = ERROR; 
            }
        else
            sntpsDstAddr.s_addr = target.s_addr;
        }
    else if (setting == SNTPS_DELAY)
        {
        interval = (short) (*( (int *)pValue));
        sntpsInterval = interval;
        }
    else
        {
        errnoSet (S_sntpsLib_INVALID_PARAMETER);
        result = ERROR;
        }

    semGive (sntpsMutexSem);

    return (result);
    }

/*******************************************************************************
*
* sntpsStart - execute the SNTP server
*
* This routine monitors the specified SNTP/NTP port for incoming requests from
* clients and transmits replies containing the NTP timestamp obtained from
* the hook provided by the user. If the server executes in broadcast mode,
* this routine also schedules the transmission of NTP messages at the assigned
* broadcast interval. It is the entry point for the SNTP server task and should
* only be called internally.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

LOCAL void sntpsStart (void)
    {
    SNTP_PACKET sntpRequest;     /* SNTP request received from client */
    SNTP_PACKET sntpReply;       /* buffer for server reply */
 
    struct sockaddr_in srcAddr;  /* address of requesting SNTP/NTP client */
    struct sockaddr_in dstAddr;  /* target address of transmission */
    int sntpSocket;
    int result;
    int addrLen;
    SNTP_TIMESTAMP refTime;
    BOOL unsync;

    if (!sntpsInitialized)    /* Sanity check to force initialization. */
        return;

    addrLen = sizeof (dstAddr);

    /* Set address information. */

    bzero ( (char *)&srcAddr, sizeof(srcAddr));
    bzero ( (char *)&dstAddr, sizeof(dstAddr));
    srcAddr.sin_addr.s_addr = INADDR_ANY;
    srcAddr.sin_family = AF_INET;
    srcAddr.sin_port = sntpsPort;
  
    /* Create UDP socket and bind to the SNTP port. */
  
    sntpSocket = socket (AF_INET, SOCK_DGRAM, 0);
    if (sntpSocket == -1) 
        return;

    result = bind (sntpSocket, (struct sockaddr *)&srcAddr, sizeof (srcAddr));
    if (result == -1)
        {
        close (sntpSocket);
        return;
        }
 
    /* 
     * The use of sntpsInterval below doesn't need to be guarded from a call to
     * sntpsConfigSet() because this routine is called during system startup.
     */

    if (sntpsMode == SNTP_ACTIVE)
        wdStart (sntpsTimer, sntpsInterval * sysClkRateGet (),
                 (FUNCPTR)netJobAdd, (int)sntpsMsgSend);
    FOREVER 
        {
        result = recvfrom (sntpSocket, (caddr_t)&sntpRequest, 
                           sizeof (sntpRequest), 0, 
                           (struct sockaddr *)&dstAddr, &addrLen);
        if (result == -1)
            continue;

        semTake (sntpsMutexSem, WAIT_FOREVER);   /* Lock out clock changes. */

        /* Can't transmit messages if no access to clock is provided. */

        if (!sntpsClockReady || sntpsClockHookRtn == NULL)
            {
            semGive (sntpsMutexSem);
            continue;
            }

        /* All timestamp fields are zero by default. */

        bzero ( (char *)&sntpReply, sizeof (sntpReply));

        /* Retrieve the current clock ID, precision, and NTP timestamp. */

        sntpReply.precision = sntpsPrecision;
        sntpReply.referenceIdentifier = sntpsClockId;

        unsync = FALSE;
        result = (* sntpsClockHookRtn) (SNTPS_TIME, &refTime);
        if (result == ERROR)
            unsync = TRUE;

        semGive (sntpsMutexSem);

        /* Set the leap indicator and version number. */

        sntpReply.leapVerMode = 0;

        if (unsync)
            {
            sntpReply.stratum = SNTP_STRATUM_0;
            sntpReply.leapVerMode |= SNTP_LI_3;
            }
        else
            {
            sntpReply.stratum = SNTP_STRATUM_1;
            sntpReply.leapVerMode |= SNTP_LI_0;
            }

        sntpReply.leapVerMode |= (sntpRequest.leapVerMode & SNTP_VN_MASK);

        /* Set mode to server for client response, or to symmetric passive. */

        if ( (sntpRequest.leapVerMode & SNTP_MODE_MASK) == SNTP_MODE_3)
            sntpReply.leapVerMode |= SNTP_MODE_4;
        else
            sntpReply.leapVerMode |= SNTP_MODE_2;

        /* Copy the poll field from the request. */

        sntpReply.poll = sntpRequest.poll;

        /* 
         * Leave the root delay and root dispersion fields at zero.
         * Set the timestamp fields and send the message.
         */

        if (!unsync)
            {
            sntpReply.referenceTimestampSec = htonl (refTime.seconds);
            sntpReply.referenceTimestampFrac = htonl (refTime.fraction);
      
            sntpReply.receiveTimestampSec = sntpReply.referenceTimestampSec;
            sntpReply.receiveTimestampFrac = sntpReply.referenceTimestampFrac;

            sntpReply.transmitTimestampSec = sntpReply.referenceTimestampSec;
            sntpReply.transmitTimestampFrac = sntpReply.referenceTimestampFrac;

            /* The originate timestamp contains the request transmit time. */

            sntpReply.originateTimestampSec = sntpRequest.transmitTimestampSec;
            sntpReply.originateTimestampFrac =
                                             sntpRequest.transmitTimestampFrac;
            }

        result = sendto (sntpSocket, (caddr_t)&sntpReply, sizeof (sntpReply), 
                          0, (struct sockaddr *)&dstAddr, sizeof (dstAddr));
        }

    /* Not reached. */

    close (sntpSocket);
    return;
    }

/*******************************************************************************
*
* sntpsMsgSend - transmit an unsolicited NTP message
*
* This routine sends an NTP message to the assigned destination address when 
* the broadcast interval has elapsed.  It is called by watchdog timers set
* during initialization, and should not be used directly.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

void sntpsMsgSend (void)
    {
    SNTP_PACKET sntpReply;
    int result;
    int optval;
    short interval;
    SNTP_TIMESTAMP refTime;
    int sntpSocket;
    struct sockaddr_in dstAddr;

    if (!sntpsInitialized)    /* Sanity check to prevent direct calls. */
        return;

    /* Lock out clock and configuration changes. */

    semTake (sntpsMutexSem, WAIT_FOREVER);

    /* Can't transmit messages if no access to clock is provided. */

    if (!sntpsClockReady || sntpsClockHookRtn == NULL)
        {
        semGive (sntpsMutexSem);
        wdStart (sntpsTimer, sntpsInterval * sysClkRateGet (),
                 (FUNCPTR)netJobAdd, (int)sntpsMsgSend);
        return;
        }

    interval = sntpsInterval;    /* Save current broadcast interval. */

    /* Retrieve the current clock ID, precision, and NTP timestamp. */

    sntpReply.precision = sntpsPrecision;
    sntpReply.referenceIdentifier = sntpsClockId;

    result = (* sntpsClockHookRtn) (SNTPS_TIME, &refTime);
    if (result == ERROR)
        {
        semGive (sntpsMutexSem);
        wdStart (sntpsTimer, sntpsInterval * sysClkRateGet (),
                 (FUNCPTR)netJobAdd, (int)sntpsMsgSend);
        return;
        }

    /* Assign target address for outgoing message. */

    bzero ( (char *)&dstAddr, sizeof(dstAddr));
    dstAddr.sin_addr.s_addr = sntpsDstAddr.s_addr;
    dstAddr.sin_family = AF_INET;
    dstAddr.sin_port = sntpsPort;

    semGive (sntpsMutexSem);
  
    /* Create UDP socket for transmission. */
  
    sntpSocket = socket (AF_INET, SOCK_DGRAM, 0);
    if (sntpSocket == -1) 
        {
        wdStart (sntpsTimer, interval * sysClkRateGet (),
                 (FUNCPTR)netJobAdd, (int)sntpsMsgSend);
        return;
        }

    /* Enable broadcast option for socket. */

    optval = 1;
    result = setsockopt (sntpSocket, SOL_SOCKET, SO_BROADCAST, (char *)&optval,
                         sizeof (optval));
    if (result == ERROR)
        {
        close (sntpSocket);
        wdStart (sntpsTimer, interval * sysClkRateGet (),
                 (FUNCPTR)netJobAdd, (int)sntpsMsgSend);
        return;
        }

    /* 
     * Set the common values for outgoing NTP messages - root delay
     * and root dispersion are 0. 
     */
  
    bzero ((char *)&sntpReply, sizeof (sntpReply));
    sntpReply.stratum = SNTP_STRATUM_1;
      
    /* Set the leap indicator, version number and mode. */

    sntpReply.leapVerMode |= SNTP_LI_0;
    sntpReply.leapVerMode |= SNTP_VN_3;
    sntpReply.leapVerMode |= SNTP_MODE_5;

    /* Set the poll field: find the nearest integral power of two. */

    sntpReply.poll = sntpsLog2Get (interval);

    /* Set the timestamp fields and send the message. */

    sntpReply.referenceTimestampSec = htonl (refTime.seconds);
    sntpReply.referenceTimestampFrac = htonl (refTime.fraction);
      
    sntpReply.receiveTimestampSec = sntpReply.referenceTimestampSec;
    sntpReply.receiveTimestampFrac = sntpReply.referenceTimestampFrac;

    sntpReply.transmitTimestampSec = sntpReply.referenceTimestampSec;
    sntpReply.transmitTimestampFrac = sntpReply.referenceTimestampFrac;

    sntpReply.originateTimestampSec = sntpReply.referenceTimestampSec;
    sntpReply.originateTimestampFrac = sntpReply.referenceTimestampFrac;

    result = sendto (sntpSocket, (caddr_t)&sntpReply, sizeof (sntpReply), 0, 
                     (struct sockaddr *)&dstAddr, sizeof (dstAddr));

    close (sntpSocket);

    /* Schedule a new transmission after the broadcast interval. */

    wdStart (sntpsTimer, interval * sysClkRateGet (),
             (FUNCPTR)netJobAdd, (int)sntpsMsgSend);
    return;
    }

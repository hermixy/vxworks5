/* sntpcLib.c - Simple Network Time Protocol (SNTP) client library */

/* Copyright 1984-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history 
--------------------
01k,07jan02,vvv  doc: added errnos for sntpcTimeGet and sntpcFetch (SPR #71557)
01j,16mar99,spm  doc: removed references to configAll.h (SPR #25663)
01e,14dec97,jdi  doc: cleanup.
01d,10dec97,kbw  making man page changes
01c,27aug97,spm  corrections for man page generation
01b,15jul97,spm  code cleanup, documentation, and integration; entered in
                 source code control
01a,24may97,kyc  written
*/

/* 
DESCRIPTION
This library implements the client side of the Simple Network Time 
Protocol (SNTP), a protocol that allows a system to maintain the 
accuracy of its internal clock based on time values reported by one 
or more remote sources.  The library is included in the VxWorks image 
if INCLUDE_SNTPC is defined at the time the image is built.

USER INTERFACE
The sntpcTimeGet() routine retrieves the time reported by a remote source and
converts that value for POSIX-compliant clocks.  The routine will either send a 
request and extract the time from the reply, or it will wait until a message is
received from an SNTP/NTP server executing in broadcast mode.

INCLUDE FILES: sntpcLib.h

SEE ALSO: clockLib, RFC 1769
*/

/* includes */

#include "vxWorks.h"
#include "sysLib.h"
#include "ioLib.h"
#include "inetLib.h"
#include "hostLib.h"
#include "sockLib.h"
#include "errnoLib.h"

#include "sntpcLib.h"

/* globals */

u_short sntpcPort;

/* forward declarations */

LOCAL STATUS sntpcListen (u_int, struct timespec *);
LOCAL STATUS sntpcFetch (struct in_addr *, u_int, struct timespec *);

/*******************************************************************************
*
* sntpcInit - set up the SNTP client
*
* This routine is called to link the SNTP client module into the VxWorks
* image. It assigns the UDP source and destination port according to the
* corresponding SNTP_PORT setting.
* 
* RETURNS: OK, always.
*
* ERRNO: N/A
*
* NOMANUAL
*/

STATUS sntpcInit
    (
    u_short 	port 	/* UDP source/destination port */
    )
    {
    sntpcPort = htons (port);

    return (OK);
    }

/*******************************************************************************
*
* sntpcFractionToNsec - convert time from the NTP format to POSIX time format
*
* This routine converts the fractional part of the NTP timestamp format to a 
* value in nanoseconds compliant with the POSIX clock.  While the NTP time 
* format provides a precision of about 200 pico-seconds, rounding error in the 
* conversion routine reduces the precision to tenths of a micro-second.
* 
* RETURNS: Value for struct timespec corresponding to NTP fractional part
*
* ERRNO:   N/A
*
* INTERNAL
*
* Floating-point calculations can't be used because some boards (notably
* the SPARC architectures) disable software floating point by default to 
* speed up context switching. These boards abort with an exception when
* floating point operations are encountered.
*
* NOMANUAL
*/

LOCAL ULONG sntpcFractionToNsec
    (
    ULONG sntpFraction      /* base 2 fractional part of the NTP timestamp */
    )
    {
    ULONG factor = 0x8AC72305; /* Conversion factor from base 2 to base 10 */
    ULONG divisor = 10;        /* Initial exponent for mantissa. */
    ULONG mask = 1000000000;   /* Pulls digits of factor from left to right. */
    int loop;
    ULONG nsec = 0;
    BOOL shift = FALSE;        /* Shifted to avoid overflow? */


    /* 
     * Adjust large values so that no intermediate calculation exceeds 
     * 32 bits. (This test is overkill, since the fourth MSB can be set 
     * sometimes, but it's fast).
     */
 
    if (sntpFraction & 0xF0000000)
        {
        sntpFraction /= 10;
        shift = TRUE;
        }

    /* 
     * In order to increase portability, the following conversion avoids
     * floating point operations, so it is somewhat obscure.
     *
     * Incrementing the NTP fractional part increases the corresponding
     * decimal value by 2^(-32). By interpreting the fractional part as an
     * integer representing the number of increments, the equivalent decimal
     * value is equal to the product of the fractional part and 0.2328306437.
     * That value is the mantissa for 2^(-32). Multiplying by 2.328306437E-10
     * would convert the NTP fractional part into the equivalent in seconds.
     *
     * The mask variable selects each digit from the factor sequentially, and
     * the divisor shifts the digit the appropriate number of decimal places. 
     * The initial value of the divisor is 10 instead of 1E10 so that the 
     * conversion produces results in nanoseconds, as required by POSIX clocks.
     */

    for (loop = 0; loop < 10; loop++)    /* Ten digits in mantissa */
        {
	nsec += sntpFraction * (factor/mask)/divisor;  /* Use current digit. */
	factor %= mask;    /* Remove most significant digit from the factor. */
	mask /= 10;        /* Reduce length of mask by one. */
	divisor *= 10;     /* Increase preceding zeroes by one. */
        }

    /* Scale result upwards if value was adjusted before processing. */

    if (shift)
        nsec *= 10;

    return (nsec);
    }

/*******************************************************************************
*
* sntpcTimeGet - retrieve the current time from a remote source
*
* This routine stores the current time as reported by an SNTP/NTP server in
* the location indicated by <pCurrTime>.  The reported time is first converted
* to the elapsed time since January 1, 1970, 00:00, GMT, which is the base value
* used by UNIX systems.  If <pServerAddr> is NULL, the routine listens for 
* messages sent by an SNTP/NTP server in broadcast mode.  Otherwise, this
* routine sends a request to the specified SNTP/NTP server and extracts the
* reported time from the reply.  In either case, an error is returned if no 
* message is received within the interval specified by <timeout>.  Typically, 
* SNTP/NTP servers operating in broadcast mode send update messages every 64 
* to 1024 seconds.  An infinite timeout value is specified by WAIT_FOREVER.
* 
* RETURNS: OK, or ERROR if unsuccessful.
*
* ERRNO:
*  S_sntpcLib_INVALID_PARAMETER, S_sntpcLib_INVALID_ADDRESS, S_sntpcLib_TIMEOUT,
*  S_sntpcLib_SERVER_UNSYNC, S_sntpcLib_VERSION_UNSUPPORTED
*/

STATUS sntpcTimeGet
    (
    char * 		pServerAddr, 	/* server IP address or hostname */
    u_int 		timeout,	/* timeout interval in ticks */
    struct timespec * 	pCurrTime	/* storage for retrieved time value */
    )
    {
    STATUS result;
    struct in_addr 	target;

    if (pCurrTime == NULL || (timeout < 0 && timeout != WAIT_FOREVER))
        {
        errnoSet (S_sntpcLib_INVALID_PARAMETER);
        return (ERROR);
        }

    if (pServerAddr == NULL)
        result = sntpcListen (timeout, pCurrTime);
    else
        {
        target.s_addr = hostGetByName (pServerAddr);
        if (target.s_addr == ERROR)
            target.s_addr = inet_addr (pServerAddr);
    
        if (target.s_addr == ERROR)
            {
            errnoSet (S_sntpcLib_INVALID_ADDRESS);
            return (ERROR);
            }
        result = sntpcFetch (&target, timeout, pCurrTime); 
        }
    return (result);
    }

/*******************************************************************************
*
* sntpcFetch - send an SNTP request and retrieve the time from the reply
*
* This routine sends an SNTP request to the IP address specified by
* <pTargetAddr>, converts the returned NTP timestamp to the POSIX-compliant 
* clock format with the UNIX base value (elapsed time since 00:00 GMT on 
* Jan. 1, 1970), and stores the result in the location indicated by <pTime>.
* 
* RETURNS: OK, or ERROR if unsuccessful.
*
* ERRNO:
*  S_sntpcLib_SERVER_UNSYNC
*  S_sntpcLib_VERSION_UNSUPPORTED
*  S_sntpcLib_TIMEOUT
*
* NOMANUAL
*/

LOCAL STATUS sntpcFetch
    (
    struct in_addr * 	pTargetAddr, 	/* SNTP/NTP server IP address */
    u_int 		timeout,	/* timeout in ticks */
    struct timespec * 	pCurrTime	/* storage for retrieved time value */
    )
    {
    SNTP_PACKET sntpRequest;     /* sntp request packet for */
                                 /* transmission to server */
    SNTP_PACKET sntpReply;       /* buffer for server reply */
    struct sockaddr_in dstAddr;
    struct sockaddr_in servAddr;
    struct timeval sockTimeout;
    int optval;
    int clockRate;
    int sntpSocket;
    fd_set readFds;
    int result;
    int servAddrLen;
  
    /* Set destination for request. */
  
    bzero ( (char *)&dstAddr, sizeof (dstAddr));
    dstAddr.sin_addr.s_addr = pTargetAddr->s_addr;
    dstAddr.sin_family = AF_INET;
    dstAddr.sin_port = sntpcPort;
  
    /* Create socket for transmission. */
  
    sntpSocket = socket (AF_INET, SOCK_DGRAM, 0);
    if (sntpSocket == -1) 
        return (ERROR);

    /* 
     * Enable broadcast option for socket in case that address is given. 
     * This use of the SNTP client is not likely, so ignore errors. If 
     * the broadcast address is used, and this call fails, the error will
     * be caught by sendto() below.
     */

    optval = 1;
    result = setsockopt (sntpSocket, SOL_SOCKET, SO_BROADCAST, 
                         (char *)&optval, sizeof (optval));

    /* Initialize SNTP message buffers. */
  
    bzero ( (char *)&sntpRequest, sizeof (sntpRequest));
    bzero ( (char *)&sntpReply, sizeof (sntpReply));
  
    sntpRequest.leapVerMode = SNTP_CLIENT_REQUEST;
  
    bzero ( (char *) &servAddr, sizeof (servAddr));
    servAddrLen = sizeof (servAddr);
  
    /* Transmit SNTP request. */
  
    if (sendto (sntpSocket, (caddr_t)&sntpRequest, sizeof(sntpRequest), 0,
                (struct sockaddr *)&dstAddr, sizeof (dstAddr)) == -1) 
        {
        close (sntpSocket);
        return (ERROR);
        }
    
    /* Convert timeout value to format needed by select() call. */

    if (timeout != WAIT_FOREVER)
        {
        clockRate = sysClkRateGet ();
        sockTimeout.tv_sec = timeout / clockRate;
        sockTimeout.tv_usec = (1000000 * timeout % clockRate) / clockRate;
        }

    /* Wait for reply at the ephemeral port selected by the sendto () call. */

    FD_ZERO (&readFds);
    FD_SET (sntpSocket, &readFds);

    if (timeout == WAIT_FOREVER)
        result = select (FD_SETSIZE, &readFds, NULL, NULL, NULL);
    else
        result = select (FD_SETSIZE, &readFds, NULL, NULL, &sockTimeout);

    if (result == -1) 
        {
        close (sntpSocket);
        return (ERROR);
        }

    if (result == 0)    /* Timeout interval expired. */
        {
        close (sntpSocket); 
        errnoSet (S_sntpcLib_TIMEOUT);
        return (ERROR);
        }

    result = recvfrom (sntpSocket, (caddr_t)&sntpReply, sizeof (sntpReply),
                       0, (struct sockaddr *)&servAddr, &servAddrLen);

    if (result == -1) 
        {
        close (sntpSocket);
        return (ERROR);	  
        }

    close (sntpSocket);

    /*
     * Return error if the server clock is unsynchronized, or the version is 
     * not supported.
     */

    if ( (sntpReply.leapVerMode & SNTP_LI_MASK) == SNTP_LI_3 ||
        sntpReply.transmitTimestampSec == 0)
        {
        errnoSet (S_sntpcLib_SERVER_UNSYNC);
        return (ERROR);
        }

    if ( (sntpReply.leapVerMode & SNTP_VN_MASK) == SNTP_VN_0 ||
        (sntpReply.leapVerMode & SNTP_VN_MASK) > SNTP_VN_3)
        {
        errnoSet (S_sntpcLib_VERSION_UNSUPPORTED);
	return (ERROR);
        }

    /* Convert the NTP timestamp to the correct format and store in clock. */

    /* Add test for 2036 base value here! */

    sntpReply.transmitTimestampSec = ntohl (sntpReply.transmitTimestampSec) - 
                                     SNTP_UNIX_OFFSET;

    /* 
     * Adjust returned value if leap seconds are present. 
     * This needs work! 
     */ 

    /* if ( (sntpReply.leapVerMode & SNTP_LI_MASK) == SNTP_LI_1)
            sntpReply.transmitTimestampSec += 1;
     else if ((sntpReply.leapVerMode & SNTP_LI_MASK) == SNTP_LI_2)
              sntpReply.transmitTimestampSec -= 1;
    */

    sntpReply.transmitTimestampFrac = ntohl (sntpReply.transmitTimestampFrac);

    pCurrTime->tv_sec = sntpReply.transmitTimestampSec;
    pCurrTime->tv_nsec = sntpcFractionToNsec (sntpReply.transmitTimestampFrac);

    return (OK);
    }

/*******************************************************************************
*
* sntpcListen - retrieve the time from an SNTP/NTP broadcast
*
* This routine listens to the SNTP/NTP port for a valid message from any 
* SNTP/NTP server executing in broadcast mode, converts the returned NTP 
* timestamp to the POSIX-compliant clock format with the UNIX base value
* (elapsed time since 00:00 GMT on January 1, 1970), and stores the result in 
* the location indicated by <pTime>.
* 
* RETURNS: OK, or ERROR if unsuccessful.
*
* ERRNO:
*  S_sntpcLib_TIMEOUT
*
* NOMANUAL
*/

LOCAL STATUS sntpcListen
    (
    u_int 		timeout,	/* timeout in ticks */
    struct timespec * 	pCurrTime	/* storage for retrieved time value */
    )
    {
    SNTP_PACKET sntpMessage;    /* buffer for message from server */
    struct sockaddr_in srcAddr;
    int sntpSocket;
    struct timeval sockTimeout;
    int clockRate;
    fd_set readFds;
    int result;
    int srcAddrLen;
 
    /* Initialize source address. */

    bzero ( (char *)&srcAddr, sizeof (srcAddr));
    srcAddr.sin_addr.s_addr = INADDR_ANY;
    srcAddr.sin_family = AF_INET;
    srcAddr.sin_port = sntpcPort;

    /* Create socket for listening. */
  
    sntpSocket = socket (AF_INET, SOCK_DGRAM, 0);
    if (sntpSocket == -1) 
        return (ERROR);
      
    result = bind (sntpSocket, (struct sockaddr *)&srcAddr, sizeof (srcAddr));
    if (result == -1) 
        {
        close (sntpSocket);
        return (ERROR);
        }

    /* Convert timeout value to format needed by select() call. */

    if (timeout != WAIT_FOREVER)
        {
        clockRate = sysClkRateGet ();
        sockTimeout.tv_sec = timeout / clockRate;
        sockTimeout.tv_usec = (1000000 * timeout % clockRate) / clockRate;
        }

    /* Wait for broadcast message from server. */

    FD_ZERO (&readFds);
    FD_SET (sntpSocket, &readFds);
      
    if (timeout == WAIT_FOREVER)
        result = select (FD_SETSIZE, &readFds, NULL, NULL, NULL);
    else
        result = select (FD_SETSIZE, &readFds, NULL, NULL, &sockTimeout);

    if (result == -1)
        {
        close (sntpSocket);
        errnoSet (S_sntpcLib_TIMEOUT);
        return (ERROR);
        }

    if (result == 0)    /* Timeout interval expired. */
        {
        close (sntpSocket);
        errnoSet (S_sntpcLib_TIMEOUT);
        return (ERROR);
        }

    result = recvfrom (sntpSocket, (caddr_t) &sntpMessage, sizeof(sntpMessage),
                       0, (struct sockaddr *) &srcAddr, &srcAddrLen);
    if (result == -1) 
        {
        close (sntpSocket);
        return (ERROR);
        }

    close (sntpSocket);

    /*
     * Return error if the server clock is unsynchronized, or the version is 
     * not supported.
     */

    if ( (sntpMessage.leapVerMode & SNTP_LI_MASK) == SNTP_LI_3 ||
        sntpMessage.transmitTimestampSec == 0)
        {
        errnoSet (S_sntpcLib_SERVER_UNSYNC);
        return (ERROR);
        }

    if ( (sntpMessage.leapVerMode & SNTP_VN_MASK) == SNTP_VN_0 ||
        (sntpMessage.leapVerMode & SNTP_VN_MASK) > SNTP_VN_3)
        {
        errnoSet (S_sntpcLib_VERSION_UNSUPPORTED);
        return (ERROR);
        }

    /* Convert the NTP timestamp to the correct format and store in clock. */

    /* Add test for 2036 base value here! */

    sntpMessage.transmitTimestampSec = 
                                     ntohl (sntpMessage.transmitTimestampSec) -
                                     SNTP_UNIX_OFFSET;

    /*
     * Adjust returned value if leap seconds are present.
     * This needs work!
     */

    /* if ( (sntpReply.leapVerMode & SNTP_LI_MASK) == SNTP_LI_1)
            sntpReply.transmitTimestampSec += 1;
     else if ((sntpReply.leapVerMode & SNTP_LI_MASK) == SNTP_LI_2)
              sntpReply.transmitTimestampSec -= 1;
    */

    sntpMessage.transmitTimestampFrac = 
                                     ntohl (sntpMessage.transmitTimestampFrac);

    pCurrTime->tv_sec = sntpMessage.transmitTimestampSec;
    pCurrTime->tv_nsec =
                       sntpcFractionToNsec (sntpMessage.transmitTimestampFrac);

    return (OK);
    }

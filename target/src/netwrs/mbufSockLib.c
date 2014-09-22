/* mbufSockLib.c - BSD mbuf socket interface library */

/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01f,15oct01,rae  merge from truestack
01e,04dec00,adb  if (errno = EWOULDBLOCK) changed to if (EWOULDBLOCK == errno)
01d,25aug97,vin  fixed SPR8908.
01d,22nov96,vin  added new cluster support, initialized header fields for
		 for every packet.
01c,13nov95,dzb  changed to validate mbufId.type off MBUF_VALID (SPR #4066).
01b,20nov94,kdl	 Fix blank line in comment block, for mangen.
01a,08nov94,dzb  written.
*/

/*
DESCRIPTION
This library contains routines that communicate over BSD sockets using
the `mbuf interface' described in the mbufLib manual page.  These mbuf
socket calls interoperate with BSD sockets in a similar manner to the
socket routines in sockLib, but they avoid copying data unnecessarily
between application buffers and network buffers.

NOMANUAL
*/

/* includes */

#include "vxWorks.h"
#include "sockLib.h"
#include "zbufSockLib.h"
#include "mbufSockLib.h"
#include "sys/socket.h"

/* declare mbuf socket interface function table */

LOCAL ZBUF_SOCK_FUNC	mbufSockFunc =
    {
    (FUNCPTR) _mbufLibInit,			/* back-end init pointer */
    (FUNCPTR) _mbufSockSend,
    (FUNCPTR) _mbufSockSendto,
    (FUNCPTR) _mbufSockBufSend,
    (FUNCPTR) _mbufSockBufSendto,
    (FUNCPTR) _mbufSockRecv,
    (FUNCPTR) _mbufSockRecvfrom
    };

/*******************************************************************************
*
* _mbufSockLibInit - initialize the BSD mbuf socket interface library
*
* The mbufSockLib API func table "mbufSockFunc" is published to the caller
* by the return value of this routine.  Typically, this func table is used by
* the zbufSock facility to call mbufSock routines within this library to
* perform socket operations.  Even though the zbufSock interface defines
* the ZBUF_SOCK_FUNC struct, it doesn't necessarily mean that zbufs must
* be present.  This library may be used even if zbufs have been scaled out
* of the system.
*
* RETURNS:
* A pointer to a func table containing the mbufSockLib API.
*
* NOMANUAL
*/

void * _mbufSockLibInit (void)
    {
    return ((void *) &mbufSockFunc);		/* no init necessary */
    }

/*******************************************************************************
*
* _mbufSockSend - send mbuf data to a socket
*
* This routine transmits all of the data in <mbufId> to a previously
* established connection-based (stream) socket.
*
* The <mbufLen> parameter is only used for determining the amount of space
* needed from the socket write buffer.  <mbufLen> has no affect on how many
* bytes are sent; the entire mbuf chain will be transmitted.  If the length of
* <mbufId> is not known, it must be determined using _mbufLength() before
* calling this routine.
*
* This routine transfers ownership of the mbuf chain from the user application
* to the VxWorks network stack.  The mbuf ID <mbufId> is deleted by this
* routine, and should not be used after this routine is called, even if
* an ERROR status is returned.  The exceptions to this are if this routine
* failed because the mbuf socket interface library was not uninitialized or an
* invalid mbuf ID was passed in, in which case <mbufId> is not deleted.
* Additionally, if the call fails during a non-blocking I/O socket write
* with an errno of EWOULDBLOCK, then <mbufId> is not deleted, so that it may
* be re-sent by the caller at a later time.
*
* RETURNS
* The number of bytes sent, or ERROR if the call fails.
*
* SEE ALSO: mbufLength
*
* NOMANUAL
*/

int _mbufSockSend
    (
    int			s,		/* socket to send to */
    MBUF_ID		mbufId,		/* mbuf chain to transmit */
    int			mbufLen,	/* length of entire mbuf chain */
    int			flags		/* flags to underlying protocols */
    )
    {
    MBUF_SEG		mbufSeg;		/* start of chain */
    int			retVal = 0;		/* send() return status */

    if (mbufId->type != MBUF_VALID)		/* invalid mbuf ID ? */
	{
	errno = S_mbufLib_ID_INVALID;
	return (ERROR);
	}

    if ((mbufSeg = mbufId->mbufHead) != NULL)	/* send mbuf chain */
	{
	mbufSeg->m_pkthdr.rcvif = NULL;
	mbufSeg->m_pkthdr.len = mbufLen;
        mbufSeg->m_flags |= M_PKTHDR;	
        retVal = send (s, (char *) mbufSeg, mbufLen, flags | MSG_MBUF);
	}

    /* do not delete mbuf ID on EWOULDBLOCK error so user can resend ID */

    if ((retVal != ERROR) || (errno != EWOULDBLOCK))
        MBUF_ID_DELETE_EMPTY(mbufId);

    return (retVal);
    }

/*******************************************************************************
*
* _mbufSockSendto - send a mbuf message to a socket
*
* This routine sends the entire message in <mbufId> to the datagram socket
* named by <to>.  The socket <s> will be received by the receiver as the
* sending socket.
*
* The <mbufLen> parameter is only used for determining the amount of space
* needed from the socket write buffer.  <mbufLen> has no affect on how many
* bytes are sent; the entire mbuf chain will be transmitted.  If the length of
* <mbufId> is not known, it must be determined using _mbufLength() before
* calling this routine.
*
* This routine transfers ownership of the mbuf chain from the user application
* to the VxWorks network stack.  The mbuf ID <mbufId> is deleted by this
* routine, and should not be used after this routine is called, even if
* an ERROR status is returned.  The exceptions to this are if this routine
* failed because the mbuf socket interface library was not uninitialized or an
* invalid mbuf ID was passed in, in which case <mbufId> is not deleted.
* Additionally, if the call fails during a non-blocking I/O socket write
* with an errno of EWOULDBLOCK, then <mbufId> is not deleted, so that it may
* be re-sent by the caller at a later time.
*
* RETURNS
* The number of bytes sent, or ERROR if the call fails.
*
* SEE ALSO: mbufLength
*
* NOMANUAL
*/

int _mbufSockSendto
    (
    int			s,		/* socket to send to */
    MBUF_ID		mbufId,		/* mbuf chain to transmit */
    int			mbufLen,	/* length of entire mbuf chain */
    int			flags,		/* flags to underlying protocols */
    struct sockaddr *	to,		/* recipient's address */
    int			tolen		/* length of <to> sockaddr */
    )
    {
    MBUF_SEG		mbufSeg;		/* start of chain */
    int			retVal = 0;		/* sendto() return status */

    if (mbufId->type != MBUF_VALID)		/* invalid mbuf ID ? */
	{
	errno = S_mbufLib_ID_INVALID;
	return (ERROR);
	}

    if ((mbufSeg = mbufId->mbufHead) != NULL)	/* send mbuf chain */
	{
	mbufSeg->m_pkthdr.rcvif = NULL;
	mbufSeg->m_pkthdr.len = mbufLen;
        mbufSeg->m_flags |= M_PKTHDR;	
        retVal = sendto (s, (char *) mbufSeg, mbufLen, flags | MSG_MBUF,
	to, tolen);
	}

    /* do not delete mbuf ID on EWOULDBLOCK error so user can resend ID */

    if ((retVal != ERROR) || (errno != EWOULDBLOCK))
        MBUF_ID_DELETE_EMPTY(mbufId);

    return (retVal);
    }

/*******************************************************************************
*
* _mbufSockBufSend - create an mbuf from user data and send it to a socket
*
* This routine creates an mbuf from the user buffer <buf>, and transmits
* it to a previously established connection-based (stream) socket.
*
* The user provided free routine <freeRtn> will be called when <buf>
* is no longer being used by the TCP/IP network stack.  If <freeRtn> is NULL,
* this routine will function normally, except that the user will not be
* notified when <buf> is released by the network stack.  <freeRtn> will be
* called from the context of the task that last references it.  This is
* typically either tNetTask context, or the caller's task context.
* <freeRtn> should be declared as follows:
* .CS
*       void freeRtn
*           (
*           caddr_t     buf,    /@ pointer to user buffer @/
*           int         freeArg /@ user provided argument to free routine @/
*           )
* .CE
*
* RETURNS:
* The number of bytes sent, or ERROR if the call fails.
*
* NOMANUAL
*/

int _mbufSockBufSend
    (
    int			s,		/* socket to send to */
    char *		buf,		/* pointer to data buffer */
    int			bufLen,		/* number of bytes to send */
    VOIDFUNCPTR		freeRtn,	/* free routine callback */
    int			freeArg,	/* argument to free routine */
    int			flags		/* flags to underlying protocols */
    )
    {
    struct mbufId	mbufIdNew;		/* dummy ID to insert into */
    MBUF_SEG		mbufSeg;		/* start of mbuf chain */
    int			retVal = 0;		/* send() return status */

    mbufIdNew.mbufHead = NULL;			/* init dummy ID */
    mbufIdNew.type = MBUF_VALID;

    /* build an mbuf chain with the user buffer as only mbuf */

    if ((mbufSeg = _mbufInsertBuf (&mbufIdNew, NULL, 0, buf,
	bufLen, freeRtn, freeArg)) == NULL)
	{
        if (freeRtn != NULL)			/* call free routine on error */
	    (*freeRtn) (buf, freeArg);

        return (ERROR);
	}
    mbufSeg->m_pkthdr.rcvif = NULL;
    mbufSeg->m_pkthdr.len = bufLen; 
    mbufSeg->m_flags |= M_PKTHDR;	
    if (((retVal = send (s, (char *) mbufSeg, bufLen, flags | MSG_MBUF)) ==
	ERROR) && (errno == EWOULDBLOCK))
        m_freem (mbufSeg);			/* free for EWOULDBLOCK case */

    return (retVal);
    }

/*******************************************************************************
*
* _mbufSockBufSendto - create an mbuf from a message and send it to a socket
*
* This routine creates an mbuf from the user buffer <buf>, and sends
* it to the datagram socket named by <to>.  The socket <s> will be
* received by the receiver as the sending socket.
*
* The user provided free routine <freeRtn> will be called when <buf>
* is no longer being used by the UDP/IP network stack.  If <freeRtn> is NULL,
* this routine will function normally, except that the user will not be
* notified when <buf> is released by the network stack.  <freeRtn> will be
* called from the context of the task that last references it.  This is
* typically either tNetTask context or the caller's task context.
* <freeRtn> should be declared as follows:
* .CS
*       void freeRtn
*           (
*           caddr_t     buf,    /@ pointer to user buffer @/
*           int         freeArg /@ user provided argument to free routine @/
*           )
* .CE
*
* RETURNS:
* The number of bytes sent, or ERROR if the call fails.
*
* NOMANUAL
*/

int _mbufSockBufSendto
    (
    int			s,		/* socket to send to */
    char *		buf,		/* pointer to data buffer */
    int			bufLen,		/* number of bytes to send */
    VOIDFUNCPTR		freeRtn,	/* free routine callback */
    int			freeArg,	/* argument to free routine */
    int			flags,		/* flags to underlying protocols */
    struct sockaddr *	to,		/* recipient's address */
    int			tolen		/* length of <to> sockaddr */
    )
    {
    struct mbufId	mbufIdNew;		/* dummy ID to insert into */
    MBUF_SEG		mbufSeg;		/* start of mbuf chain */
    int			retVal = 0;		/* sendto() return status */

    mbufIdNew.mbufHead = NULL;			/* init dummy ID */
    mbufIdNew.type = MBUF_VALID;

    /* build an mbuf chain with the user buffer as only mbuf */

    if ((mbufSeg = _mbufInsertBuf (&mbufIdNew, NULL, 0, buf,
	bufLen, freeRtn, freeArg)) == NULL)
	{
        if (freeRtn != NULL)			/* call free routine on error */
	    (*freeRtn) (buf, freeArg);

        return (ERROR);
	}
    mbufSeg->m_pkthdr.rcvif = NULL;
    mbufSeg->m_pkthdr.len = bufLen; 
    mbufSeg->m_flags |= M_PKTHDR;	
    if (((retVal = sendto (s, (char *) mbufSeg, bufLen, flags | MSG_MBUF,
        to, tolen)) == ERROR) && (EWOULDBLOCK == errno))
        m_freem (mbufSeg);			/* free for EWOULDBLOCK case */

    return (retVal);
    }

/*******************************************************************************
*
* _mbufSockRecv - receive data from a socket and place into a mbuf
*
* This routine receives data from a connection-based (stream) socket, and
* returns the data to the user in a newly created mbuf chain.
*
* The <pLen> parameter indicates the number of bytes requested by the caller.
* If the operation is successful, the number of bytes received will be
* returned through this paramter.
*
* RETURNS:
* The mbuf ID of a newly created mbuf chain containing the received data,
* or NULL if the operation failed.
*
* NOMANUAL
*/

MBUF_ID _mbufSockRecv
    (
    int			s,		/* socket to receive data from */
    int			flags,		/* flags to underlying protocols */
    int *		pLen		/* length of mbuf requested/received */
    )
    {
    MBUF_ID		mbufId;			/* mbuf returned to user */
    MBUF_SEG		mbufSeg = NULL;		/* received mbuf chain */
    int			status;			/* recv() return status */

    MBUF_ID_CREATE(mbufId);			/* create ID for recv data */
    if (mbufId == NULL)
        return (NULL);

    if ((status = (recv (s, (char *) &mbufSeg, *pLen, flags | MSG_MBUF))) ==
	ERROR)					/* Rx data from the socket */
	{
	MBUF_ID_DELETE_EMPTY(mbufId);
	return (NULL);
	}

    mbufId->mbufHead = mbufSeg;			/* hook into mbuf ID */
    *pLen = status;				/* stuff return length */

    return (mbufId);
    }

/*******************************************************************************
*
* _mbufSockRecvfrom - receive a message from a socket and place into a mbuf
*
* This routine receives a message from a datagram socket, and
* returns the message to the user in a newly created mbuf chain.
*
* The message is received regardless of whether the socket is connected.
* If <from> is non-zero, the address of the sender's socket is copied to it.
* The value-result parameter <pFromLen> should be initialized to the size of
* the <from> buffer.  On return, <pFromLen> contains the actual size of the
* address stored in <from>.
*
* The <pLen> parameter indicates the number of bytes requested by the caller.
* If the operation is successful, the number of bytes received will be
* returned through this paramter.
*
* RETURNS:
* The mbuf ID of a newly created mbuf chain containing the received message,
* or NULL if the operation failed.
*
* NOMANUAL
*/

MBUF_ID _mbufSockRecvfrom
    (
    int			s,		/* socket to receive from */
    int			flags,		/* flags to underlying protocols */
    int *		pLen,		/* length of mbuf requested/received */
    struct sockaddr *	from,		/* where to copy sender's addr */
    int *		pFromLen	/* value/result length of <from> */
    )
    {
    MBUF_ID		mbufId;			/* mbuf returned to user */
    MBUF_SEG		mbufSeg = NULL;		/* received mbuf chain */
    int			status;			/* recvfrom() return status */

    MBUF_ID_CREATE(mbufId);			/* create ID for recv data */
    if (mbufId == NULL)
        return (NULL);

    if ((status = (recvfrom (s, (char *) &mbufSeg, *pLen, flags | MSG_MBUF,
	from, pFromLen))) == ERROR)		/* Rx message from the socket */
	{
	MBUF_ID_DELETE_EMPTY(mbufId);
	return (NULL);
	}

    mbufId->mbufHead = mbufSeg;			/* hook into mbuf ID */
    *pLen = status;				/* stuff return length */

    return (mbufId);
    }

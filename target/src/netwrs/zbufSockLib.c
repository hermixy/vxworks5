/* zbufSockLib.c - zbuf socket interface library */

/* Copyright 1984 - 2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01l,07may02,kbw  man page edits
01k,15oct01,rae  merge from truestack ver 01l, base 01j (cleanup)
01j,18oct00,zhu  check NULL for sockFdtosockFunc
01i,29apr99,pul  Upgraded NPT phase3 code to tor2.0.0
01h,14mar99,jdi  doc: removed refs to config.h and/or configAll.h (SPR 25663).
01h,02feb99,sgv  Added checks for zbuf support in the underlying backend in
		 zbufSockSend,zbufSockSendto,zbufSockBufSend, zbufSockBufSendto,
		 zbufSockRecv, zbufSockRecvfrom calls.
01g,26oct95,rhp  doc: mention values for <flags> params in socket
                 calls (SPR#4423)
01f,10mar95,jdi  doc addn to zbufSockRecv(), zbufSockRecvfrom() as per dana.
01e,11feb95,jdi  fixed doc style for `errno'.
01d,23jan95,jdi  more doc tweaks.
01c,12nov94,rhp  doc tweaks.
01b,09nov94,rhp  library man page added, subroutine man pages edited.
01a,08nov94,dzb  written.
*/

/*
DESCRIPTION
This library contains routines that communicate over BSD sockets using
the
.I "zbuf interface"
described in the zbufLib manual page.  These zbuf
socket calls communicate over BSD sockets in a similar manner to the
socket routines in sockLib, but they avoid copying data unnecessarily
between application buffers and network buffers.


VXWORKS AE PROTECTION DOMAINS
Under VxWorks AE, this feature is accessible from the kernel protection 
domain only.  This restriction does not apply under non-AE versions of 
VxWorks.  

To use this feature, include the INCLUDE_ZBUF_SOCK component.

SEE ALSO
zbufLib, sockLib 
*/

/* includes */

#include "vxWorks.h"
#include "zbufSockLib.h"
#include "mbufSockLib.h"
#include "sys/socket.h"
#include "sockFunc.h"
#include "netLib.h"

/* locals */

LOCAL ZBUF_SOCK_FUNC	zbufSockFuncNull =	/* null funcs for unconnected */
    {
    NULL,					/* zbufLibInit()	*/
    NULL,					/* zbufSockSend()	*/
    NULL,					/* zbufSockSendto()	*/
    NULL,					/* zbufSockBufSend()	*/
    NULL,					/* zbufSockBufSendto()	*/
    NULL,					/* zbufSockRecv()	*/
    NULL					/* zbufSockRecvfrom()	*/
    };

LOCAL ZBUF_SOCK_FUNC *	pZbufSockFunc = &zbufSockFuncNull;

/* forward declarations */

IMPORT SOCK_FUNC * sockFdtosockFunc (int s);



/*******************************************************************************
*
* zbufSockLibInit - initialize the zbuf socket interface library
*
* This routine initializes the zbuf socket interface
* library.  It must be called before any zbuf socket routines are used.
* It is called automatically when INCLUDE_ZBUF_SOCK is defined.
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  In addition, all arguments to this function can  
* reference only that data which is valid in the kernel protection domain. 
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS:
* OK, or ERROR if the zbuf socket interface could not be initialized.
*/

STATUS zbufSockLibInit (void)
    {
    ZBUF_SOCK_FUNC *	pZbufSockFuncTemp;

    /* call the back-end initialization routine */

    if ((pZbufSockFuncTemp = _mbufSockLibInit()) == NULL)
	return (ERROR);
    
    pZbufSockFunc = pZbufSockFuncTemp;	/* connect socket back-end func table */

    return (zbufLibInit (pZbufSockFunc->libInitRtn));
    }

/*******************************************************************************
*
* zbufSockSend - send zbuf data to a TCP socket
*
* This routine transmits all of the data in <zbufId> to a previously
* established connection-based (stream) socket.
*
* The <zbufLen> parameter is used only for determining the amount of space
* needed from the socket write buffer.  <zbufLen> has no effect on how many
* bytes are sent; the entire zbuf is always transmitted.  If the length of
* <zbufId> is not known, the caller must first determine it by calling
* zbufLength().
*
* This routine transfers ownership of the zbuf from the user application
* to the VxWorks network stack.  The zbuf ID, <zbufId>, is deleted by this
* routine, and should not be used after the routine is called, even if
* an ERROR status is returned.  (Exceptions:  when the routine
* fails because the zbuf socket interface library was not initialized or an
* invalid zbuf ID was passed in, in which case there is no zbuf to delete.
* Moreover, if the call fails during a non-blocking I/O socket write
* with an `errno' of EWOULDBLOCK, then <zbufId> is not deleted; thus the
* caller may send it again at a later time.)
*
* You may OR the following values into the <flags> parameter with this
* operation:
*
* .iP "MSG_OOB (0x1)" 26
* Out-of-band data.
* 
* .iP "MSG_DONTROUTE (0x4)"
* Send without using routing tables.
* .LP
* 
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  In addition, all arguments to this function can  
* reference only that data which is valid in the kernel protection domain. 
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS
* The number of bytes sent, or ERROR if the call fails.
*
* SEE ALSO: zbufLength(), zbufSockBufSend(), send()
*/

int zbufSockSend
    (
    int			s,		/* socket to send to */
    ZBUF_ID		zbufId,		/* zbuf to transmit */
    int			zbufLen,	/* length of entire zbuf */
    int			flags		/* flags to underlying protocols */
    )
    {
    SOCK_FUNC * pSockFunc = sockFdtosockFunc(s);


    if (pSockFunc == NULL || pSockFunc->zbufRtn == NULL ||
	(pSockFunc->zbufRtn)() == FALSE)
	{
	netErrnoSet (ENOTSUP);
	return (ERROR);
	}

    return ((pZbufSockFunc->sendRtn == NULL) ? ERROR :
	(pZbufSockFunc->sendRtn) (s, zbufId, zbufLen, flags));
    }

/*******************************************************************************
*
* zbufSockSendto - send a zbuf message to a UDP socket
*
* This routine sends the entire message in <zbufId> to the datagram socket
* named by <to>.  The socket <s> is the sending socket.
*
* The <zbufLen> parameter is used only for determining the amount of space
* needed from the socket write buffer.  <zbufLen> has no effect on how many
* bytes are sent; the entire zbuf is always transmitted.  If the length of
* <zbufId> is not known, the caller must first determine it by calling
* zbufLength().
*
* This routine transfers ownership of the zbuf from the user application
* to the VxWorks network stack.  The zbuf ID <zbufId> is deleted by this
* routine, and should not be used after the routine is called, even if
* an ERROR status is returned.  (Exceptions:  when the routine
* fails because the zbuf socket interface library was not initialized or an
* invalid zbuf ID was passed in, in which case there is no zbuf to delete.
* Moreover, if the call fails during a non-blocking I/O socket write
* with an `errno' of EWOULDBLOCK, then <zbufId> is not deleted; thus the
* caller may send it again at a later time.)
*
* You may OR the following values into the <flags> parameter with this
* operation:
*
* .iP "MSG_OOB (0x1)" 26
* Out-of-band data.
* 
* .iP "MSG_DONTROUTE (0x4)"
* Send without using routing tables.
* .LP
* 
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  In addition, all arguments to this function can  
* reference only that data which is valid in the kernel protection domain. 
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS
* The number of bytes sent, or ERROR if the call fails.
*
* SEE ALSO: zbufLength(), zbufSockBufSendto(), sendto()
*/

int zbufSockSendto
    (
    int			s,		/* socket to send to */
    ZBUF_ID		zbufId,		/* zbuf to transmit */
    int			zbufLen,	/* length of entire zbuf */
    int			flags,		/* flags to underlying protocols */
    struct sockaddr *	to,		/* recipient's address */
    int			tolen		/* length of <to> socket addr */
    )
    {
    SOCK_FUNC * pSockFunc = sockFdtosockFunc(s);
    
    if (pSockFunc == NULL) 
	{
	netErrnoSet (ENOTSUP);
	return (ERROR);
	}

    if ((pSockFunc->zbufRtn == NULL) || ((pSockFunc->zbufRtn)() == FALSE))
	{
	netErrnoSet (ENOTSUP);
	return (ERROR);
	}

    return ((pZbufSockFunc->sendtoRtn == NULL) ? ERROR :
	(pZbufSockFunc->sendtoRtn) (s, zbufId, zbufLen, flags, to, tolen));
    }

/*******************************************************************************
*
* zbufSockBufSend - create a zbuf from user data and send it to a TCP socket
*
* This routine creates a zbuf from the user buffer <buf>, and transmits
* it to a previously established connection-based (stream) socket.
*
* The user-provided free routine callback at <freeRtn> is called when <buf>
* is no longer in use by the TCP/IP network stack.  Applications can
* exploit this callback to receive notification that <buf> is free.
* If <freeRtn> is NULL, the routine functions normally, except that the 
* application has no way of being notified when <buf> is released by the
* network stack.  The free routine runs in the context of the task that last
* references the buffer.  This is typically either the context of tNetTask, 
* or the context of the caller's task.  Declare <freeRtn> as follows
* (using whatever name is convenient):
* .CS
*       void freeCallback
*           (
*           caddr_t     buf,    /@ pointer to user buffer @/
*           int         freeArg /@ user-provided argument to free routine @/
*           )
* .CE
*
* You may OR the following values into the <flags> parameter with this
* operation:
*
* .iP "MSG_OOB (0x1)" 26
* Out-of-band data.
* 
* .iP "MSG_DONTROUTE (0x4)"
* Send without using routing tables.
* .LP
* 
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  In addition, all arguments to this function can  
* reference only that data which is valid in the kernel protection domain. 
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS:
* The number of bytes sent, or ERROR if the call fails.
*
* SEE ALSO
* zbufSockSend(), send()
*
*/

int zbufSockBufSend
    (
    int			s,		/* socket to send to */
    char *		buf,		/* pointer to data buffer */
    int		        bufLen,		/* number of bytes to send */
    VOIDFUNCPTR		freeRtn,	/* free routine callback */
    int			freeArg,	/* argument to free routine */
    int			flags		/* flags to underlying protocols */
    )
    {
    SOCK_FUNC * pSockFunc = sockFdtosockFunc(s);

    if (pSockFunc == NULL || pSockFunc->zbufRtn == NULL ||
	 (pSockFunc->zbufRtn)() == FALSE)
	{
	netErrnoSet (ENOTSUP);
	return (ERROR);
	}

    return ((pZbufSockFunc->bufSendRtn == NULL) ? ERROR :
	(pZbufSockFunc->bufSendRtn) (s, buf, bufLen, freeRtn, freeArg, flags));
    }

/*******************************************************************************
*
* zbufSockBufSendto - create a zbuf from a user message and send it to a UDP socket
*
* This routine creates a zbuf from the user buffer <buf>, and sends
* it to the datagram socket named by <to>.  The socket <s> is the
* sending socket.
*
* The user-provided free routine callback at <freeRtn> is called when <buf>
* is no longer in use by the UDP/IP network stack.  Applications can
* exploit this callback to receive notification that <buf> is free.
* If <freeRtn> is NULL, the routine functions normally, except that the 
* application has no way of being notified when <buf> is released by the
* network stack.  The free routine runs in the context of the task that last
* references the buffer.  This is typically either tNetTask context, 
* or the caller's task context.  Declare <freeRtn> as follows
* (using whatever name is convenient):
* .CS
*       void freeCallback
*           (
*           caddr_t     buf,    /@ pointer to user buffer @/
*           int         freeArg /@ user-provided argument to free routine @/
*           )
* .CE
*
* You may OR the following values into the <flags> parameter with this
* operation:
*
* .iP "MSG_OOB (0x1)" 26
* Out-of-band data.
* 
* .iP "MSG_DONTROUTE (0x4)"
* Send without using routing tables.
* .LP
* 
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  In addition, all arguments to this function can  
* reference only that data which is valid in the kernel protection domain. 
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS:
* The number of bytes sent, or ERROR if the call fails.
*
* SEE ALSO
* zbufSockSendto(), sendto()
*/

int zbufSockBufSendto
    (
    int			s,		/* socket to send to */
    char *		buf,		/* pointer to data buffer */
    int		        bufLen,		/* number of bytes to send */
    VOIDFUNCPTR		freeRtn,	/* free routine callback */
    int			freeArg,	/* argument to free routine */
    int			flags,		/* flags to underlying protocols */
    struct sockaddr *	to,		/* recipient's address */
    int			tolen		/* length of <to> socket addr */
    )
    {
    SOCK_FUNC * pSockFunc = sockFdtosockFunc(s);

    if (pSockFunc == NULL || pSockFunc->zbufRtn == NULL ||
	(pSockFunc->zbufRtn)() == FALSE)
	{
	netErrnoSet (ENOTSUP);
	return (ERROR);
	}

    return ((pZbufSockFunc->bufSendtoRtn == NULL) ? ERROR :
	(pZbufSockFunc->bufSendtoRtn) (s, buf, bufLen, freeRtn, freeArg,
	flags, to, tolen));
    }

/*******************************************************************************
*
* zbufSockRecv - receive data in a zbuf from a TCP socket
*
* This routine receives data from a connection-based (stream) socket, and
* returns the data to the user in a newly created zbuf.
*
* The <pLen> parameter indicates the number of bytes requested by the caller.
* If the operation is successful, the number of bytes received is
* copied to <pLen>.
*
* You may OR the following values into the <flags> parameter with this
* operation:
*
* .iP "MSG_OOB (0x1)" 26
* Out-of-band data.
* 
* .iP "MSG_PEEK (0x2)"
* Return data without removing it from socket.
* .LP
* 
* Once the user application is finished with the zbuf, zbufDelete() should
* be called to return the zbuf memory buffer to the VxWorks network stack.
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  In addition, all arguments to this function can  
* reference only that data which is valid in the kernel protection domain. 
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS:
* The zbuf ID of a newly created zbuf containing the received data,
* or NULL if the operation fails.
*
* SEE ALSO
* recv()
*/

ZBUF_ID zbufSockRecv
    (
    int			s,		/* socket to receive data from */
    int			flags,		/* flags to underlying protocols */
    int *		pLen		/* number of bytes requested/returned */
    )
    {
    SOCK_FUNC * pSockFunc = sockFdtosockFunc(s);

    if (pSockFunc == NULL || pSockFunc->zbufRtn == NULL || (pSockFunc->zbufRtn)() == FALSE)
	{
	netErrnoSet (ENOTSUP);
	return (NULL);
	}

    return ((pZbufSockFunc->recvRtn == NULL) ? NULL :
	(ZBUF_ID) (pZbufSockFunc->recvRtn) (s, flags, pLen));
    }

/*******************************************************************************
*
* zbufSockRecvfrom - receive a message in a zbuf from a UDP socket
*
* This routine receives a message from a datagram socket, and
* returns the message to the user in a newly created zbuf.
*
* The message is received regardless of whether the socket is connected.
* If <from> is nonzero, the address of the sender's socket is copied to it.
* Initialize the value-result parameter <pFromLen> to the size of
* the <from> buffer.  On return, <pFromLen> contains the actual size of the
* address stored in <from>.
*
* The <pLen> parameter indicates the number of bytes requested by the caller.
* If the operation is successful, the number of bytes received is
* copied to <pLen>.
*
* You may OR the following values into the <flags> parameter with this
* operation:
*
* .iP "MSG_OOB (0x1)" 26
* Out-of-band data.
* 
* .iP "MSG_PEEK (0x2)" 
* Return data without removing it from socket.  
* .LP
* 
* Once the user application is finished with the zbuf, zbufDelete() should
* be called to return the zbuf memory buffer to the VxWorks network stack.
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  In addition, all arguments to this function can  
* reference only that data which is valid in the kernel protection domain. 
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS:
* The zbuf ID of a newly created zbuf containing the received message,
* or NULL if the operation fails.
*/

ZBUF_ID zbufSockRecvfrom
    (
    int			s,		/* socket to receive from */
    int			flags,		/* flags to underlying protocols */
    int *		pLen,		/* number of bytes requested/returned */
    struct sockaddr *	from,		/* where to copy sender's addr */
    int *		pFromLen	/* value/result length of <from> */
    )
    {
    SOCK_FUNC * pSockFunc = sockFdtosockFunc(s);

    if (pSockFunc == NULL || pSockFunc->zbufRtn == NULL ||
	(pSockFunc->zbufRtn)() == FALSE)
	{
	netErrnoSet (ENOTSUP);
	return (NULL);
	}

    return ((pZbufSockFunc->recvfromRtn == NULL) ? NULL :
	(ZBUF_ID) (pZbufSockFunc->recvfromRtn) (s, flags, pLen,
	from, pFromLen));
    }

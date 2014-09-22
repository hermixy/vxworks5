/* bsdSockLib.c - UNIX BSD 4.4 compatible socket library */

/* Copyright 1984 - 2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01u,05nov01,vvv  fixed compilation warnings
01t,15oct01,rae  merge from truestack ver 01u, base 01r (SPRs 34005, 30608)
01s,10nov00,spm  merged from version 01s of tor3_x branch (SPR #30608 fix)
01r,29apr99,pul  Upgraded NPT phase3 code to tor2.0.0
01q,16mar99,spm  recovered orphaned code from tor2_0_x branch (SPR #25770)
01p,02feb99,sgv  Added zbufsockrtn routine
01o,02dec98,n_s  fixed sendmsg and recvmsg. spr 23552
01n,10nov98,spm  unmapped closed socket file descriptors (SPR #21583)
01m,26aug98,n_s  added return val check to mBufClGet in accept (). spr #22238.
01l,17mar98,vin  fixed a nonblocking I/O bug SPR 20542.
01k,17mar98,jmb  Merge patch by jmb from 03nov97: set small receive buffer 
                 which  HPSIM is TCP client. (SPR 8709) 
01j,17mar98,jmb  Merge patch by jmb from 31oct97: Set receive buffer when 
                 HPSIM is a TCP server.  Work-aroun for MTU mismatch and
                 pty problem in the SLIP connection.  (SPR 8709)
01i,10dec97,sgv  fixed a bug in recvmsg, intialized control and from variables
		 to NULL. 
01h,06oct97,sgv  fixed sendmsg/recvmsg call, mbufs are not freed when EINVAL 
		 is returned
01g,06oct97,spm  added optional binary compatibility for BSD 4.3 applications
01f,25jul97,vin  fixed so_accept function call, saved new fd in the socket
		 structure.
01e,03dec96,vin	 changed m_getclr to fit the new declaration.
01d,22nov96,vin  modified for BSD44.
01c,07aug95,dzb  NOMANUAL on headers.
01b,25jul95,dzb  changed to just use DEV_HDR device header.
01a,21jul95,dzb  created from sockLib.c, ver 04g.
*/

/*
This library provides UNIX BSD 4.4 compatible socket calls.  These calls
may be used to open, close, read, and write sockets, either on the same
CPU or over a network.  The calling sequences of these routines are
identical to UNIX BSD 4.4.

ADDRESS FAMILY
VxWorks sockets support only the Internet Domain address family; use
AF_INET for the <domain> argument in subroutines that require it.
There is no support for the UNIX Domain address family.

IOCTL FUNCTIONS
Sockets respond to the following ioctl() functions.  These functions are
defined in the header files ioLib.h and ioctl.h.
.iP "FIONBIO" 18 3
Turns on/off non-blocking I/O.
.CS
    on = TRUE;
    status = ioctl (sFd, FIONBIO, &on);

.CE
.iP "FIONREAD"
Reports the number of bytes available to read on the socket.  On the return
of ioctl(), <bytesAvailable> has the number of bytes available to read
on the socket.
.CS
    status = ioctl (sFd, FIONREAD, &bytesAvailable);
.CE
.iP "SIOCATMARK"
Reports whether there is out-of-band data to be read on the socket.  On
the return of ioctl(), <atMark> will be TRUE (1) if there is out-of-band
data, otherwise it will be FALSE (0).
.CS
    status = ioctl (sFd, SIOCATMARK, &atMark);
.CE

INCLUDE FILES: types.h, bsdSockLib.h, socketvar.h

SEE ALSO: netLib,
.pG "Network"

NOMANUAL
*/

#include "vxWorks.h"
#include "sockFunc.h"
#include "bsdSockMap.h"		/* map socket routines to bsdxxx() */
#include "strLib.h"
#include "errnoLib.h"
#include "netLib.h"
#include "ioLib.h"
#include "iosLib.h"
#include "tickLib.h"
#include "vwModNum.h"
#include "net/systm.h"
#include "net/mbuf.h"
#include "net/protosw.h"
#include "net/socketvar.h"
#include "net/uio.h"
#include "bsdSockLib.h"
#include "netinet/in.h"

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#endif    /* VIRTUAL_STACK */

IMPORT STATUS soo_ioctl ();		/* for call into sys_socket.c */

/* locals */

LOCAL DEV_HDR	bsdSockDev;			/* BSD-specific device header */
LOCAL int	bsdSockDrvNum;			/* BSD socket driver number */
LOCAL char *	bsdSockName = "(socket)";	/* file name of BSD sockets */

/* globals */

BOOL 	bsdSock43ApiFlag = FALSE; 	/* Provide backward binary */
                                        /* compatibility for BSD 4.3? */
                                        /* (Set TRUE by default with */
                                        /* BSD43_COMPATIBLE in configAll.h). */

IMPORT SOCK_FUNC ** pSockFdMap;

/* declare BSD socket interface function table */

SOCK_FUNC	bsdSockFunc =
    {
    (FUNCPTR) sockLibInit,
    (FUNCPTR) accept,
    (FUNCPTR) bind,
    (FUNCPTR) connect,
    (FUNCPTR) connectWithTimeout,
    (FUNCPTR) getpeername,
    (FUNCPTR) getsockname,
    (FUNCPTR) listen,
    (FUNCPTR) recv,
    (FUNCPTR) recvfrom,
    (FUNCPTR) recvmsg,
    (FUNCPTR) send,
    (FUNCPTR) sendto,
    (FUNCPTR) sendmsg,
    (FUNCPTR) shutdown,
    (FUNCPTR) socket,
    (FUNCPTR) getsockopt,
    (FUNCPTR) setsockopt,
    (FUNCPTR) zbufsockrtn
    };

/* forward declarations  */

LOCAL STATUS	bsdSockClose (struct socket *so);
LOCAL int	bsdSockWrite (struct socket *so, char *buf, int bufLen);
LOCAL int	bsdSockRead (struct socket *so, caddr_t buf, int bufLen);
LOCAL int	bsdSockargs (struct mbuf ** aname, caddr_t name,
		    int namelen, int type);
LOCAL void 	bsdSockAddrRevert (struct sockaddr *);

#ifdef VIRTUAL_STACK
LOCAL STATUS    stackInstFromSockVsidSet (int s);
#endif /* VIRTUAL_STACK */

/*******************************************************************************
*
* bsdSockLibInit - install the BSD "socket driver" into the I/O system
*
* bsdSockLibInit must be called once at configuration time before any socket
* operations are performed.
*
* RETURNS: a pointer to the BSD socket table, or NULL.
*
* NOMANUAL
*/

SOCK_FUNC * sockLibInit (void)
    {
    if (bsdSockDrvNum > 0)
        return (&bsdSockFunc);

    if ((bsdSockDrvNum = iosDrvInstall ((FUNCPTR) NULL, (FUNCPTR) NULL,
        (FUNCPTR) NULL, (FUNCPTR) bsdSockClose, (FUNCPTR) bsdSockRead,
	(FUNCPTR) bsdSockWrite, (FUNCPTR) soo_ioctl)) == ERROR)
	return (NULL);

    bsdSockDev.drvNum = bsdSockDrvNum;	/* to please I/O system */
    bsdSockDev.name   = bsdSockName;	/* for iosFdSet name optimiz. */

    return (&bsdSockFunc);
    }

/*******************************************************************************
*
* bsdSocket - open a socket
*
* This routine opens a socket and returns a socket descriptor.
* The socket descriptor is passed to the other socket routines to identify the
* socket.  The socket descriptor is a standard I/O system file descriptor
* (fd) and can be used with the close(), read(), write(), and ioctl() routines.
*
* Available socket types include:
* .iP SOCK_STREAM 18
* connection-based (stream) socket.
* .iP SOCK_DGRAM
* datagram (UDP) socket.
* .iP SOCK_RAW
* raw socket.
* .LP
*
* RETURNS: A socket descriptor, or ERROR.
*
* NOMANUAL
*/

int socket
    (
    int domain,         /* address family (e.g. AF_INET) */
    int type,           /* SOCK_STREAM, SOCK_DGRAM, or SOCK_RAW */
    int protocol        /* socket protocol (usually 0) */
    )
    {
    struct socket *so;
    int fd;
#ifndef  _WRS_VXWORKS_5_X
    OBJ_ID fdObject;
#endif
    FAST int spl = splnet ();
    int status = socreate (domain, &so, type, protocol);
    splx (spl);

    if (status)
	{
	netErrnoSet (status);
        return (ERROR);
	}

    /* stick the socket into a file descriptor */

    if ((fd = iosFdNew ((DEV_HDR *) &bsdSockDev, bsdSockName, (int) so)) ==
	ERROR)
	{
	(void)bsdSockClose (so);
	return (ERROR);
	}
#ifndef  _WRS_VXWORKS_5_X
    /* set ownership of socket semaphores */

    fdObject = iosFdObjIdGet (fd);
    if (fdObject == NULL)
        {
	(void)bsdSockClose (so);
	return (ERROR);
        }

    status = objOwnerSet (so->so_timeoSem, fdObject);
    if (status == ERROR)
        {
	(void)bsdSockClose (so);
	return (ERROR);
        }

    status = objOwnerSet (so->so_rcv.sb_Sem, fdObject);
    if (status == ERROR)
        {
	(void)bsdSockClose (so);
	return (ERROR);
        }

    status = objOwnerSet (so->so_snd.sb_Sem, fdObject);
    if (status == ERROR)
        {
	(void)bsdSockClose (so);
	return (ERROR);
        }
#endif /*  _WRS_VXWORKS_5_X */
    
    /* save fd in the socket structure */

    so->so_fd = fd;

#ifdef VIRTUAL_STACK
    
    /* set the vsid in the socket to the current stack */

    status = setsockopt (fd, SOL_SOCKET, SO_VSID,
                                (char *)&myStackNum, sizeof (myStackNum));
    if (status)
        {
        netErrnoSet (status);
        (void)bsdSockClose (so);
        return (ERROR);
        }

#endif    /* VIRTUAL_STACK */
    
    return (fd);
    }

/*******************************************************************************
*
* bsdSockClose - close a socket
*
* bsdSockClose is called by the I/O system to close a socket.
*
*/

LOCAL STATUS bsdSockClose
    (
    struct socket *so
    )
    {
    FAST int ret;
    FAST int sx;
    int fd;

#ifdef VIRTUAL_STACK
    /* Check if we are in the correct stack */
    
    if (stackInstFromSockVsidSet (so->so_fd) != OK)
        return (ERROR);

#endif    /* VIRTUAL_STACK */

    sx = splnet ();

    fd = so->so_fd;

    if (so->so_state & SS_NOFDREF)
        ret = 0;
    else
        ret = soclose (so);

    splx (sx);

    /* 
     * Remove the mapping installed by the switchboard. Performing this
     * step here is ugly, but the alternative is worse.
     */

    pSockFdMap [fd] = NULL;

    return (ret);
    }

/*******************************************************************************
*
* bsdBind - bind a name to a socket
*
* This routine associates a network address (also referred to as its "name")
* with a specified socket so that other processes can connect or send to it.
* When a socket is created with socket(), it belongs to an address family
* but has no assigned name.
*
* RETURNS:
* OK, or ERROR if there is an invalid socket, the address is either
* unavailable or in use, or the socket is already bound.
*
* NOMANUAL
*/

STATUS bind
    (
    int s,                      /* socket descriptor */
    struct sockaddr *name,      /* name to be bound */
    int namelen                 /* length of name */
    )
    {
    struct mbuf *nam;
    int status;
    struct socket *so;

    /* extract the socket from the fd */

    if ((so = (struct socket *) iosFdValue (s)) == (struct socket *) ERROR)
        return (ERROR);

#ifdef VIRTUAL_STACK

    if (stackInstFromSockVsidSet (s) != OK)
        return (ERROR);

#endif    /* VIRTUAL_STACK */
    
    status = bsdSockargs (&nam, (caddr_t) name, namelen, MT_SONAME);

    if (status)
	{
	netErrnoSet (status);
        return (ERROR);
	}

    status = sobind (so, nam);

    if (status)
	{
	netErrnoSet (status);
	m_freem (nam);
        return (ERROR);
	}

    m_freem (nam);
    return (OK);
    }

/*******************************************************************************
*
* bsdListen - enable connections to a socket
*
* This routine enables connections to a socket.  It also specifies the
* maximum number of unaccepted connections that can be pending at one time
* (<backlog>).  After enabling connections with listen(), connections are
* actually accepted by accept().
*
* RETURNS: OK, or ERROR if the socket is invalid or unable to listen.
*
* NOMANUAL
*/

STATUS listen
    (
    int s,              /* socket descriptor */
    int backlog         /* number of connections to queue */
    )
    {
    FAST int status;
    FAST struct socket *so;

    /* extract the socket from the fd */

    if ((so = (struct socket *) iosFdValue (s)) == (struct socket *) ERROR)
        return (ERROR);

#ifdef VIRTUAL_STACK

    if (stackInstFromSockVsidSet (s) != OK)
        return (ERROR);

#endif    /* VIRTUAL_STACK */
    
    status = solisten (so, backlog);

    if (status)
	{
	netErrnoSet (status);
        return (ERROR);
	}

#if (CPU == SIMHPPA)
/*
 *  Set the size of the receive buffer.
 *  This is the way (without rewriting the user app) to get
 *  data from the master to auto-segment and fit in the pty.
 */
{
    int bLen = 660;
    return (setsockopt (s, SOL_SOCKET, SO_RCVBUF, &bLen, sizeof (bLen)));
}
#endif

    return (OK);
    }

/*******************************************************************************
*
* bsdAccept - accept a connection from a socket
*
* This routine accepts a connection on a socket, and returns a new socket
* created for the connection.  The socket must be bound to an address with
* bind(), and enabled for connections by a call to listen().  The accept()
* routine dequeues the first connection and creates a new socket with the
* same properties as <s>.  It blocks the caller until a connection is
* present, unless the socket is marked as non-blocking.
*
* The parameter <addrlen> should be initialized to the size of the available
* buffer pointed to by <addr>.  Upon return, <addrlen> contains the size in
* bytes of the peer's address stored in <addr>.
*
* RETURNS
* A socket descriptor, or ERROR if the call fails.
*
* NOMANUAL
*/

int accept
    (
    int s,                      /* socket descriptor */
    struct sockaddr *addr,      /* peer address */
    int *addrlen                /* peer address length */
    )
    {
    struct mbuf *nam;
    int namelen = *addrlen;
    int slev;
    FAST struct socket *so;
    FAST int fd;
#ifndef  _WRS_VXWORKS_5_X
    OBJ_ID fdObject;
    STATUS status;
#endif

    /* extract the socket from the fd */

    if ((so = (struct socket *) iosFdValue (s)) == (struct socket *) ERROR)
        return (ERROR);

#ifdef VIRTUAL_STACK

    if (stackInstFromSockVsidSet (s) != OK)
        return (ERROR);

#endif    /* VIRTUAL_STACK */

    slev = splnet ();

    if ((so->so_options & SO_ACCEPTCONN) == 0)
        {
        netErrnoSet (EINVAL);
        splx (slev);
        return (ERROR);
        }

    if ((so->so_state & SS_NBIO) && so->so_qlen == 0)
        {
        netErrnoSet (EWOULDBLOCK);
        splx(slev);
        return (ERROR);
        }

    while (so->so_qlen == 0 && so->so_error == 0)
        {
        if (so->so_state & SS_CANTRCVMORE)
            {
            so->so_error = ECONNABORTED;
            break;
            }
        ksleep (so->so_timeoSem);
        }

    if (so->so_error)
        {
        netErrnoSet ((int) so->so_error);
        so->so_error = 0;
        splx (slev);
        return (ERROR);
        }

    if ( (nam = mBufClGet (M_WAIT, MT_SONAME, CL_SIZE_128, TRUE)) == NULL)
	{
	netErrnoSet (ENOBUFS);
	splx (slev);
	return (ERROR);
	}

    {
    struct socket *aso = so->so_q;
    if (soqremque(aso, 1) == 0)
	panic ("accept");
    so = aso;
    }

    (void) soaccept (so, nam);

    if (addr)
        {
        if (namelen > nam->m_len)
            namelen = nam->m_len;
        /* XXX SHOULD COPY OUT A CHAIN HERE */
        (void) copyout (mtod(nam, caddr_t), (caddr_t)addr,
			namelen);
        (void) copyout ((caddr_t)&namelen, (caddr_t)addrlen,
			sizeof (*addrlen));

        if (bsdSock43ApiFlag)
            {
            /* Convert the address structure contents to the BSD 4.3 format. */

            bsdSockAddrRevert (addr);
            }
        }

    m_freem (nam);
    splx (slev);

    /* put the new socket into an fd */

    if ((fd = iosFdNew ((DEV_HDR *) &bsdSockDev, bsdSockName, (int) so)) ==
	ERROR)
	{
	(void)bsdSockClose (so);
	return (ERROR);
	}
#ifndef _WRS_VXWORKS_5_X
    
    /* set ownership of socket semaphores */
    
    fdObject = iosFdObjIdGet (fd);
    if (fdObject == NULL)
        {
        (void)bsdSockClose (so);
        return (ERROR);
        }

    status = objOwnerSet (so->so_timeoSem, fdObject);
    if (status == ERROR)
        {
        (void)bsdSockClose (so);
        return (ERROR);
        }

    status = objOwnerSet (so->so_rcv.sb_Sem, fdObject);
    if (status == ERROR)
        {
        (void)bsdSockClose (so);
        return (ERROR);
        }

    status = objOwnerSet (so->so_snd.sb_Sem, fdObject);
    if (status == ERROR)
        {
        (void)bsdSockClose (so);
        return (ERROR);
        }
#endif /*  _WRS_VXWORKS_5_X */
    
    /* save fd in the socket structure */

    so->so_fd = fd;

#ifdef VIRTUAL_STACK
    
    /* set the vsid in the socket to the current stack */

    if (setsockopt (fd, SOL_SOCKET, SO_VSID,
                            (char *)&myStackNum, sizeof (myStackNum)) != OK)
        {
        (void)bsdSockClose (so);
        return (ERROR);
        }

#endif    /* VIRTUAL_STACK */
    
    return (fd);
    }

/*******************************************************************************
*
* bsdConnect - initiate a connection to a socket
*
* If <s> is a socket of type SOCK_STREAM, this routine establishes a virtual
* circuit between <s> and another socket specified by <name>.  If <s> is of
* type SOCK_DGRAM, it permanently specifies the peer to which messages
* are sent.  If <s> is of type SOCK_RAW, it specifies the raw socket upon
* which data is to be sent and received.  The <name> parameter specifies the
* address of the other socket.
*
* RETURNS
* OK, or ERROR if the call fails.
*
* NOMANUAL
*/

STATUS connect
    (
    int s,                      /* socket descriptor */
    struct sockaddr *name,      /* addr of socket to connect */
    int namelen                 /* length of name, in bytes */
    )
    {
    struct mbuf *nam;
    int slev;
    int status;
    FAST struct socket *so;

    /* extract the socket from the fd */

    if ((so = (struct socket *) iosFdValue (s)) == (struct socket *) ERROR)
        return (ERROR);

#ifdef VIRTUAL_STACK

    if (stackInstFromSockVsidSet (s) != OK)
        return (ERROR);

#endif    /* VIRTUAL_STACK */

    /*
     * For non-blocking sockets, exit immediately if a previous connection
     * attempt is still pending.
     */

    slev = splnet ();

    if ((so->so_state & SS_NBIO) &&
	(so->so_state & SS_ISCONNECTING)) {
        splx (slev);
	netErrnoSet (EALREADY);
	return (ERROR);
	}

    splx (slev);

    status = bsdSockargs (&nam, (caddr_t)name, namelen, MT_SONAME);
    if (status)
	{
	netErrnoSet (status);
        return (ERROR);
	}

#if (CPU == SIMHPPA)
/*
 *  Set the size of the receive buffer.
 *  This is the way (without rewriting the user app) to get
 *  data from the master to auto-segment and fit in the pty.
 */
{
    int bLen = 660;
    status = setsockopt (s, SOL_SOCKET, SO_RCVBUF, &bLen, sizeof (bLen));
    if (status == ERROR)
	return (ERROR);
}
#endif

    /*
     * Attempt to establish the connection. For TCP sockets, this routine
     * just begins the three-way handshake process. For all other sockets,
     * it succeeds or fails immediately.
     */

    status = soconnect (so, nam);

    if (status)
	{
        /*
         * Fatal error: unable to record remote address or (TCP only)
         * couldn't send initial SYN segment.
         */

	netErrnoSet (status);
	so->so_state &= ~SS_ISCONNECTING;
	m_freem(nam);
	return (ERROR);
	}

    /*
     * For non-blocking sockets, exit if the connection attempt is
     * still pending. This condition only occurs for TCP sockets if
     * no SYN reply arrives before the soconnect() routine completes.
     */

    slev = splnet();

    if ((so->so_state & SS_NBIO) &&
        (so->so_state & SS_ISCONNECTING))
        {
        netErrnoSet (EINPROGRESS);
	splx (slev);
	m_freem (nam);
        return (ERROR);
        }

    /*
     * Wait until a pending connection completes or a timeout occurs.
     * This delay might occur for TCP sockets. Other socket types
     * "connect" or fail instantly.
     */

    while ((so->so_state & SS_ISCONNECTING) && so->so_error == 0)
        ksleep (so->so_timeoSem);

    if (so->so_error)
	{
        /* Connection attempt failed immediately or (TCP only) timed out. */

        status = ERROR;
	netErrnoSet ((int) so->so_error);
	so->so_error = 0;
	}
    else
        {
        /* Connection attempt succeeded. */

        status = OK;
        }

    splx (slev);
    m_freem (nam);
    return (status);
    }

/******************************************************************************
*
* bsdConnectWithTimeout - attempt socket connection within a specified duration
*
* This routine performs the same function as connect(); however, in addition,
* users can specify how long to continue attempting the new connection.  
*
* If the <timeVal> is a NULL pointer, this routine performs exactly like
* connect().  If <timeVal> is not NULL, it will attempt to establish a new
* connection for the duration of the time specified in <timeVal>, after
* which it will report a time-out error if the connection is not
* established.
*
* RETURNS: OK, or ERROR if a connection cannot be established before timeout
*
* SEE ALSO: connect()
*
* NOMANUAL
*/

STATUS connectWithTimeout
    (
    int                 sock,           /* socket descriptor */
    struct sockaddr     *adrs,          /* addr of the socket to connect */
    int                 adrsLen,        /* length of the socket, in bytes */
    struct timeval      *timeVal        /* time-out value */
    )
    {
    int			on = 0;
    fd_set		writeFds;
    int			retVal = ERROR;
    int 		error;
    int			peerAdrsLen;
    struct sockaddr	peerAdrs;
    struct socket	*so;

    if (timeVal == NULL)
	return (connect (sock, adrs, adrsLen));

    if ((so = (struct socket *) iosFdValue (sock)) == (struct socket *) ERROR)
        return (ERROR);

#ifdef VIRTUAL_STACK

    if (stackInstFromSockVsidSet (sock) != OK)
        return (ERROR);

#endif    /* VIRTUAL_STACK */
    
    if (!(so->so_state & SS_NBIO))
	{
	on = 1;	/* set NBIO to have the connect() return without pending */
        if ((ioctl (sock, FIONBIO, (int) &on)) == ERROR)
	    return (ERROR);
	}

    if (connect (sock, adrs, adrsLen) < 0)
	{
	/*
         * When a TCP socket is set to non-blocking mode, the connect() 
         * routine might return EINPROGRESS (if the three-way handshake
         * is not completed immediately) or EALREADY (if a previous connect
         * attempt is still pending). All other errors (for any socket type)
         * indicate a fatal problem, such as insufficient memory to record
         * the remote address or (for TCP) an inability to transmit the
         * initial SYN segment.
	 */

        error = errnoGet () & 0xffff;
	if (error == EINPROGRESS || error == EALREADY)
            {
	    /*
             * Use the select() routine to monitor the socket status. The
             * socket will be writable when the three-way handshake completes
             * or when the handshake timeout occurs.
	     */

	    FD_ZERO (&writeFds);
	    FD_SET (sock, &writeFds);

	    if (select (FD_SETSIZE, (fd_set *) NULL, &writeFds,
			(fd_set *) NULL, timeVal) > 0)
		{
		/* The FD_ISSET test is theoretically redundant, but safer. */

		if (FD_ISSET (sock, &writeFds))
		    {
		    /*
                     * The connection attempt has completed. The getpeername()
                     * routine retrieves the remote address if the three-way
                     * handshake succeeded and fails if the attempt timed out.
		     */

		    peerAdrsLen = sizeof (peerAdrs);

		    if (getpeername (sock, &peerAdrs, &peerAdrsLen) != ERROR)
			retVal = OK;
		    }
		}
	    else
		netErrnoSet (ETIMEDOUT);
	    }
	}
    else
	retVal = OK;

    if (on)
	{
	on = 0;		/* turn OFF the non-blocking I/O */
        if ((ioctl (sock, FIONBIO, (int) &on)) == ERROR)
	    return (ERROR);
	}

    return (retVal);
    }

/*******************************************************************************
*
* bsdSendto - send a message to a socket
*
* This routine sends a message to the datagram socket named by <to>.  The
* socket <s> will be received by the receiver as the sending socket.
* 
* The maximum length of <buf> is subject to the limits on UDP buffer
* size; see the discussion of SO_SNDBUF in the setsockopt() manual
* entry.
*
* RETURNS
* The number of bytes sent, or ERROR if the call fails.
* 
* SEE ALSO
* setsockopt()
*
* NOMANUAL
*/

int sendto
    (
    FAST int             s,             /* socket to send data to */
    FAST caddr_t         buf,           /* pointer to data buffer */
    FAST int             bufLen,        /* length of buffer */
    FAST int             flags,         /* flags to underlying protocols */
    FAST struct sockaddr *to,           /* recipient's address */
    FAST int             tolen          /* length of <to> sockaddr */
    )
    {
    FAST struct socket 	*so;
    FAST int		spl;
    int			status;
    struct mbuf		*mto;
    int			len;		/* value/result */
    struct uio		usrIo;		/* user IO structure */
    struct iovec	ioVec;		/* IO vector structure */
    struct mbuf *	pMbuf = NULL;	/* pointer to mbuf */
    struct uio *	pUsrIo = NULL;	/* pointer to User IO structure */

    /* extract the socket from the fd */

    if ((so = (struct socket *) iosFdValue (s)) == (struct socket *) ERROR)
	{
	if ((flags & MSG_MBUF) && (buf))
	    m_freem ((struct mbuf *)buf);

        return (ERROR);
	}

#ifdef VIRTUAL_STACK

    if (stackInstFromSockVsidSet (s) != OK)
        return (ERROR);

#endif    /* VIRTUAL_STACK */

    spl = splnet ();

    status = bsdSockargs (&mto, (char *) to, tolen, MT_SONAME);

    if (status)
	{
	netErrnoSet (status);
	if ((flags & MSG_MBUF) && (buf))
	    m_freem ((struct mbuf *)buf);

        splx (spl);
	return (ERROR);
	}

    if (flags & MSG_MBUF)
	{
	pMbuf = (struct mbuf *) buf; 
	len = bufLen;
	}
    else
	{
	usrIo.uio_iov = &ioVec;
	usrIo.uio_iovcnt = 1;
	usrIo.uio_offset = 0;
	usrIo.uio_resid = bufLen;
	usrIo.uio_segflg = UIO_USERSPACE;
	usrIo.uio_rw = UIO_WRITE;

	ioVec.iov_base = (caddr_t) buf;
	ioVec.iov_len = bufLen;
	pUsrIo = &usrIo; 
	len = usrIo.uio_resid;
	}

    status = sosend (so, mto,  pUsrIo, pMbuf, 0, flags);
    
    m_free (mto);

    splx (spl);

    if (status)
        {
        if (pUsrIo != NULL && pUsrIo->uio_resid != len &&
            (status == EINTR || status == EWOULDBLOCK))
            status = OK;
        }

    return (status == OK ? ((pUsrIo == NULL) ? len : (len - pUsrIo->uio_resid))
	    : ERROR);
    }

/*******************************************************************************
*
* bsdSend - send data to a socket
*
* This routine transmits data to a previously established connection-based
* (stream) socket.
*
* The maximum length of <buf> is subject to the limits
* on TCP buffer size; see the discussion of SO_SNDBUF in the
* setsockopt() manual entry.
*
* RETURNS
* The number of bytes sent, or ERROR if the call fails.
* 
* SEE ALSO
* setsockopt()
* 
*
* NOMANUAL
*/

int send
    (
    FAST int            s,              /* socket to send to */
    FAST char           *buf,           /* pointer to buffer to transmit */
    FAST int            bufLen,         /* length of buffer */
    FAST int            flags           /* flags to underlying protocols */
    )
    {
    FAST struct socket 	*so;
    FAST int		status;
    FAST int		spl;
    int			len;		/* value/result */
    struct uio		usrIo;		/* user IO structure */
    struct iovec	ioVec;		/* IO vector structure */
    struct mbuf *	pMbuf = NULL;	/* pointer to mbuf */
    struct uio *	pUsrIo = NULL;	/* pointer to User IO structure */

    /* extract the socket from the fd */

    if ((so = (struct socket *) iosFdValue (s)) == (struct socket *) ERROR)
	{
	if ((flags & MSG_MBUF) && (buf))
	    m_freem ((struct mbuf *)buf);

        return (ERROR);
	}

#ifdef VIRTUAL_STACK

    if (stackInstFromSockVsidSet (s) != OK)
        return (ERROR);

#endif    /* VIRTUAL_STACK */
    
    spl = splnet ();

    if (flags & MSG_MBUF)
	{
	pMbuf = (struct mbuf *) buf; 
	len = bufLen;
	}
    else
	{
	usrIo.uio_iov = &ioVec;
	usrIo.uio_iovcnt = 1;
	usrIo.uio_offset = 0;
	usrIo.uio_resid = bufLen;
	usrIo.uio_segflg = UIO_USERSPACE;
	usrIo.uio_rw = UIO_WRITE;

	ioVec.iov_base = (caddr_t) buf;
	ioVec.iov_len = bufLen;
	pUsrIo = &usrIo;
	len = usrIo.uio_resid;
	}

    status = sosend (so, (struct mbuf *) NULL,  pUsrIo, pMbuf,
		     (struct mbuf *)NULL, flags);
    splx (spl);

    if (status)
        {
        if (pUsrIo != NULL && pUsrIo->uio_resid != len &&
            (status == EINTR || status == EWOULDBLOCK))
            status = OK;
        }

    return (status == OK ? ((pUsrIo == NULL) ? len : (len - pUsrIo->uio_resid))
	    : ERROR);
    }

/*******************************************************************************
*
* bsdSockWrite - write to a socket
*
* This routine is called by the I/O system when a write is done on a socket.
*/

LOCAL int bsdSockWrite
    (
    FAST struct socket  *so,
    FAST char           *buf,
    FAST int            bufLen
    )
    {
    FAST int		status;
    FAST int		spl;
    int			len;		/* value/result */
    struct uio		usrIo;		/* user IO structure */
    struct iovec	ioVec;		/* IO vector structure */

#ifdef VIRTUAL_STACK

    if (stackInstFromSockVsidSet (so->so_fd) != OK)
        return (ERROR);

#endif    /* VIRTUAL_STACK */
    
    spl = splnet ();

    usrIo.uio_iov = &ioVec;
    usrIo.uio_iovcnt = 1;
    usrIo.uio_offset = 0;
    usrIo.uio_resid = bufLen;
    usrIo.uio_segflg = UIO_USERSPACE;
    usrIo.uio_rw = UIO_WRITE;

    ioVec.iov_base = (caddr_t) buf;
    ioVec.iov_len = bufLen;

    len = usrIo.uio_resid;

    status = sosend (so, (struct mbuf *) NULL,  &usrIo, (struct mbuf *)0,
		     (struct mbuf *)NULL, 0);
    splx (spl);

    if (status)
        {
        if (usrIo.uio_resid != len &&
            (status == EINTR || status == EWOULDBLOCK))
            status = OK;
        }
    return (status == OK ? (len - usrIo.uio_resid)  : ERROR);
    }

/*******************************************************************************
*
* bsdSendmsg - send a message to a socket
*
* This routine sends a message to a datagram socket.  It may be used in 
* place of sendto() to decrease the overhead of reconstructing the 
* message-header structure (`msghdr') for each message.
* 
* RETURNS
* The number of bytes sent, or ERROR if the call fails.
*
* NOMANUAL
*/

int sendmsg
    (
    int                 sd,     /* socket to send to */
    struct msghdr       *mp,    /* scatter-gather message header */
    int                 flags   /* flags to underlying protocols */
    )
    {
    FAST struct iovec 	*iov;
    FAST struct iovec   * pIovCopy; /* kernel copy of user iov */
    FAST int 		ix;
    struct socket 	*so;
    struct uio 		auio;
    struct iovec        aiov [UIO_SMALLIOV];  /* use stack for small user iov */
    struct mbuf		*to;
    struct mbuf		*control;
    int 		len;
    int 		status;
    int 		slev;
    BOOL                mallocedIov;    /* TRUE if kernel iov copy malloced */
                                        /* from system pool */
    int                 sendLen;        /* length of sent message */

    if (flags & MSG_MBUF)
	{
        netErrnoSet (EINVAL);
        return (ERROR);			/* mbuf uio not supported */
	}

    /* extract the socket from the fd */

    if ((so = (struct socket *) iosFdValue (sd)) == (struct socket *) ERROR)
        return (ERROR);

#ifdef VIRTUAL_STACK

    if (stackInstFromSockVsidSet (sd) != OK)
        return (ERROR);

#endif    /* VIRTUAL_STACK */
    
    /* allocate space to copy user struct iovec */

    if (mp->msg_iovlen >= UIO_SMALLIOV)
	{
	if (mp->msg_iovlen >= UIO_MAXIOV)
	    {
	    netErrnoSet (EMSGSIZE);
	    return (ERROR);
	    }
	MALLOC (pIovCopy, struct iovec *, 
		sizeof (struct iovec) * mp->msg_iovlen, MT_DATA, M_WAIT);
	if (pIovCopy == NULL)
	    {
	    netErrnoSet (ENOBUFS);
	    return (ERROR);
	    }
	mallocedIov = TRUE;
	}
    else
	{
	pIovCopy = aiov;
	mallocedIov = FALSE;
	}

    slev = splnet ();

    auio.uio_iov 	= pIovCopy;
    auio.uio_iovcnt 	= mp->msg_iovlen;
    auio.uio_segflg 	= UIO_USERSPACE;
    auio.uio_offset 	= 0;
    auio.uio_rw 	= UIO_WRITE;
    auio.uio_resid 	= 0;
    iov 		= mp->msg_iov;

    for (ix = 0; ix < mp->msg_iovlen; ix++, iov++)
        {
        if (iov->iov_len < 0)
            {
            netErrnoSet (EINVAL);
	    splx (slev);
	    if (mallocedIov)
		{
		FREE (auio.uio_iov, MT_IOV);
		}
            return (ERROR);
            }

        if (iov->iov_len == 0)
            continue;

	pIovCopy->iov_len = iov->iov_len;
	pIovCopy->iov_base = iov->iov_base;
	pIovCopy++;

        auio.uio_resid += iov->iov_len;
        }

    /* Save a pointer to struct iovec copy */

    pIovCopy = auio.uio_iov;

    if (mp->msg_name)
        {
        status = bsdSockargs (&to, mp->msg_name, mp->msg_namelen, MT_SONAME);

        if (status)
	    {
	    netErrnoSet (status);
	    splx (slev);
	    if (mallocedIov)
		{
		FREE (pIovCopy, MT_DATA);
		}
            return (ERROR);
	    }
        }
    else
        to = 0;

    if (mp->msg_control)
        {
        status = bsdSockargs (&control, mp->msg_control, mp->msg_controllen,
                          MT_CONTROL);

        if (status)
	    {
	    if (to)
		m_freem(to);

	    netErrnoSet (status);
	    splx (slev);
	    if (mallocedIov)
		{
		FREE (pIovCopy, MT_DATA);
		}

	    return (ERROR);
	    }
        }
    else
        control = 0;

    len = auio.uio_resid;

    status = sosend (so, to, &auio, (struct mbuf *)0, control, flags);

    sendLen = len - auio.uio_resid;

    /* If status has been set to EINVAL in TCP, then TCP has freed the
     * mbufs. We do not free the mbufs, we return the errno
     * to the application.
     */

    if (status == EINVAL)
	{
	netErrnoSet (status);
	splx (slev);
	if (mallocedIov)
	    {
	    FREE (pIovCopy, MT_DATA);
	    }
	return (ERROR);
	}

    /* if status is not EINVAL, we free the mbufs as TCP has not freed it */

    if (control)
        m_freem(control);
    if (to)
        m_freem(to);

    splx (slev);

    if (mallocedIov)
	{
	FREE (pIovCopy, MT_DATA);
	}
    
    if (status)
	{
        if (auio.uio_resid != len &&
            (status == EINTR || status == EWOULDBLOCK))
            status = OK;
	}

    return (status == OK ? sendLen : ERROR);
    }

/*******************************************************************************
*
* bsdRecvfrom - receive a message from a socket
*
* This routine receives a message from a datagram socket regardless of 
* whether it is connected.  If <from> is non-zero, the address of the
* sender's socket is copied to it.  The value-result parameter <pFromLen>
* should be initialized to the size of the <from> buffer.  On return,
* <pFromLen> contains the actual size of the address stored in <from>.
* 
* The maximum length of <buf> is subject to the limits on UDP buffer
* size; see the discussion of SO_RCVBUF in the setsockopt() manual
* entry.
* 
* RETURNS
* The number of number of bytes received, or ERROR if the call fails.
* 
* SEE ALSO
* setsockopt()
*
* NOMANUAL
*/

int recvfrom
    (
    FAST int             s,         /* socket to receive from */
    FAST char            *buf,      /* pointer to data buffer */
    FAST int             bufLen,    /* length of buffer */
    FAST int             flags,     /* flags to underlying protocols */
    FAST struct sockaddr *from,     /* where to copy sender's addr */
    FAST int             *pFromLen  /* value/result length of <from> */
    )
    {
    FAST struct socket *so;
    FAST int		spl;
    FAST int		fromLen;
    int			status;
    struct mbuf	*	mfrom;
    int			len;			/* value/result */
    struct uio		usrIo;			/* user IO structure */
    struct iovec	ioVec;			/* IO vector structure */
    int			soFlags;
    struct mbuf **	ppRcvMbuf = NULL;	/* for zero copy interface */

#ifdef VIRTUAL_STACK
    if (stackInstFromSockVsidSet (s) != OK)
        return (ERROR);
#endif    /* VIRTUAL_STACK */

    /* extract the socket from the fd */

    if ((so = (struct socket *) iosFdValue (s)) == (struct socket *) ERROR)
        return (ERROR);

    spl = splnet ();

    usrIo.uio_iov = &ioVec;
    usrIo.uio_iovcnt = 1;
    usrIo.uio_offset = 0;
    usrIo.uio_resid = bufLen;
    usrIo.uio_segflg = UIO_USERSPACE;
    usrIo.uio_rw = UIO_READ;

    ioVec.iov_base = (caddr_t) buf;
    ioVec.iov_len = bufLen;
    soFlags = flags;

    if (flags & MSG_MBUF)		/* if zero copy interface enabled */
	ppRcvMbuf = (struct mbuf **)buf;

    len = usrIo.uio_resid;

    status = soreceive (so, &mfrom, &usrIo, ppRcvMbuf, (struct mbuf **)0, 
			&soFlags);

    if (from && pFromLen)
	{
        fromLen = *pFromLen;
    	if (fromLen <= 0 || mfrom == NULL)
	    fromLen = 0;
    	else
	    {
	    if (fromLen > mfrom->m_len)
	   	 fromLen = mfrom->m_len;

	    bcopy ((char *) mtod (mfrom, char *), (char *) from, fromLen);
	    }
    	*pFromLen = fromLen;

        if (bsdSock43ApiFlag)
            {
            /* Convert the address structure contents to the BSD 4.3 format. */

            bsdSockAddrRevert (from);
            }
	}

    if (mfrom != NULL)
        m_freem (mfrom);

    splx (spl);

    if (status)
        {
        if (usrIo.uio_resid != len &&
            (status == EINTR || status == EWOULDBLOCK))
            status = OK;
        }

    return (status == OK ? (len - usrIo.uio_resid)  : ERROR);
    }

/*******************************************************************************
*
* bsdRecv - receive data from a socket
*
* This routine receives data from a connection-based (stream) socket.
*
* The maximum length of <buf> is subject to the limits on TCP buffer
* size; see the discussion of SO_RCVBUF in the setsockopt() manual
* entry.
* 
* RETURNS
* The number of bytes received, or ERROR if the call fails.
* 
* SEE ALSO
* setsockopt()
*
* NOMANUAL
*/

int recv
    (
    FAST int    s,      /* socket to receive data from */
    FAST char   *buf,   /* buffer to write data to */
    FAST int    bufLen, /* length of buffer */
    FAST int    flags   /* flags to underlying protocols */
    )
    {
    FAST struct socket	*so;
    FAST int		spl;
    FAST int		status;
    int			len;
    struct uio		usrIo;		/* user IO structure */
    struct iovec	ioVec;		/* IO vector structure */
    int			soFlags;
    struct mbuf **	ppRcvMbuf = NULL;	/* for zero copy interface */

    /* extract the socket from the fd */

    if ((so = (struct socket *) iosFdValue (s)) == (struct socket *) ERROR)
        return (ERROR);

#ifdef VIRTUAL_STACK

    if (stackInstFromSockVsidSet (s) != OK)
        return (ERROR);

#endif    /* VIRTUAL_STACK */
    
    spl = splnet ();

    usrIo.uio_iov = &ioVec;
    usrIo.uio_iovcnt = 1;
    usrIo.uio_offset = 0;
    usrIo.uio_resid = bufLen;
    usrIo.uio_segflg = UIO_USERSPACE;
    usrIo.uio_rw = UIO_READ;

    ioVec.iov_base = (caddr_t) buf;
    ioVec.iov_len = bufLen;
    soFlags = flags;

    if (flags & MSG_MBUF)		/* if zero copy interface enabled */
	ppRcvMbuf = (struct mbuf **)buf;

    len = usrIo.uio_resid;

    status = soreceive (so, (struct mbuf **)NULL, &usrIo, ppRcvMbuf, 
			(struct mbuf **)0, &soFlags);
    splx (spl);

    if (status)
        {
        if (usrIo.uio_resid != len &&
            (status == EINTR || status == EWOULDBLOCK))
            status = OK;
        }

    return (status == OK ? (len - usrIo.uio_resid)  : ERROR);
    }

/*******************************************************************************
*
* bsdSockRead - read from a socket
*
* bsdSockRead is called by the I/O system to do a read on a socket.
*/

LOCAL int bsdSockRead
    (
    FAST struct socket  *so,
    FAST caddr_t        buf,
    FAST int            bufLen
    )
    {
    FAST int spl;
    FAST int status;
    int len;
    struct uio		usrIo;		/* user IO structure */
    struct iovec	ioVec;		/* IO vector structure */
    int			soFlags = 0;

#ifdef VIRTUAL_STACK

    if (stackInstFromSockVsidSet (so->so_fd) != OK)
        return (ERROR);

#endif    /* VIRTUAL_STACK */
    
    spl = splnet ();

    usrIo.uio_iov = &ioVec;
    usrIo.uio_iovcnt = 1;
    usrIo.uio_offset = 0;
    usrIo.uio_resid = bufLen;
    usrIo.uio_segflg = UIO_USERSPACE;
    usrIo.uio_rw = UIO_READ;

    ioVec.iov_base = (caddr_t) buf;
    ioVec.iov_len = bufLen;

    len = usrIo.uio_resid;

    status = soreceive (so, (struct mbuf **)NULL, &usrIo, (struct mbuf **)0, 
			(struct mbuf **)0, &soFlags);
    splx (spl);

    if (status)
        {
        if (usrIo.uio_resid != len &&
            (status == EINTR || status == EWOULDBLOCK))
            status = OK;
        }

    return (status == OK ? (len - usrIo.uio_resid)  : ERROR);
    }

/*******************************************************************************
*
* bsdRecvmsg - receive a message from a socket
*
* This routine receives a message from a datagram socket.  It may be used in
* place of recvfrom() to decrease the overhead of breaking down the
* message-header structure `msghdr' for each message.
*
* RETURNS
* The number of bytes received, or ERROR if the call fails.
*
* NOMANUAL
*/

int recvmsg
    (
    int                 sd,     /* socket to receive from */
    struct msghdr       *mp,    /* scatter-gather message header */
    int                 flags   /* flags to underlying protocols */
    )
    {
    FAST struct iovec 	*iov;
    FAST struct iovec   * pIovCopy; /* kernel copy of user iov */
    FAST int 		ix;
    caddr_t		namelenp;
    caddr_t		controllenp;
    struct socket	*so;
    struct uio 		auio;
    struct iovec        aiov [UIO_SMALLIOV];  /* use stack for small user iov */
    struct mbuf 	*from = NULL;
    struct mbuf		*control = NULL;
    int 		len;
    int 		status;
    int 		slev;
    BOOL                mallocedIov;    /* TRUE if kernel iov copy malloced */
                                        /* from system pool */
    int                 recvLen;        /* length of received message */

    if (flags & MSG_MBUF)
	{
        netErrnoSet (EINVAL);
        return (ERROR);			/* mbuf uio not supported */
	}

    /* extract the socket from the fd */

    if ((so = (struct socket *) iosFdValue (sd)) == (struct socket *) ERROR)
        return (ERROR);

#ifdef VIRTUAL_STACK

    if (stackInstFromSockVsidSet (sd) != OK)
        return (ERROR);

#endif    /* VIRTUAL_STACK */
    
    /* allocate space to copy user struct iovec */

    if (mp->msg_iovlen >= UIO_SMALLIOV)
	{
	if (mp->msg_iovlen >= UIO_MAXIOV)
	    {
	    netErrnoSet (EMSGSIZE);
	    return (ERROR);
	    }
	MALLOC (pIovCopy, struct iovec *, 
		sizeof (struct iovec) * mp->msg_iovlen, MT_DATA, M_WAIT);
	if (pIovCopy == NULL)
	    {
	    netErrnoSet (ENOBUFS);
	    return (ERROR);
	    }
	mallocedIov = TRUE;
	}
    else
	{
	pIovCopy = aiov;
	mallocedIov = FALSE;
	}

    slev = splnet ();

    namelenp	= (caddr_t) &mp->msg_namelen;
    controllenp	= (caddr_t) &mp->msg_controllen;
    mp->msg_flags = flags;
    
    auio.uio_iov        = pIovCopy;
    auio.uio_iovcnt 	= mp->msg_iovlen;
    auio.uio_segflg 	= UIO_USERSPACE;
    auio.uio_offset	= 0;
    auio.uio_resid 	= 0;
    auio.uio_rw 	= UIO_READ;
    iov 		= mp->msg_iov;

    for (ix = 0; ix < mp->msg_iovlen; ix++, iov++)
        {
        if (iov->iov_len < 0)
            {
            netErrnoSet (EINVAL);
	    splx (slev);
	    if (mallocedIov)
		{
		FREE (pIovCopy, MT_DATA);
		}
            return (ERROR);
            }

        if (iov->iov_len == 0)
            continue;
	
	pIovCopy->iov_len = iov->iov_len;
	pIovCopy->iov_base = iov->iov_base;
	pIovCopy++;

        auio.uio_resid += iov->iov_len;
        }

    /* Save a pointer to struct iovec copy */

    pIovCopy = auio.uio_iov;
    len = auio.uio_resid;

    status = soreceive (so, &from, &auio, (struct mbuf **)0, 
			mp->msg_control ? &control : (struct mbuf **)0,
			&mp->msg_flags);
    
    recvLen = len - auio.uio_resid;

    if (mp->msg_name)
        {
        len = mp->msg_namelen;

        if (len <= 0 || from == 0)
            len = 0;
        else
            {
            if (len > from->m_len)
                len = from->m_len;

            (void) copyout((caddr_t) mtod(from, caddr_t),
			   (caddr_t) mp->msg_name, len);
            }

        (void) copyout ((caddr_t) &len, namelenp, sizeof (int));

        if (bsdSock43ApiFlag)
            {
            /* Convert the address structure contents to the BSD 4.3 format. */

            bsdSockAddrRevert ((struct sockaddr *)mp->msg_name);
            }
        }

    if (mp->msg_control)
        {
        len = mp->msg_controllen;

        if (len <= 0 || control == 0)
            len = 0;
        else
            {
            if (len > control->m_len)
                len = control->m_len;

            (void) copyout ((caddr_t) mtod(control, caddr_t),
			    (caddr_t) mp->msg_control, len);
            }

        (void) copyout ((caddr_t) &len, controllenp, sizeof (int));
        }

    /* If status has been set to EINVAL in TCP, then TCP has freed the
     * mbufs. We do not free the mbufs, we return the errno
     * to the application.
     */

    if (status == EINVAL)
	{
	netErrnoSet (status);
	splx (slev);
	if (mallocedIov)
	    {
	    FREE (pIovCopy, MT_DATA);
	    }
	return (ERROR);
	}

    /* if status is not EINVAL, we free the mbufs as TCP has not freed it */

    if (control)
        m_freem(control);

    if (from)
        m_freem(from);

    splx (slev);

    if (mallocedIov)
	{
	FREE (pIovCopy, MT_DATA);
	}

    if (status)
	{
        if (auio.uio_resid != len &&
            (status == EINTR || status == EWOULDBLOCK))
            status = OK;
	}

    return (status == OK ? recvLen : ERROR);
    }

/*******************************************************************************
*
* bsdSetsockopt - set socket options
*
* This routine sets the options associated with a socket.
* To manipulate options at the "socket" level, <level> should be SOL_SOCKET.
* Any other levels should use the appropriate protocol number.
*
* OPTIONS FOR STREAM SOCKETS
* The following sections discuss the socket options available for
* stream (TCP) sockets.
*
* .SS "SO_KEEPALIVE -- Detecting a Dead Connection"
* Specify the SO_KEEPALIVE option to make the transport protocol (TCP)
* initiate a timer to detect a dead connection:
* .CS
*     setsockopt (sock, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof (optval));
* .CE
* This prevents an application from  hanging on an invalid connection. 
* The value at <optval> for this option is an integer (type `int'),
* either 1 (on) or 0 (off).
* 
* The integrity of a connection is verified by transmitting
* zero-length TCP segments triggered by a timer, to force a response
* from a peer node.  If the peer does not respond after repeated
* transmissions of the KEEPALIVE segments, the connection is dropped,
* all protocol data structures are reclaimed, and processes sleeping
* on the connection are awakened with an ETIMEDOUT error.
* 
* The ETIMEDOUT timeout can happen in two ways.  If the connection is
* not yet established, the KEEPALIVE timer expires after idling
* for TCPTV_KEEP_INIT.  If the connection is established, the
* KEEPALIVE timer starts up when there is no traffic for
* TCPTV_KEEP_IDLE.  If no response is received from the peer after
* sending the KEEPALIVE segment TCPTV_KEEPCNT times with interval
* TCPTV_KEEPINTVL, TCP assumes that the connection is invalid.
* The parameters TCPTV_KEEP_INIT, TCPTV_KEEP_IDLE, TCPTV_KEEPCNT, and
* TCPTV_KEEPINTVL are defined in the file vw/h/net/tcp_timer.h.
*
* .SS "SO_LINGER -- Closing a Connection"
* Specify the SO_LINGER option to determine whether TCP should perform a  
* "graceful" close:
* .CS
*     setsockopt (sock, SOL_SOCKET, SO_LINGER, &optval, sizeof (optval));
* .CE
* For a "graceful" close, when a connection is shut down TCP tries
* to make sure that all the unacknowledged data in transmission channel
* are acknowledged, and the peer is shut down properly, by going through an
* elaborate set of state transitions.
* 
* The value at <optval> indicates the amount of time to linger if
* there is unacknowledged data, using `struct linger' in
* vw/h/sys/socket.h.  The `linger' structure has two members:
* `l_onoff' and `l_linger'.  `l_onoff' can be set to 1 to turn on the
* SO_LINGER option, or set to 0 to turn off the SO_LINGER option.
* `l_linger' indicates the amount of time to linger.  If `l_onoff' is
* turned on and `l_linger' is set to 0, a default value TCP_LINGERTIME
* (specified in netinet/tcp_timer.h) is used for incoming
* connections accepted on the socket.
* 
* When SO_LINGER is turned on and the `l_linger' field is set to 0,
* TCP simply drops the connection by sending out an RST if a
* connection is already established; frees up the space for the TCP
* protocol control block; and wakes up all tasks sleeping on the
* socket.
* 
* For the client side socket, the value of `l_linger' is not changed
* if it is set to 0.  To make sure that the value of `l_linger' is 0
* on a newly accepted socket connection, issue another setsockopt()
* after the accept() call.
* 
* Currently the exact value of `l_linger' time is actually ignored
* (other than checking for 0); that is, TCP performs the state
* transitions if `l_linger' is not 0, but does not explicitly use its
* value.
* 
* .SS "TCP_NODELAY -- Delivering Messages Immediately"
* Specify the TCP_NODELAY option for real-time protocols, such as the X
* Window System Protocol, that require immediate delivery of many small
* messages:
* .CS
*     setsockopt (sock, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof (optval));
* .CE
* The value at <optval> is an integer (type `int') set to either 1
* (on) or 0 (off).
* 
* By default, the VxWorks TCP implementation employs an algorithm that
* attempts to avoid the congestion that can be produced by a large number
* of small TCP segments. This typically arises with virtual terminal
* applications (such as telnet or rlogin) across networks that have
* low bandwidth and long delays.  The algorithm attempts to have no
* more than one outstanding unacknowledged segment in the transmission
* channel while queueing up the rest of the smaller segments for later
* transmission.  Another segment is sent only if enough new data is
* available to make up a maximum sized segment, or if the outstanding
* data is acknowledged.
* 
* This congestion-avoidance algorithm works well for virtual terminal
* protocols and bulk data transfer protocols such as FTP without any
* noticeable side effects.  However, real-time protocols that require
* immediate delivery of many small messages, such as the X Window
* System Protocol, need to defeat this facility to guarantee proper
* responsiveness in their operation.
* 
* TCP_NODELAY is a mechanism to turn off the use of this algorithm.
* If this option is turned on and there is data to be sent out, TCP
* bypasses the congestion-avoidance algorithm: any available data
* segments are sent out if there is enough space in the send window.
* 
* OPTION FOR DATAGRAM SOCKETS
* The following section discusses an option for datagram (UDP) sockets.
* 
* .SS "SO_BROADCAST -- Sending to Multiple Destinations"
* Specify the SO_BROADCAST option when an application needs to send
* data to more than one destination:
* .CS
*     setsockopt (sock, SOL_SOCKET, SO_BROADCAST, &optval, sizeof (optval));
* .CE
* The value at <optval> is an integer (type <int>), either 1 (on) or 0
* (off).
* 
* OPTIONS FOR BOTH STREAM AND DATAGRAM SOCKETS
* The following sections describe options that can be used with either
* stream or datagram sockets.
* 
* .SS "SO_REUSEADDR -- Reusing a Socket Address"
* Specify the SO_REUSEADDR option to bind a stream socket to a local
* port that may be still bound to another stream socket:
* .CS
*     setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof (optval));
* .CE
* The value at <optval> is an integer (type <int>), either 1 (on) or 0
* (off).
* 
* When the SO_REUSEADDR option is turned on, applications may bind a
* stream socket to a local port even if it is still bound to another
* stream socket, if that other socket is associated with a "zombie" protocol
* control block context not yet freed from previous sessions.  The
* uniqueness of port number combinations for each connection is still
* preserved through sanity checks performed at actual connection
* setup time.  If this option is not turned on and an application
* attempts to bind to a port which is being used by a zombie protocol
* control block, the bind() call fails.
* 
* .SS "SO_SNDBUF -- Specifying the Size of the Send Buffer"
* Specify the SO_SNDBUF option to adjust the maximum size of the
* socket-level send buffer:
* .CS
*     setsockopt (sock, SOL_SOCKET, SO_SNDBUF, &optval, sizeof (optval));
* .CE
* The value at <optval> is an integer (type `int') that specifies the
* size of the socket-level send buffer to be allocated.
* 
* When stream or datagram sockets are created, each transport protocol
* reserves a set amount of space at the socket level for use when the
* sockets are attached to a protocol.  For TCP, the default size of
* the send buffer is 8192 bytes.  For UDP, the default size of the
* send buffer is 9216 bytes.  Socket-level buffers are allocated
* dynamically from the mbuf pool.
* 
* The effect of setting the maximum size of buffers (for both
* SO_SNDBUF and SO_RCVBUF, described below) is not actually to
* allocate the mbufs from the mbuf pool, but to set the high-water
* mark in the protocol data structure which is used later to limit the
* amount of mbuf allocation.  Thus, the maximum size specified for the
* socket level send and receive buffers can affect the performance of
* bulk data transfers.  For example, the size of the TCP receive
* windows is limited by the remaining socket-level buffer space.
* These parameters must be adjusted to produce the optimal result for
* a given application.
* 
* .SS "SO_RCVBUF -- Specifying the Size of the Receive Buffer"
* Specify the SO_RCVBUF option to adjust the maximum size of the
* socket-level receive buffer:
* .CS
*     setsockopt (sock, SOL_SOCKET, SO_RCVBUF, &optval, sizeof (optval));
* .CE
* The value at <optval> is an integer (type `int') that specifies the
* size of the socket-level receive buffer to be allocated.
* 
* When stream or datagram sockets are created, each transport protocol
* reserves a set amount of space at the socket level for use when the
* sockets are attached to a protocol.  For TCP, the default size is
* 8192 bytes.  UDP reserves 41600 bytes, enough space for up to forty
* incoming datagrams (1 Kbyte each).
*
* See the SO_SNDBUF discussion above for a discussion of the impact of
* buffer size on application performance.
* 
* .SS "SO_OOBINLINE -- Placing Urgent Data in the Normal Data Stream"
* Specify the SO_OOBINLINE option to place urgent data within the
* normal receive data stream:
* .CS
*     setsockopt (sock, SOL_SOCKET, SO_OOBINLINE, &optval, sizeof (optval));
* .CE
* TCP provides an expedited data service which does not conform to the
* normal constraints of sequencing and flow control of data
* streams. The expedited service delivers "out-of-band" (urgent) data
* ahead of other "normal" data to provide interrupt-like services (for
* example, when you hit a ^C during telnet or rlogin session while
* data is being displayed on the screen.)
* 
* TCP does not actually maintain a separate stream to support the
* urgent data.  Instead, urgent data delivery is implemented as a
* pointer (in the TCP header) which points to the sequence number of
* the octet following the urgent data. If more than one transmission
* of urgent data is received from the peer, they are all put into the
* normal stream. This is intended for applications that cannot afford
* to miss out on any urgent data but are usually too slow to respond
* to them promptly.
* 
* RETURNS:
* OK, or ERROR if there is an invalid socket, an unknown option, an option
* length greater than CL_SIZE_128, insufficient mbufs, or the call is unable 
* to set the specified option.
*
* NOMANUAL
*/

STATUS setsockopt
    (
    int s,              /* target socket */
    int level,          /* protocol level of option */
    int optname,        /* option name */
    char *optval,       /* pointer to option value */
    int optlen          /* option length */
    )
    {
    FAST struct mbuf *m = NULL;
    FAST struct socket *so;
    FAST int status;

    /* extract the socket from the fd */

    if ((so = (struct socket *) iosFdValue (s)) == (struct socket *) ERROR)
        return (ERROR);

    if (optlen > CL_SIZE_128)
        {
        netErrnoSet (EINVAL);
        return (ERROR);
        }

    if (optval)
        {
        m = mBufClGet(M_WAIT, MT_SOOPTS, CL_SIZE_128, TRUE);
        if (m == NULL)
            {
	    netErrnoSet (ENOBUFS);
	    return (ERROR);
            }

	copyin(optval, mtod(m, caddr_t), optlen);
        m->m_len = optlen;
        }

    if ((status = sosetopt (so, level, optname, m)) != 0)
	{
	netErrnoSet (status);
	return (ERROR);
	}

    return (OK);
    }

/*******************************************************************************
*
* bsdGetsockopt - get socket options
*
* This routine returns relevant option values associated with a socket.
* To manipulate options at the "socket" level, <level> should be SOL_SOCKET.
* Any other levels should use the appropriate protocol number.
*
* RETURNS:
* OK, or ERROR if there is an invalid socket, an unknown option, or the call
* is unable to get the specified option.
*
* SEE ALSO: setsockopt()
*
* NOMANUAL
*/

STATUS getsockopt
    (
    int         s,              /* socket */
    int         level,          /* protocol level for options */
    int         optname,        /* name of option */
    char        *optval,        /* where to put option */
    int         *optlen         /* where to put option length */
    )
    {
    FAST struct socket	*so;
    FAST int		status;
    int			valsize;
    struct mbuf 	*m = NULL;

    if ((so = (struct socket *) iosFdValue (s)) == (struct socket *) ERROR)
        return (ERROR);

    if (optval)			/* optval can be NULL */
	copyin ((char *) optlen, (char *) &valsize, sizeof (valsize));
    else
	valsize = 0;

    if ((status = sogetopt (so, level, optname, &m)) != 0)
	{
	netErrnoSet (status);
	return (ERROR);
	}

    if (optval && valsize && m != NULL)
	{
	if (valsize > m->m_len)
	    valsize = m->m_len;

	copyout ((char *) mtod (m, char *), optval, valsize);
	copyout ((char *) &valsize, (char *) optlen, sizeof (valsize));
	}

    if (m != NULL)
	m_free (m);

    return (OK);
    }

/*******************************************************************************
*
* bsdSockargs - encapsulate socket arguments into an mbuf
*/

LOCAL int bsdSockargs
    (
    struct mbuf         **aname,
    caddr_t             name,
    int                 namelen,
    int			type
    )
    {
    FAST struct mbuf *m;
    FAST struct sockaddr *sa;

    if (namelen > CL_SIZE_128)
        return (EINVAL);

    m = mBufClGet (M_WAIT, type, CL_SIZE_128, TRUE);
    if (m == NULL)
        return (ENOBUFS);

    m->m_len = namelen;
    copyin (name, mtod(m, caddr_t), namelen);
    *aname = m;

    if (type == MT_SONAME)
	{
	sa = mtod(m, struct sockaddr *);

        if (bsdSock43ApiFlag) 	/* Accept calls from BSD 4.3 applications? */
            {
            /* 
             * Convert the address structure contents from the BSD 4.3 format.
             * For BSD 4.4, the 16-bit BSD 4.3 sa_family field has been split 
             * into an 8-bit sa_len field followed by an 8-bit sa_family field.
             * The resized 8-bit family field will be set correctly by BSD 4.3 
             * applications compiled for a big endian architecture, but the
             * new length field will be set to zero. However, that field is
             * set correctly by the default processing. The following code
             * handles BSD 4.3 applications compiled for a little endian
             * architecture.
             */

            if (sa->sa_family == 0 && sa->sa_len == AF_INET)
                {
                /* 
                 * Due to the reversed byte ordering, BSD 4.3 applications 
                 * which were compiled for a little endian architecture will 
                 * provide an address structure whose sa_len field contains 
                 * the family specification. This processing would also
                 * change the address structure of a BSD 4.4 application with 
                 * a family of AF_UNSPEC and a length of 2 into an AF_INET
                 * socket, but that address structure is not valid anyway.
                 */

                sa->sa_family = sa->sa_len;
                }
            }
	sa->sa_len = namelen;
	}

    return (OK);
    }

/*******************************************************************************
*
* bsdSockAddrRevert - convert a BSD 4.4 address structure to BSD 4.3 format
*
* This routine is provided for backward compatibility with BSD 4.3 
* applications. It changes the contents of the 8-bit sa_family and sa_len 
* fields used in BSD 4.4 to match the expected contents of the corresponding
* 16-bit sa_family field for BSD 4.3 addresses.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

LOCAL void bsdSockAddrRevert
    (
    struct sockaddr * 	pSrcAddr 	/* BSD 4.4 address for conversion */
    )
    {
#if (_BYTE_ORDER == _BIG_ENDIAN)

    /*
     * For BSD 4.3 applications compiled with a big endian architecture,
     * the sa_len field corresponds to the upper 8 bits of the family field,
     * and must be set to zero. The sa_family field is unchanged.
     */

    pSrcAddr->sa_len = 0;

#else

    /*
     * Due to the reversed byte ordering, BSD 4.3 applications compiled for
     * a little endian architecture expect the low-order bits of the family 
     * specification in the high-order bits of the associated address, which 
     * corresponds to the sa_len field of the BSD 4.4 address structure. The 
     * BSD 4.4 sa_family field will be interpreted as the high-order bits of
     * the family specification, and must be set to zero.
     */

    pSrcAddr->sa_len = pSrcAddr->sa_family;
    pSrcAddr->sa_family = 0;

#endif /* _BYTE_ORDER == _BIG_ENDIAN */

    return;
    }

/*******************************************************************************
*
* bsdGetsockname - get a socket name
*
* This routine gets the current name for the specified socket <s>.
* The parameter <namelen> should be initialized to indicate the amount of
* space referenced by <name>.  On return, the name of the socket is copied to
* <name> and the actual size of the socket name is copied to <namelen>.
*
* RETURNS: OK, or ERROR if the socket is invalid
* or not connected.
*
* NOMANUAL
*/

STATUS getsockname
    (
    int s,                      /* socket descriptor */
    struct sockaddr *name,      /* where to return name */
    int *namelen                /* space available in name, later */
                                /* filled in with actual name size */
    )
    {
    FAST struct mbuf *m;
    FAST int status;
    FAST struct socket *so;

    /* extract the socket from the fd */

    if ((so = (struct socket *) iosFdValue (s)) == (struct socket *) ERROR)
        return (ERROR);

#ifdef VIRTUAL_STACK

    if (stackInstFromSockVsidSet (s) != OK)
        return (ERROR);

#endif    /* VIRTUAL_STACK */
    
    m = m_getclr (M_WAIT, MT_SONAME, CL_SIZE_128, TRUE);

    if (m == NULL)
	{
	netErrnoSet (ENOBUFS);
	return (ERROR);
	}

    status = (*so->so_proto->pr_usrreq)(so, PRU_SOCKADDR, 0, m, 0);

    if (status)
	{
	m_freem(m);
	netErrnoSet (status);
	return (ERROR);
	}

    if (*namelen > m->m_len)
	*namelen = m->m_len;

    copyout (mtod(m, caddr_t), (caddr_t) name, *namelen);

    if (bsdSock43ApiFlag)
        {
        /* Convert the address structure contents to the BSD 4.3 format. */

        bsdSockAddrRevert (name);
        }

    m_freem(m);

    return (OK);
    }

/*******************************************************************************
*
* bsdGetpeername - get the name of a connected peer
*
* This routine gets the name of the peer connected to socket <s>.
* The parameter <namelen> should be initialized to indicate the amount of
* space referenced by <name>.  On return, the name of the socket is copied to
* <name> and the actual size of the socket name is copied to <namelen>.
*
* RETURNS: OK, or ERROR if the socket is invalid
* or not connected.
*
* NOMANUAL
*/

STATUS getpeername
    (
    int s,                      /* socket descriptor */
    struct sockaddr *name,      /* where to put name */
    int *namelen                /* space available in name, later */
                                /* filled in with actual name size */
    )
    {
    FAST struct socket *so;
    FAST struct mbuf *m;
    FAST int status;

    /* extract the socket from the fd */

    if ((so = (struct socket *) iosFdValue (s)) == (struct socket *) ERROR)
        return (ERROR);

#ifdef VIRTUAL_STACK

    if (stackInstFromSockVsidSet (s) != OK)
        return (ERROR);

#endif    /* VIRTUAL_STACK */    

    if ((so->so_state & SS_ISCONNECTED) == 0)
	{
	netErrnoSet (ENOTCONN);
	return (ERROR);
	}

    m = m_getclr(M_WAIT, MT_SONAME, CL_SIZE_128, TRUE);
    if (m == NULL)
	{
	netErrnoSet (ENOBUFS);
	return (ERROR);
	}

    status = (*so->so_proto->pr_usrreq)(so, PRU_PEERADDR, 0, m, 0);

    if (status)
	{
	netErrnoSet (status);
	m_freem (m);
	return (ERROR);
	}

    if (*namelen > m->m_len)
	*namelen = m->m_len;

    copyout (mtod(m, caddr_t), (caddr_t) name, *namelen);

    if (bsdSock43ApiFlag)
        {
        /* Convert the address structure contents to the BSD 4.3 format. */

        bsdSockAddrRevert (name);
        }

    m_freem (m);

    return (OK);
    }

/******************************************************************************
*
* bsdShutdown - shut down a network connection
*
* This routine shuts down all, or part, of a connection-based socket <s>.
* If the value of <how> is 0, receives will be disallowed.  If <how> is 1,
* sends will be disallowed.  If <how> is 2, both sends and receives are
* disallowed.
*
* RETURNS: OK, or ERROR if the socket is invalid or not connected.
*
* NOMANUAL
*/

STATUS shutdown
    (
    int s,      /* the socket to shutdown */
    int how     /* 0 = receives disallowed */
                /* 1 = sends disallowed */
                /* 2 = sends and receives disallowed */
    )
    {
    struct socket *so;

    /* extract the socket from the fd */

    if ((so = (struct socket *) iosFdValue (s)) == (struct socket *) ERROR)
        return (ERROR);

#ifdef VIRTUAL_STACK

    if (stackInstFromSockVsidSet (s) != OK)
        return (ERROR);

#endif    /* VIRTUAL_STACK */    

    return (soshutdown (so, how));
    }

STATUS bsdZbufSockRtn ( void )
   {

   /* This routine returns TRUE is the backend supports Zbufs, or
    * else it returns FALSE
    */

   return TRUE;
   }


#ifdef VIRTUAL_STACK

/******************************************************************************
*
*
* stackInstFromSockVsidSet - set stack instance from socket vsid
*
* This routine will lookup the value of the VSID in the socket and
* associate the current task's context with that of this VSID.
*
* RETURNS
* OK if we set it successfully, or ERROR if the call fails.
*
* SEE ALSO
* getsockopt(), virtualStackNumTaskIdSet()
*
* NOMANUAL
*/

STATUS stackInstFromSockVsidSet
    (
    int    s                    /* socket descriptor */
    )
    {
    int    stackInstance;       /* stack inst obtained using getsockopt */
    int    status;
    int    stackInstLen = sizeof (stackInstance); /* length of the stack instance */

    /*
     * get the stack instance associated with this socket
     * and check the context of the current task.
     * If they don't match, we can't proceed.
     */

    status = getsockopt (s, SOL_SOCKET, SO_VSID,
                              (char *)&stackInstance, &stackInstLen);

    if ((status != OK) || (myStackNum != stackInstance))
        {
        return (ERROR);
        }

    return (OK);
    }

#endif /* VIRTUAL_STACK */

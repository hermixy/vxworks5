/* sockLib.c - generic socket library */

/* Copyright 1984 - 2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
05g,27jun02,rae  removed excessive accept() argument validation
05f,21may02,vvv  updated connectWithTimeout doc (SPR #27684)
05e,10may02,kbw  making man page edits
05d,28apr02,rae  Corrected getsockname() doc (SPR #70327)
05c,26apr02,vvv  updated setsockopt doc (SPR #21992/28868/30335)
05b,07dec01,rae  merge from synth ver 05c (spr #28181 correctly)
05a,15oct01,rae  merge from truestack ver 05b base 04q (SPR #28181 etc.)
04z,12sep01,cyr  doc: correct typo per SPR 29064
04y,16jan01,cco  Add 'See Also' reference to UNIX network Programming
04x,10jan01,dgr  correcting comment header block for shutdown() as per
                 SPR#35529
04w,14nov00,rae  removed unused argument from sockLibAdd()
04v,10nov00,ham  doc: corrected setsockopt(SO_DEBUG) example (SPR #62267).
04u,24oct00,dgr  adding to setsockopt() comment header block (re: SPR #20562)
04t,18oct00,zhu  change return value for sockFdtosockFunc
04s,17oct00,zhu  calloc doesn't need zero out
04r,16oct00,zhu  zero out pSockFdMap and move file descriptor check funtion
		 here
04q,17aug99,pai  changed send() prototype to use const on the second param
                 (SPR 21829)
04t,08nov99,pul  T2 cumulative patch 2
04s,29apr99,pul  Upgraded NPT phase3 code to tor2.0.0
04r,16mar99,spm  recovered orphaned code from tor2_0_x branch (SPR #25770)
04q,-2feb99,sgv  Added bitFlags to the sockLibAdd routine, added routine
		 sockfdtosockFunc
04p,02dec98,n_s  updated comments for sendmsg and recvmsg. spr 23552
04o,10nov98,spm  changed socket map for removal of closed files (SPR #21583)
04n,15mar99,c_c  Doc: Updated setsockopt () with SO_DEBUG flag (SPR #9636).
04m,14dec97,jdi  doc: cleanup.
04l,10dec97,kbw  fixed man page problems 
04k,04aug97,kbw  fixed man page problems found in beta review
04j,15apr97,kbw  fixed minor man page format problems
04i,14apr97,vin  added changes to setsockopt docs.
04l,14jul97,dgp  doc: change ^C to CTRL-C in setsockopt()
04k,10jul97,dgp  doc: fix SPRs 8470 & 8471, parameters of getsockopt()
04j,31jul95,dzb  check for lib init in sockLibAdd() and socket().
04i,25jul95,dzb  added fd-to-pSockFunc table.  added sockLibAdd().
04h,21jul95,dzb  changed to be a BSD/STREAMS switchyard.
		 Previous sockLib.c file moved to bsdSockLib.c.
04i,26oct95,rhp  doc: mention values for <flags> params in socket
                 calls (SPR#4423)
04h,14oct95,jdi  doc: revised pathnames for Tornado.
04g,13mar95,dzb  added spl semaphore protection to socket() (SPR #4016).
04f,11feb95,jdi  doc format repairs.
04e,18jan95,jdi  doc cleanup for connectWithTimeout().
04d,14not94,rhp  added socket-option documentation to setsockopt man page,
                 and added pointers from send() and sendto() (SPR#3785);
                 specified no support for UNIX Domain sockets (SPR#3519,1432)
04c,18aug94,dzb  Removed the NOMANUAL from connectWithTimeout() (SPR #3550).
                 Added m_freem() calls for mbuf interface (MSG_MBUF).
04b,22apr93,caf  ansification: added cast to ioctl() parameter.
04a,11aug93,jmm  Changed ioctl.h and socket.h to sys/ioctl.h and sys/socket.h
03z,01mar93,jdi  doc: addition of ioctl() functions (SPR 1922); added
		 explanation of accept() parameters (SPR 1382).
03y,03feb93,jdi  documentation cleanup for 5.1.
03x,19aug92,smb  changed systime.h to sys/times.h.
03w,26may92,rrr  the tree shuffle
03v,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -changed copyright notice
03u,30apr91,jdi	 documentation tweaks.
03t,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by dnw.
03s,24mar91,jaa  documentation cleanup.
03r,07feb91,elh  changed recvfrom to check if from is NULL before filling it.
03q,31jan91,jaa	 documentation.
03p,25oct90,dnw  changed utime.h to systime.h.
03o,04oct90,shl  made connectWithTimeOut() NOMANUAL.
03n,02oct90,hjb  eliminated a race and a bug in sockClose().
03m,25may90,dnw  removed malloc of file name "(socket)" before calling iosFdSet.
		 added "(socket)" as device name for iosFdSet optimization.
03l,11may90,yao  added missing modification history (03k) for the last checkin.
03k,09may90,yao  typecasted malloc to (char *).
03j,07may90,hjb  added sockFdVerify(), connectWithTimeout(), a hack to avoid
		 register d2 trashing bug.
03i,19apr90,hjb  deleted sendit(), recvit() and modified sendmsg() and
		 recvmsg() accordingly, de-linted.
03h,18mar90,hjb  optimization of send(), sendto(), sockWrite(), recv(),
		 recvfrom(), sockRead().
03g,16mar90,rdc  removed polling version of select.
03f,09jun89,gae  cleanup of 03e.
03e,07jun89,gae  changed SOCKADDR back to "struct sockaddr".
03d,24may89,hjb  fixed a bug in bind(); when sobind() fails, we now free
		 the allocated mbuf before returning error code.
03c,14apr89,gae  made select backward compatible with BSD 4.2, fixed bug,
		 made time of 0 not actually delay, more optimizing.
03b,13apr89,rdc  made select detect passively closed sockets.
		 installed marc ullman's select optimizations.
03a,15oct88,dnw  changed call to sleep() to ksleep().
02z,13sep88,llk  Fixed bug in select.  Now it polls at least once at all times.
02y,26aug88,gae  lint.
02x,18aug88,gae  documentation.
02w,15jul88,llk  changed to malloc the name of a socket fd before calling
		   iosFdNew.
02v,22jun88,dnw  changed apparent socket filename from "(network)" to "(socket)"
02u,30may88,dnw  changed to v4 names.
02t,28may88,dnw  removed setNetStatus to netLib.
02s,04may88,jcf  changed so_timeoSem to so_timeoSemId
	   +ecs  fixed bug in select introduced in 02r.
02r,30mar88,gae  made mainly iosLib independent.  Cleaned up select(),
		   made it use BSD4.3 fd_set's and caught small bug.
02q,27jan88,jcf  made kernel independent.
02p,05jan88,rdc  added include of sysm.h
02o,14dec87,gae  added include of vxWorks.h!!!
02n,18nov87,ecs  documentation.
02m,17nov87,rdc  fixed documentation for setsockopt.
02l,04nov87,dnw  fixed warning from missing coercion in sendto.
02k,03nov87,rdc  added shutdown, cleaned up documentation.
02j,13oct87,rdc  added select, setSelectDelay. make setNetStatus not be LOCAL.
02i,30sep87,gae  removed use of fdTable by replacing with calls to iosFdSet().
02h,02may87,dnw  removed unnecessary includes.
02g,01apr87,ecs  fixed bugs in getsockname & getpeername (found by lint!)
		 changed references to "struct sockaddr" to "SOCKADDR".
02f,26mar87,rdc  made sendit and recvit call splnet first thing.
02e,23mar87,jlf  documentation.
02d,30jan87,rdc  added getsockname and getpeername.
		 sendit and recvit weren't always calling setNetStatus.
02c,24dec86,gae  changed stsLib.h to vwModNum.h.
02b,08nov86,dnw  fixed bug in closeSock of not freeing fd.
02a,15oct86,rdc  completely rewritten for new 4.3 network code.
01i,04sep86,jlf  minor documentation.
		 made listen return OK always.
01h,01jul86,jlf  minor documentation
01g,18jun86,rdc  delinted.
01f,29apr86,rdc  changed memAllocates to mallocs and de-linted.
		 Added getsockopt and setsockopt to implement KEEP_ALIVE.
01e,09apr86,rdc  all linked list code now uses lstLib.
		 added sockCloseAll and sockNumActive.
01d,28jan86,jlf  documentation
01c,20nov85,rdc  fixed accept to be 4.2 like
01b,08oct85,rdc  de-linted
01a,03oct85,rdc  written
*/

/*
This library provides UNIX BSD 4.4 compatible socket calls.  Use these 
calls to open, close, read, and write sockets. These sockets can join 
processes on the same CPU or on different CPUs between which there is a
network connection.  The calling sequences of these routines are identical 
to their equivalents under UNIX BSD 4.4.

However, although the socket interface is compatible with VxWorks, 
the VxWorks environment does affect how you use sockets. Specifically, 
the globally accessible file descriptors available in the single address 
space world of VxWorks require that you take extra precautions when 
closing a file descriptor.

You must make sure that you do not close the file descriptor on which 
a task is pending during an accept(). Although the accept() on the 
closed file descriptor sometimes returns with an error, the accept() 
can also fail to return at all. Thus, if you need to be able to close 
a socket connections file descriptor asynchronously, you may need to 
set up a semaphore-based locking mechanism that prevents the close 
while an accept() is pending on the file descriptor.

ADDRESS FAMILY
VxWorks sockets support only the Internet Domain address family.  Use
AF_INET for the <domain> argument in subroutines that require it.
There is no support for the UNIX Domain address family.

IOCTL FUNCTIONS
Sockets respond to the following ioctl() functions.  These functions are
defined in the header files ioLib.h and ioctl.h.
.IP 'FIONBIO' 15 
Turns on/off non-blocking I/O.
.CS
    on = TRUE;
    status = ioctl (sFd, FIONBIO, &on);
.CE
.IP 'FIONREAD'
Reports the number of read-ready bytes available on the socket.  On the 
return of ioctl(), <bytesAvailable> has the number of bytes available to 
read from the socket.
.CS
    status = ioctl (sFd, FIONREAD, &bytesAvailable);
.CE
.IP 'SIOCATMARK'
Reports whether there is out-of-band data to be read from the socket.  On
the return of ioctl(), <atMark> is TRUE (1) if there is out-of-band
data.  Otherwise, it is FALSE (0).
.CS
    status = ioctl (sFd, SIOCATMARK, &atMark);
.CE

To use this feature, include the following component:
INCLUDE_BSD_SOCKET

INCLUDE FILES: types.h, mbuf.h, socket.h, socketvar.h

SEE ALSO: netLib,
.I "UNIX Network Programming", by W. Richard Stevens
*/

/* includes */

#include "vxWorks.h"
#include "stdlib.h"
#include "iosLib.h"
#include "netLib.h"
#include "errnoLib.h"
#include "sockFunc.h"
#include "sockLib.h"
#include "memPartLib.h"
#include "netBufLib.h"
#include "string.h"

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#endif    /* VIRTUAL_STACK */

/* globals */

SOCK_FUNC ** 	pSockFdMap = NULL;

/* locals */

LOCAL SOCK_LIB_MAP *	pSockLibMapH = NULL;
LOCAL UINT		sockFdMax = 0;	/* for parameter verification */

/*******************************************************************************
*
* sockLibInit - initialize a socket library back-end library.
*
* RETURNS: OK, or ERROR if the back-end could not be initialized.
*
* NOMANUAL
*/

STATUS sockLibInit
    (
    int		fdMax			/* maximum file descriptors */
    )
    {
    if (pSockFdMap != NULL)		/* already initialized ? */
	return (OK);

    /* allocate fd-to-pSockFunc mapping array */

    if ((pSockFdMap = (SOCK_FUNC **) KHEAP_ALLOC(fdMax *
	sizeof (SOCK_FUNC **))) == NULL)
	return (ERROR);
    
    bzero ((char *)pSockFdMap, fdMax * sizeof(SOCK_FUNC **));
    sockFdMax = fdMax;
    return (OK);
    }

/*******************************************************************************
*
* sockLibAdd - add a socket library back-end
*
* RETURNS: OK, or ERROR if the socket back-end could not be added.
*
* NOMANUAL
*/

STATUS sockLibAdd
    (
    FUNCPTR	sockLibInitRtn,		/* back-end init routine */
    int		domainMap,		/* address family */
    int		domainReal		/* address family */
    )
    {
    SOCK_LIB_MAP *	pSockLibMap;	/* back-end mapping entry ptr */

    if (pSockFdMap == NULL)		/* initialized yet ? */
	return (ERROR);

    if ((pSockLibMap = (SOCK_LIB_MAP *) KHEAP_ALLOC(sizeof(SOCK_LIB_MAP))) == NULL)
	return (ERROR);

    /* init socket back-end */

    if ((sockLibInitRtn == NULL) ||
	((pSockLibMap->pSockFunc = (SOCK_FUNC *) (sockLibInitRtn) ()) == NULL))
	{
	KHEAP_FREE((char *)pSockLibMap);	/* back-end error */
	return (ERROR);
	}

    pSockLibMap->domainMap = domainMap;   /* stuff mapping address family */
    pSockLibMap->domainReal = domainReal; /* stuff real address family */
    pSockLibMap->pNext = pSockLibMapH;    /* hook into lib list */

    pSockLibMapH = pSockLibMap;		/* update lib list head */

    return (OK);
    }

/*******************************************************************************
*
* socket - open a socket
*
* This routine opens a socket and returns a socket descriptor.
* The socket descriptor is passed to the other socket routines to identify the
* socket.  The socket descriptor is a standard I/O system file descriptor
* (fd) and can be used with the close(), read(), write(), and ioctl() routines.
*
* Available socket types include:
* .IP SOCK_STREAM 18
* Specifies a connection-based (stream) socket.
* .IP SOCK_DGRAM
* Specifies a datagram (UDP) socket.
* .IP SOCK_RAW
* Specifies a raw socket.
* .LP
*
* RETURNS: A socket descriptor, or ERROR.
*/

int socket
    (
    int domain,         /* address family (for example, AF_INET) */
    int type,           /* SOCK_STREAM, SOCK_DGRAM, or SOCK_RAW */
    int protocol        /* socket protocol (usually 0) */
    )
    {
    SOCK_LIB_MAP *	pSockLibMap;
    int			newFd;

    if (pSockFdMap == NULL)		/* initialized yet ? */
	return (ERROR);

    /* search for a suitable back-end library entry */

    for (pSockLibMap = pSockLibMapH; (pSockLibMap != NULL) &&
	(pSockLibMap->domainMap != domain); pSockLibMap = pSockLibMap->pNext)
	;

    /* found library map entry ? */

    if ((pSockLibMap == NULL) || (pSockLibMap->pSockFunc->socketRtn == NULL))
	{
	netErrnoSet (ENOTSUP);
	return (ERROR);
	}

    /* call back-end to open a socket */

    if ((newFd = (pSockLibMap->pSockFunc->socketRtn)
	(pSockLibMap->domainReal, type, protocol)) == ERROR)
	return (ERROR);

    pSockFdMap[newFd] = pSockLibMap->pSockFunc;	/* stuff fd map array */

    return (newFd);
    }

/*******************************************************************************
*
* bind - bind a name to a socket
*
* This routine associates a network address (also referred to as its "name")
* with a specified socket so that other processes can connect or send to it.
* When a socket is created with socket(), it belongs to an address family
* but has no assigned name.
*
* RETURNS:
* OK, or ERROR if there is an invalid socket, the address is either
* unavailable or in use, or the socket is already bound.
*/

STATUS bind
    (
    int s,                      /* socket descriptor */
    struct sockaddr *name,      /* name to be bound */
    int namelen                 /* length of name */
    )
    {
    SOCK_FUNC *	pSockFunc = NULL;

    if (iosFdValue (s) ==  ERROR || name == NULL)
        return (ERROR);

    pSockFunc = pSockFdMap[s];

    if ((pSockFunc == NULL) || (pSockFunc->bindRtn == NULL))
	{
        netErrnoSet (ENOTSUP);
	return (ERROR);
	}

    return ((pSockFunc->bindRtn) (s, name, namelen));
    }

/*******************************************************************************
*
* listen - enable connections to a socket
*
* This routine enables connections to a socket.  It also specifies the
* maximum number of unaccepted connections that can be pending at one time
* (<backlog>).  After enabling connections with listen(), connections are
* actually accepted by accept().
*
* RETURNS: OK, or ERROR if the socket is invalid or unable to listen.
*/

STATUS listen
    (
    int s,              /* socket descriptor */
    int backlog         /* number of connections to queue */
    )
    {
    SOCK_FUNC *	pSockFunc = NULL;

   
    if (iosFdValue (s) ==  ERROR)
        return ERROR;

    pSockFunc = pSockFdMap[s];

    if (pSockFunc == NULL || pSockFunc->listenRtn == NULL)
	{
        netErrnoSet (ENOTSUP);
	return (ERROR);
	}

    return ((pSockFunc->listenRtn) (s, backlog));
    }

/*******************************************************************************
*
* accept - accept a connection from a socket
*
* This routine accepts a connection on a socket, and returns a new socket
* created for the connection.  The socket must be bound to an address with
* bind(), and enabled for connections by a call to listen().  The accept()
* routine dequeues the first connection and creates a new socket with the
* same properties as <s>.  It blocks the caller until a connection is
* present, unless the socket is marked as non-blocking.
*
* The <addrlen> parameter should be initialized to the size of the available
* buffer pointed to by <addr>.  Upon return, <addrlen> contains the size in
* bytes of the peer's address stored in <addr>.
*
* WARNING
* You must make sure that you do not close the file descriptor on which 
* a task is pending during an accept(). Although the accept() on the 
* closed file descriptor sometimes returns with an error, the accept() 
* can also fail to return at all. Thus, if you need to be able to close 
* a socket connections file descriptor asynchronously, you may need to 
* set up a semaphore-based locking mechanism that prevents the close 
* while an accept() is pending on the file descriptor.
*
* RETURNS
* A socket descriptor, or ERROR if the call fails.
*/

int accept
    (
    int s,                      /* socket descriptor */
    struct sockaddr *addr,      /* peer address */
    int *addrlen                /* peer address length */
    )
    {
    int		newFd;
    
    SOCK_FUNC *	pSockFunc = NULL;

    if (iosFdValue (s) ==  ERROR)
        {
        netErrnoSet (EINVAL);
        return (ERROR);
        }

    pSockFunc = pSockFdMap[s];
    if ((pSockFunc == NULL) || (pSockFunc->acceptRtn == NULL))
	{
        netErrnoSet (ENOTSUP);
	return (ERROR);
	}

    if ((newFd = (pSockFunc->acceptRtn) (s, addr, addrlen)) == ERROR)
	return (ERROR);

    pSockFdMap[newFd] = pSockFunc;		/* stuff fd map array */

    return (newFd);
    }

/*******************************************************************************
*
* connect - initiate a connection to a socket
*
* If <s> is a socket of type SOCK_STREAM, this routine establishes a virtual
* circuit between <s> and another socket specified by <name>.  If <s> is of
* type SOCK_DGRAM, it permanently specifies the peer to which messages
* are sent.  If <s> is of type SOCK_RAW, it specifies the raw socket upon
* which data is to be sent and received.  The <name> parameter specifies the
* address of the other socket.
*
* NOTE: If a socket with type SOCK_STREAM is marked non-blocking, this
* routine will return ERROR with an error number of EINPROGRESS or EALREADY
* if a connection attempt is pending. A later call will return ERROR and set
* the error number to EISCONN once the connection is established. The
* connection attempt must be repeated until that result occurs or until
* this routine establishes a connection immediately and returns OK.
*
* RETURNS: OK, or ERROR if the connection attempt does not complete.
*/

STATUS connect
    (
    int s,                      /* socket descriptor */
    struct sockaddr *name,      /* addr of socket to connect */
    int namelen                 /* length of name, in bytes */
    )
    {
    SOCK_FUNC *	pSockFunc = NULL;

    if (name == NULL)
        {
        netErrnoSet (EINVAL);
        return (ERROR);
        }
    
    if (iosFdValue (s) ==  ERROR)
        return (ERROR);

    pSockFunc = pSockFdMap[s];
    if ((pSockFunc == NULL) || (pSockFunc->connectRtn == NULL))
	{
        netErrnoSet (ENOTSUP);
	return (ERROR);
	}

    return ((pSockFunc->connectRtn) (s, name, namelen));
    }

/******************************************************************************
*
* connectWithTimeout - attempt socket connection within a specified duration
*
* Use this routine as an alternative to connect() when your application
* requires a shorter time out on a connection attempt.  By design, a TCP
* connection attempt times out after 75 seconds if unsuccessful.  Thus, a 
* blocking TCP socket connect() call might not return for 75 seconds.  A 
* connectWithTimeout() call lets you reduce this time out by scheduling an 
* abort of the connection attempt if it is not successful before <timeVal>.
* However, connectWithTimeout() does not actually change the TCP timeout
* value.  Thus, you cannot use connectWithTimeout() to lengthen the
* connection time out beyond the TCP default.
*
* In all respects other than the time out value, a connectWithTimeout() call 
* behaves exactly like connect(). Thus, if no application is listening for 
* connections at the other end, connectWithTimeout() returns immediately just 
* like connect().  If you specify a NULL pointer for <timeVal>, 
* connectWithTimeout() behaves exactly like a connect() call.
*
* RETURNS: OK, or ERROR if a new connection is not established before timeout.
*
* SEE ALSO: connect()
*/

STATUS connectWithTimeout
    (
    int                 sock,           /* socket descriptor */
    struct sockaddr     *adrs,          /* addr of the socket to connect */
    int                 adrsLen,        /* length of the socket, in bytes */
    struct timeval      *timeVal        /* time-out value */
    )
    {
    SOCK_FUNC *	pSockFunc = NULL;

    if (adrs == NULL)
        {
        netErrnoSet (EINVAL);
        return (ERROR);
        }
    
    if (iosFdValue (sock) ==  ERROR)
        return (ERROR);

    pSockFunc = pSockFdMap[sock];

    if ((pSockFunc == NULL) || (pSockFunc->connectWithTimeoutRtn == NULL))
	{
        netErrnoSet (ENOTSUP);
	return (ERROR);
	}

    return ((pSockFunc->connectWithTimeoutRtn) (sock, adrs,
	adrsLen, timeVal));
    }

/*******************************************************************************
*
* sendto - send a message to a socket
*
* This routine sends a message to the datagram socket named by <to>.  The
* socket <s> is received by the receiver as the sending socket.
*
* The maximum length of <buf> is subject to the limits on UDP buffer
* size. See the discussion of SO_SNDBUF in the setsockopt() manual
* entry.
*
* You can OR the following values into the <flags> parameter with this
* operation:
*
* .IP "MSG_OOB (0x1)" 26
* Out-of-band data.
*
* .IP "MSG_DONTROUTE (0x4)"
* Send without using routing tables.
* .LP
*
* RETURNS
* The number of bytes sent, or ERROR if the call fails.
*
* SEE ALSO
* setsockopt()
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

    SOCK_FUNC *	pSockFunc = NULL;

    if (buf == NULL || to == NULL)
        {
        netErrnoSet (EINVAL);
        return (ERROR);
        }

    if (iosFdValue (s) ==  ERROR)
        return (ERROR);

    pSockFunc = pSockFdMap[s];
    
    if ((pSockFunc == NULL) || (pSockFunc->sendtoRtn == NULL))
	{
        netErrnoSet (ENOTSUP);
	return (ERROR);
	}

    return ((pSockFunc->sendtoRtn) (s, buf, bufLen, flags, to,
	tolen));
    }

/*******************************************************************************
*
* send - send data to a socket
*
* This routine transmits data to a previously established connection-based
* (stream) socket.
*
* The maximum length of <buf> is subject to the limits
* on TCP buffer size; see the discussion of SO_SNDBUF in the
* setsockopt() manual entry.
*
* You may OR the following values into the <flags> parameter with this
* operation:
*
* .IP "MSG_OOB (0x1)" 26
* Out-of-band data.
*
* .IP "MSG_DONTROUTE (0x4)"
* Send without using routing tables.
* .LP
*
* RETURNS
* The number of bytes sent, or ERROR if the call fails.
*
* SEE ALSO
* setsockopt(), sendmsg()
*
*/

int send
    (
    FAST int            s,              /* socket to send to */
    FAST const char *   buf,            /* pointer to buffer to transmit */
    FAST int            bufLen,         /* length of buffer */
    FAST int            flags           /* flags to underlying protocols */
    )
    {

    SOCK_FUNC *	pSockFunc = NULL;

    if (buf == NULL || iosFdValue (s) ==  ERROR)
        return (ERROR);

    pSockFunc = pSockFdMap[s];
    if ((pSockFunc == NULL) || (pSockFunc->sendRtn == NULL))
	{
        netErrnoSet (ENOTSUP);
	return (ERROR);
	}

    return ((pSockFunc->sendRtn) (s, buf, bufLen, flags));
    }

/*******************************************************************************
*
* sendmsg - send a message to a socket
*
* This routine sends a message to a datagram socket.  It may be used in
* place of sendto() to decrease the overhead of reconstructing the
* message-header structure (`msghdr') for each message.
*
* For BSD 4.4 sockets a copy of the <mp>->msg_iov array will be made.  This
* requires a cluster from the network stack system pool of size 
* <mp>->msg_iovlen * sizeof (struct iovec) or 8 bytes.
*
* RETURNS
* The number of bytes sent, or ERROR if the call fails.
*
* SEE ALSO
* sendto()
*/

int sendmsg
    (
    int                 sd,     /* socket to send to */
    struct msghdr       *mp,    /* scatter-gather message header */
    int                 flags   /* flags to underlying protocols */
    )
    {

    SOCK_FUNC *	pSockFunc = NULL;

    if (mp == NULL || iosFdValue (sd) ==  ERROR)
	{
        netErrnoSet (EINVAL);
        return (ERROR);
	}

    pSockFunc = pSockFdMap[sd];
    if ((pSockFunc == NULL) || (pSockFunc->sendmsgRtn == NULL))
	{
        netErrnoSet (ENOTSUP);
	return (ERROR);
	}

    return ((pSockFunc->sendmsgRtn) (sd, mp, flags));
    }

/*******************************************************************************
*
* recvfrom - receive a message from a socket
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
* You may OR the following values into the <flags> parameter with this
* operation:
*
* .IP "MSG_OOB (0x1)" 26
* Out-of-band data.
*
* .IP "MSG_PEEK (0x2)"
* Return data without removing it from socket.
* .LP
*
* RETURNS
* The number of number of bytes received, or ERROR if the call fails.
*
* SEE ALSO
* setsockopt()
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

    SOCK_FUNC *	pSockFunc = NULL;

    if (buf == NULL || from == NULL || pFromLen == NULL || iosFdValue (s) ==  ERROR)
        {
        netErrnoSet (EINVAL);
        return (ERROR);
        }

    pSockFunc = pSockFdMap[s];
    if ((pSockFunc == NULL) || (pSockFunc->recvfromRtn == NULL))
	{
        netErrnoSet (ENOTSUP);
	return (ERROR);
	}

    return ((pSockFunc->recvfromRtn) (s, buf, bufLen, flags,
	from, pFromLen));
    }

/*******************************************************************************
*
* recv - receive data from a socket
*
* This routine receives data from a connection-based (stream) socket.
*
* The maximum length of <buf> is subject to the limits on TCP buffer
* size; see the discussion of SO_RCVBUF in the setsockopt() manual
* entry.
*
* You may OR the following values into the <flags> parameter with this
* operation:
*
* .IP "MSG_OOB (0x1)" 26
* Out-of-band data.
*
* .IP "MSG_PEEK (0x2)"
* Return data without removing it from socket.
* .LP
*
* RETURNS
* The number of bytes received, or ERROR if the call fails.
*
* SEE ALSO
* setsockopt()
*/

int recv
    (
    FAST int    s,      /* socket to receive data from */
    FAST char   *buf,   /* buffer to write data to */
    FAST int    bufLen, /* length of buffer */
    FAST int    flags   /* flags to underlying protocols */
    )
    {

    SOCK_FUNC *	pSockFunc = NULL;

    if (buf == NULL || iosFdValue (s) ==  ERROR)
        {
        netErrnoSet (EINVAL);
        return (ERROR);
        }

    pSockFunc = pSockFdMap[s];
    if ((pSockFunc == NULL) || (pSockFunc->recvRtn == NULL))
	{
        netErrnoSet (ENOTSUP);
	return (ERROR);
	}

    return ((pSockFunc->recvRtn) (s, buf, bufLen, flags));
    }

/*******************************************************************************
*
* recvmsg - receive a message from a socket
*
* This routine receives a message from a datagram socket.  It may be used in
* place of recvfrom() to decrease the overhead of breaking down the
* message-header structure `msghdr' for each message.
*
* For BSD 4.4 sockets a copy of the <mp>->msg_iov array will be made.  This
* requires a cluster from the network stack system pool of size 
* <mp>->msg_iovlen * sizeof (struct iovec) or 8 bytes. 
*
* RETURNS
* The number of bytes received, or ERROR if the call fails.
*/

int recvmsg
    (
    int                 sd,     /* socket to receive from */
    struct msghdr       *mp,    /* scatter-gather message header */
    int                 flags   /* flags to underlying protocols */
    )
    {

    SOCK_FUNC *	pSockFunc = NULL;

    if (mp == NULL || iosFdValue (sd) ==  ERROR)
         {
        netErrnoSet (EINVAL);
        return (ERROR);
        }

    pSockFunc = pSockFdMap[sd];
    if ((pSockFunc == NULL) || (pSockFunc->recvmsgRtn == NULL))
	{
        netErrnoSet (ENOTSUP);
	return (ERROR);
	}

    return ((pSockFunc->recvmsgRtn) (sd, mp, flags));
    }

/*******************************************************************************
*
* setsockopt - set socket options
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
* The TCPTV_KEEP_INIT, TCPTV_KEEP_IDLE, TCPTV_KEEPCNT, and TCPTV_KEEPINTVL 
* parameters are defined in the file target/h/netinet/tcp_timer.h.
*
* .SS "SO_LINGER -- Closing a Connection"
* Specify the SO_LINGER option to determine whether TCP should perform a
* "graceful" close:
* .CS
*     setsockopt (sock, SOL_SOCKET, SO_LINGER, &optval, sizeof (optval));
* .CE
* To achieve a "graceful" close in response to the shutdown of a connection,
* TCP puts itself through an elaborate set of state transitions.  The goal is 
* to assure that all the unacknowledged data in the transmission channel are 
* acknowledged, and that the peer is shut down properly.
*
* The value at <optval> indicates the amount of time to linger if
* there is unacknowledged data, using `struct linger' in
* target/h/sys/socket.h.  The `linger' structure has two members:
* `l_onoff' and `l_linger'.  `l_onoff' can be set to 1 to turn on the
* SO_LINGER option, or set to 0 to turn off the SO_LINGER option.
* `l_linger' indicates the amount of time to linger.  If `l_onoff' is
* turned on and `l_linger' is set to 0, a default value TCP_LINGERTIME
* (specified in netinet/tcp_timer.h) is used for incoming
* connections accepted on the socket.
*
* When SO_LINGER is turned on and the `l_linger' field is set to 0,
* TCP simply drops the connection by sending out an RST (if a
* connection is already established).  This frees up the space for the TCP
* protocol control block, and wakes up all tasks sleeping on the
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
* setsockopt (sock, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof (optval));
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
* .SS "TCP_MAXSEG -- Changing TCP MSS for the connection"
* Specify the TCP_MAXSEG option to decrease the maximum allowable size of an
* outgoing TCP segment. This option cannot be used to increase the MSS.
* .CS
* setsockopt (sock, IPPROTO_TCP, TCP_MAXSEG, &optval, sizeof (optval));
* .CE
* The value at <optval> is an integer set to the desired MSS (eg. 1024).
*
* When a TCP socket is created, the MSS is initialized to the default MSS
* value which is determined by the configuration parameter `TCP_MSS_DFLT' (512
* by default). When a connection request is received from the other end with
* an MSS option, the MSS is modified depending on the value of the received
* MSS and on the results of Path MTU Discovery (which is enabled by default). 
* The MSS may be set as high as the outgoing interface MTU (1460 for an 
* Ethernet). Therefore, after a call to `socket' but before a connection is 
* established, an application can only decrease the MSS from its default of 512. 
* After a connection is established, the application can decrease the MSS from 
* whatever value was selected.
*
* .SS "SO_DEBUG -- Debugging the  underlying protocol"
* Specify the SO_DEBUG option to let the underlying protocol module record
* debug information.
* .CS
*     setsockopt (sock, SOL_SOCKET, SO_DEBUG, &optval, sizeof (optval));
* .CE
* The value at <optval> for this option is an integer (type `int'),
* either 1 (on) or 0 (off).
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
* OPTIONS FOR DATAGRAM AND RAW SOCKETS
* The following section discusses options for multicasting on UDP and RAW
* sockets.
*
* .SS "IP_ADD_MEMBERSHIP -- Join a Multicast Group"
* Specify the IP_ADD_MEMBERSHIP option when a process needs to join
* multicast group:
* .CS
*     setsockopt (sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&ipMreq,
*                 sizeof (ipMreq));
* .CE
* The value of <ipMreq> is an 'ip_mreq' structure. 
* `ipMreq.imr_multiaddr.s_addr' is the internet multicast address
* `ipMreq.imr_interface.s_addr' is the internet unicast address of the 
* interface through which the multicast packet needs to pass.
*
* .SS "IP_DROP_MEMBERSHIP -- Leave a Multicast Group"
* Specify the IP_DROP_MEMBERSHIP option when a process needs to leave
* a previously joined multicast group:
* .CS
*     setsockopt (sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *)&ipMreq,
*                 sizeof (ipMreq));
* .CE
* The value of <ipMreq> is an 'ip_mreq' structure.  
* `ipMreq.imr_multiaddr.s_addr' is the internet multicast address.
* `ipMreq.imr_interface.s_addr' is the internet unicast address of the 
* interface to which the multicast address was bound.
*
* .SS "IP_MULTICAST_IF -- Select a Default Interface for Outgoing Multicasts"
* Specify the IP_MULTICAST_IF option when an application needs to specify
* an outgoing network interface through which all multicast packets
* are sent:
* .CS
*     setsockopt (sock, IPPROTO_IP, IP_MULTICAST_IF, (char *)&ifAddr,
*                 sizeof (mCastAddr));
* .CE
* The value of <ifAddr> is an 'in_addr' structure.  
* `ifAddr.s_addr' is the internet network interface address.
*
* .SS "IP_MULTICAST_TTL -- Select a Default TTL"
* Specify the IP_MULTICAST_TTL option when an application needs to select
* a default TTL (time to live) for outgoing multicast
* packets:
*
* .CS
*     setsockopt (sock, IPPROTO_IP, IP_MULTICAST_TTL, &optval, sizeof(optval));
* .CE
* The value at <optval> is an integer (type <int>), time to live value.
*
* .TS
* tab(|);
* lf3 lf3 lf3
* l l l.
* optval(TTL)  |  Application                   | Scope
* _
* 0            |                                | same interface
* 1            |                                | same subnet
* 31           | local event video              | 
* 32           |                                | same site
* 63           | local event audio              | 
* 64           |                                | same region
* 95           | IETF channel 2 video           | 
* 127          | IETF channel 1 video           | 
* 128          |                                | same continent
* 159          | IETF channel 2 audio           | 
* 191          | IETF channel 1 audio           | 
* 223          | IETF channel 2 low-rate audio  | 
* 255          | IETF channel 1 low-rate audio  |
*              | unrestricted in scope          |
* .TE
*
* .SS "IP_MULTICAST_LOOP -- Enable or Disable Loopback"
* Enable or disable loopback of outgoing multicasts.
* .CS
*     setsockopt (sock, IPPROTO_IP, IP_MULTICAST_LOOP, &optval, sizeof(optval));
* .CE
* The value at <optval> is an integer (type <int>), either 1(on) or 0
* (off).
*
* OPTIONS FOR DATAGRAM, STREAM AND RAW SOCKETS
* The following section discusses options for RAW, DGRAM or STREAM
* sockets.
*
* .SS "IP_OPTIONS -- set options to be included in outgoing datagrams"
* Sets the IP options sent from this socket with every packet.
* .CS
*     setsockopt (sock, IPPROTO_IP, IP_OPTIONS, optbuf, optbuflen);
* .CE
* Here <optbuf> is a buffer containing the options.
*
* .SS "IP_TOS-- set options to be included in outgoing datagrams"
* Sets the Type-Of-Service field for each packet sent from this socket.
* .CS
*     setsockopt (sock, IPPROTO_IP, IP_TOS, &optval, sizeof(optval));
* .CE
* Here <optval> is an integer (type <int>).  This integer can be set to
* IPTOS_LOWDELAY, IPTOS_THROUGHPUT, IPTOS_RELIABILITY, or IPTOS_MINCOST,
* to indicate how the packets sent on this socket should be prioritized.
*
* .SS "IP_TTL-- set the time-to-live field in outgoing datagrams"
* Sets the Time-To-Live field for each packet sent from this socket.
* .CS
*     setsockopt (sock, IPPROTO_IP, IP_TTL, &optval, sizeof(optval));
* .CE
* Here <optval> is an integer (type <int>), indicating the number of hops
* a packet can take before it is discarded.
*
* .SS "IP_RECVRETOPTS -- [un-]set queueing of reversed source route"
* Sets whether or not reversed source route queueing will be enabled for
* incoming datagrams. (Not implemented)
* .CS
*     setsockopt (sock, IPPROTO_IP, IP_RECVRETOPTS, &optval, sizeof(optval));
* .CE
* Here <optval> is a boolean (type <int>).  However, this option is
* currently not implemented, so setting it will not change the behavior
* of the system.
*
* .SS "IP_RECVDSTADDR -- [un-]set queueing of IP destination address"
* Sets whether or not the socket will receive the IP address of the
* destination of an incoming datagram in control data.
* .CS
*     setsockopt (sock, IPPROTO_IP, IP_RECVDSTADDR, &optval, sizeof(optval));
* .CE
* Here <optval> is a boolean (type <int>).
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
* When the SO_REUSEADDR option is turned on, applications may bind a stream 
* socket to a local port. This is possible even if the port is still bound 
* to another stream socket.  It is even  possible if that other socket is 
* associated with a "zombie" protocol control block context that has not yet 
* freed from previous sessions.  The uniqueness of port number combinations 
* for each connection is still preserved through sanity checks performed at 
* actual connection setup time.  If this option is not turned on and an 
* application attempts to bind to a port that is being used by a zombie 
* protocol control block, the bind() call fails.
*
* .SS "SO_REUSEPORT -- Reusing a Socket address and port"
* This option is similar to the SO_REUSEADDR option but it allows binding to
* the same local address and port combination. 
* .CS
*     setsockopt (sock, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof (optval));
* .CE
* The value at <optval> is an integer (type <int>), either 1 (on) or 0 (off).
*
* The SO_REUSEPORT option is mainly required by multicast applications where
* a number of applications need to bind to the same multicast address and port
* to receive multicast data. Unlike SO_REUSEADDR where only the later
* applications need to set this option, with SO_REUSEPORT all applications
* including the first to bind to the port are required to set this option. For
* multicast addresses SO_REUSEADDR and SO_REUSEPORT show the same behavior so
* SO_REUSEADDR can be used instead. 
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
* The effect of setting the maximum size of buffers (for both SO_SNDBUF and 
* SO_RCVBUF, described below) is not actually to allocate the mbufs from 
* the mbuf pool.  Instead, the effect is to set the high-water mark in the 
* protocol data structure, which is used later to limit the amount of mbuf 
* allocation.  Thus, the maximum size specified for the socket level send 
* and receive buffers can affect the performance of bulk data transfers.  
* For example, the size of the TCP receive windows is limited by the 
* remaining socket-level buffer space.  These parameters must be adjusted 
* to produce the optimal result for a given application.
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
* TCP provides an expedited data service that does not conform to the
* normal constraints of sequencing and flow control of data
* streams. The expedited service delivers "out-of-band" (urgent) data
* ahead of other "normal" data to provide interrupt-like services (for
* example, when you hit a CTRL-C during telnet or rlogin session while
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
* length greater than MLEN, insufficient mbufs, or the call is unable to set
* the specified option.
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
    SOCK_FUNC *	pSockFunc = NULL;

    if (optval == NULL || iosFdValue (s) ==  ERROR)
        {
        netErrnoSet (EINVAL);
        return (ERROR);
        }

    pSockFunc = pSockFdMap[s];
    if ((pSockFunc == NULL) || (pSockFunc->setsockoptRtn == NULL))
	{
        netErrnoSet (ENOTSUP);
	return (ERROR);
	}

    return ((pSockFunc->setsockoptRtn) (s, level, optname,
	    optval, optlen));
    }

/*******************************************************************************
*
* getsockopt - get socket options
*
* This routine returns relevant option values associated with a socket.
* To manipulate options at the "socket" level, <level> should be SOL_SOCKET.
* Any other levels should use the appropriate protocol number.
* The <optlen> parameter should be initialized to indicate the amount of
* space referenced by <optval>.  On return, the value of the option is copied to
* <optval> and the actual size of the option is copied to <optlen>.
*
* Although <optval> is passed as a char *, the actual variable whose address 
* gets passed in should be an integer or a structure, depending on which 
* <optname> is being passed. Refer to setsockopt() to determine the correct 
* type of the actual variable (whose address should then be cast to a char *).
*
* RETURNS:
* OK, or ERROR if there is an invalid socket, an unknown option, or the call
* is unable to get the specified option.
*
* EXAMPLE
* Because SO_REUSEADDR has an integer parameter, the variable to be
* passed to getsockopt() should be declared as
* .CS
*    int reuseVal;
* .CE
* and passed in as 
* .CS
*    (char *)&reuseVal.
* .CE
* Otherwise the user might mistakenly declare `reuseVal' as a character,
* in which case getsockopt() will only return the first byte of the
* integer representing the state of this option.  Then whether the return
* value is correct or always 0 depends on the endian-ness of the machine.
*
* SEE ALSO: setsockopt()
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
    SOCK_FUNC *	pSockFunc = NULL;

    if (optval == NULL || optlen == NULL || iosFdValue (s) ==  ERROR)
        {
        netErrnoSet (EINVAL);
        return (ERROR);
        }

    pSockFunc = pSockFdMap[s];
    if ((pSockFunc == NULL) || (pSockFunc->getsockoptRtn == NULL))
	{
        netErrnoSet (ENOTSUP);
	return (ERROR);
	}

    return ((pSockFunc->getsockoptRtn) (s, level, optname,
	optval, optlen));
    }

/*******************************************************************************
*
* getsockname - get a socket name
*
* This routine gets the current name for the specified socket <s>.
* The <namelen> parameter should be initialized to indicate the amount of
* space referenced by <name>.  On return, the name of the socket is copied to
* <name> and the actual size of the socket name is copied to <namelen>.
*
* RETURNS: OK, or ERROR if the socket is invalid.
*/

STATUS getsockname
    (
    int s,                      /* socket descriptor */
    struct sockaddr *name,      /* where to return name */
    int *namelen                /* space available in name, later */
                                /* filled in with actual name size */
    )
    {
    SOCK_FUNC *	pSockFunc = NULL;

    if (name == NULL || namelen == NULL || iosFdValue (s) ==  ERROR)
        {
        netErrnoSet (EINVAL);
        return (ERROR);
        }

    pSockFunc = pSockFdMap[s];
    if ((pSockFunc == NULL) || (pSockFunc->getsocknameRtn == NULL))
	{
        netErrnoSet (ENOTSUP);
	return (ERROR);
	}

    return ((pSockFunc->getsocknameRtn) (s, name, namelen));
    }

/*******************************************************************************
*
* getpeername - get the name of a connected peer
*
* This routine gets the name of the peer connected to socket <s>.
* The <namelen> parameter should be initialized to indicate the amount of
* space referenced by <name>.  On return, the name of the socket is copied to
* <name> and the actual size of the socket name is copied to <namelen>.
*
* RETURNS: OK, or ERROR if the socket is invalid
* or not connected.
*/

STATUS getpeername
    (
    int s,                      /* socket descriptor */
    struct sockaddr *name,      /* where to put name */
    int *namelen                /* space available in name, later */
                                /* filled in with actual name size */
    )
    {
    SOCK_FUNC *	pSockFunc = NULL;

    if (name == NULL || namelen == NULL || iosFdValue (s) ==  ERROR)
        {
        netErrnoSet (EINVAL);
        return (ERROR);
        }

    pSockFunc = pSockFdMap[s];
    if ((pSockFunc == NULL) || (pSockFunc->getpeernameRtn == NULL))
	{
        netErrnoSet (ENOTSUP);
	return (ERROR);
	}

    return ((pSockFunc->getpeernameRtn) (s, name, namelen));
    }

/******************************************************************************
*
* shutdown - shut down a network connection
*
* This routine shuts down all, or part, of a connection-based socket <s>.
* If the value of <how> is 0, receives are disallowed.  If <how> is 1,
* sends are disallowed.  If <how> is 2, both sends and receives are
* disallowed.
*
* RETURNS: ERROR if the socket is invalid or has no registered
* socket-specific routines; otherwise shutdown() returns the return value
* from the socket-specific shutdown routine (typically OK in the case of
* a successful shutdown or ERROR otherwise).
*/

STATUS shutdown
    (
    int s,      /* socket to shut down */
    int how     /* 0 = receives disallowed */
                /* 1 = sends disallowed */
                /* 2 = sends and receives disallowed */
    )
    {
    SOCK_FUNC *	pSockFunc = NULL;

    if (((UINT)how) > 2 || iosFdValue (s) ==  ERROR)
        {
        netErrnoSet (EINVAL);
        return (ERROR);
        }

    pSockFunc = pSockFdMap[s];

    if ((pSockFunc == NULL) || (pSockFunc->shutdownRtn == NULL))
	{
        netErrnoSet (ENOTSUP);
	return (ERROR);
	}

    return ((pSockFunc->shutdownRtn) (s, how));
    }

SOCK_FUNC * sockFdtosockFunc
     (
     int s
     )
     {
     if (iosFdValue (s) ==  ERROR)
        return (NULL);

     return pSockFdMap[s];
     }

/* distIfUdp.c - distributed objects UDP adapter library (VxFusion option) */

/* Copyright 1999-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01l,22oct01,jws  fix diab compiler warnings (SPR 71117)
01k,11jun99,drm  Changing flags to host byte order before checking for
                 broadcast flag.
01j,24may99,drm  Adding vxfusion prefix to VxFusion related includes.
01i,19may99,drm  Changing WindMP to VxFusion.
01h,25feb99,drm  added additional status output during initialization
01g,22feb99,drm  adding #include for distNetLib.h
01f,30oct98,drm  documentation updates
01e,29oct98,drm  removed maxTBufs argument to distIfUdpInit
01d,01sep98,drm  removed pDistIf definition
01c,04aug98,drm  cleanup and bug fixes
01b,20may98,drm  removed compiler warning messages
01a,04apr98,jag  written.
*/

/*
DESCRIPTION
This module implements a UDP adapter for VxFusion.  This is the default 
adapter that VxFusion uses when it starts up.

There are only two external routines: distIfUdpInit() and distIfUdpStart().
Both are called by the VxFusion initialization routine distInit() and should 
not be called directly by the user.

AVAILABILITY
This module is distributed as a component of the unbundled distributed
message queues option, VxFusion.

INCLUDE FILES: distIfUdp.h

SEE ALSO: distLib
*/


/* includes */

#include "vxWorks.h"
#include "string.h"
#include "stdio.h"
#include "taskLib.h"
#include "sysLib.h"
#include "inetLib.h"
#include "sockLib.h"
#include "netLib.h"
#include "ioLib.h"
#include "wdLib.h"
#include "usrLib.h"
#include "sys/ioctl.h"
#include "netinet/in.h"
#include "net/if.h"
#include "selectLib.h"
#include "errnoLib.h"
#include "vxfusion/distIfLib.h"
#include "vxfusion/distStatLib.h"
#include "vxfusion/distNetLib.h"
#include "drv/vxfusion/distIfUdp.h"


/* defines */

#define UNUSED_ARG(x)  if (sizeof(x)) {};  /* suppress compiler warning */

#define SOCKOPT_ON  1                   /* 1 = on for setsockopt() */

#define sleep(timeO) taskDelay (timeO * (sysClkRateGet())) /* macro */


/* locals */

LOCAL DIST_IF distIfUdp;                /* used to set pDistIf */

LOCAL BOOL distIfUdpInstalled = FALSE;  /* indicates whether driver installed */
LOCAL BOOL distIfUdpStarted   = FALSE;  /* indicates whether driver started */

LOCAL int ioSocket = 0;                 /* socket used to send/rcv messages */

LOCAL unsigned long  myIpAddr      = 0; /* used to uniquely identify node */
LOCAL unsigned long  myIpBcastAddr = 0; /* address used to broadcast packets */


/* forward declarations */

STATUS distIfUdpStart (void *pConf);                 /* startup function */
LOCAL STATUS distIfUdpSend (DIST_NODE_ID nodeIdDest, 
                            DIST_TBUF *pTBuf,
                            int priority);           /* send function */
LOCAL int distIfUdpIoctl (int func, ...);            /* ioctl function */
LOCAL void distIfUdpInputTask (void);                /* input function */


/***************************************************************************
*
* distIfUdpInit - initialize the UDP driver (VxFusion option)
*
* Initialize the UDP driver for VxFusion.
*
* \is
* \i This routine initializes the UDP driver by doing the following:
* 
* 
* Temporarily opening a socket in order to determine the IP address, netmask, 
* and broadcast address for that interface.
*
* Filling in the DIST_IF structure which provides the upper layers with some 
* necessary information about the adapter and transport 
* \ie
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
* 
* RETURNS: OK, or ERROR if the operation fails 
*/

STATUS distIfUdpInit
    (
    void *    pConf,        /* not currently used */
    FUNCPTR * pStartup      /* Ptr to startup routine */
    )
    {
    int bsocket;           /* socket used to obtain broadcast address */
    int retVal;            /* variable used to check return value */
    struct ifreq ifr;      /* interface request struct for socket ioctls */
    struct in_addr ifAddr; /* ip address for the interface */
    struct in_addr ifMask; /* netmask for the interface*/

    *pStartup = (FUNCPTR) (distIfUdpStart);

    if (distIfUdpInstalled)
        {
#ifdef DIST_DIAGNOSTIC
        printf ("distIfUdpInit() - already initalized.\n");
#endif
        return (OK);
        }

    /*
     * Temporarily open a socket in order to determine the node IP address, 
     * netmask, and broadcast address.
     */

    bsocket = socket (AF_INET, SOCK_RAW, 0);
    if (bsocket == -1)
        {
        printf ("distIfUdpInit() - can't open RAW socket (errno = %d)\n",
                errno);
        printf ("Unable to initialize VxFusion UDP adapter.\n");
        return (ERROR);
        }

    bzero ((char *)&ifr, sizeof (struct ifreq));

    if ((strlen (pConf) + 1) > IFNAMSIZ)
        {
        printf ("distIfUdpInit() - interface name too long");
        printf ("Unable to initialize VxFusion UDP adapter.\n");
        close (bsocket);
        return (ERROR);
        }

    strcpy (ifr.ifr_name, (char *) pConf);

    /* Get interface IP address */

    retVal = ioctl (bsocket, (int)SIOCGIFADDR, (int)&ifr);
    if (retVal != 0)
        {
        printf ("distIfUdpInit() - ioctl SIOCGIFADDR failed (errno = %d)\n",
                errno);
        printf ("Unable to initialize VxFusion UDP adapter.\n");
        close (bsocket);
        return (ERROR);
        }

    ifAddr.s_addr = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;
    myIpAddr = ntohl (ifAddr.s_addr);

    /* Get interface IP Netmask */

    retVal = ioctl (bsocket, (int)SIOCGIFNETMASK, (int)&ifr);
    if (retVal != 0)
        {
        printf ("distIfUdpInit() - ioctl SIOCGIFNETMASK failed (errno=%d)\n",
                errno);
        printf ("Unable to initialize VxFusion UDP adapter.\n");
        close (bsocket);
        return (ERROR);
        }

    ifMask.s_addr = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;

    close (bsocket);

    /*
     * Extract network address and fill host portion with ones
     * in order to create broadcast address.
     */

    myIpBcastAddr = (ifAddr.s_addr & ifMask.s_addr) | ~ifMask.s_addr;
    myIpBcastAddr = ntohl (myIpBcastAddr);  

    /* Fill in the distIfUdp structure */

    distIfUdp.distIfName          = "UDP adapter";
    distIfUdp.distIfMTU           = UDP_MTU_BUF_SZ;
    distIfUdp.distIfHdrSize       = sizeof (NET_HDR);
    distIfUdp.distIfBroadcastAddr = myIpBcastAddr;
    distIfUdp.distIfRngBufSz      = UDP_RING_BUF_SZ;
    distIfUdp.distIfMaxFrags      = UDP_MAX_FRAGS;  
    distIfUdp.distIfSend          = distIfUdpSend;
    distIfUdp.distIfIoctl         = distIfUdpIoctl;

    pDistIf = &distIfUdp;

    distIfUdpInstalled = TRUE;

    printf ("VxFusion UDP adapter initialized OK\n");

    return (OK);
    }


/***************************************************************************
*
* distIfUdpStart - start the UDP adapter (VxFusion option)
*
* This routine creates a socket that is used for communications.  After
* enabling the broadcast option on the socket, it spawns the input task.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: OK, or ERROR if unable to perform the socket operations or 
* spawn the input task
*/

STATUS distIfUdpStart
    (
    void * pConf      /* ptr to configuration data - not used here */
    )
    {
    int optval;                       /* options value for setsockopt() */
    struct sockaddr_in addrToListen;  /* socket info needed for bind() call */
    
    UNUSED_ARG(pConf);    

    if (distIfUdpStarted)
        {
#ifdef DIST_DIAGNOSTIC
        printf ("distIfUdpStart() - already started.\n");
#endif
        return (OK);
        }


    /* Open socket */

    if ((ioSocket = socket (AF_INET, SOCK_DGRAM, 0)) < 0)
        {
        printf ("distIfUdpStart() - ioSocket open failed (errno = %d)\n",
                errno);
        printf ("Unable to start VxFusion UDP adapter.\n");
        return (ERROR);
        }

    /* Enable broadcast option for socket. */

    optval = SOCKOPT_ON;
    if ((setsockopt (ioSocket, SOL_SOCKET, SO_BROADCAST, (char *)&optval,
                     sizeof (optval))) < 0)
        {
        printf ("distIfUdpStart() - setsockopt SO_BROADCAST failed ");
        printf ("(errno = %d)\n", errno);
        printf ("Unable to start VxFusion UDP adapter.\n");
        close (ioSocket);
        return (ERROR);
        }

    /* Initialize source address. */

    bzero ((char *)&addrToListen, sizeof (addrToListen));
    addrToListen.sin_addr.s_addr = INADDR_ANY;
    addrToListen.sin_family      = AF_INET;
    addrToListen.sin_port        = htons (UDP_IO_PORT);

    /* Bind to the socket */
      
    if ((bind (ioSocket, (struct sockaddr *)&addrToListen, 
               sizeof (addrToListen))) < 0)
        {
        printf ("distIfUdpStart() - ioSocket bind failed (errno = %d)\n",
                errno);
        printf ("Unable to start VxFusion UDP adapter.\n");
        close (ioSocket);
        return (ERROR);
        }

    /* Spawn Input task */

    if ((taskSpawn ("tDiUdpNet", 50, VX_SUPERVISOR_MODE,
                    5000, (FUNCPTR) distIfUdpInputTask,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0)) == ERROR)
        {
        printf ("distIfUdpStart() - can't spawn distIfUdpInputTask task ");
        printf ("(errno = %d)\n", errno);
        printf("Unable to start VxFusion UDP adapter.\n");
        close (ioSocket);
        return (ERROR); /* taskSpawn() failed */    
        }

    distIfUdpStarted = TRUE;

    printf ("VxFusion UDP adapter started OK\n");

    return (OK);
    }


/***************************************************************************
*
* distIfUdpInputTask - receives a broadcast and node to node messages (VxFusion option)
*
* The purpose of this routine is to receive a message from the network and
* send it on up to the upper layers using distNetInput().
*
* This task does a select on a file descripter set containing the socket
* file descripter.  When a message is received, it will be checked to see if 
* the source of the message was this node.  If this node was the source, then
* the message is discarded because the upper layers do not expect to receive
* broadcast messages from itself.  NOTE: This is probably unnecessary for
* in most cases as the Ethernet driver should already filter out such 
* messages.
*
* If the message appears to be a valid message, a TBuf is allocated and the
* message is copied into it.  The header fields of the TBuf are reconstructed
* from the adapter specific header.  Finally, the message length is checked 
* with the expected message length before sending the packet upward using
* distNetInput().
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: N/A
*
* NOMANUAL
*/

LOCAL void distIfUdpInputTask (void)
    {
    char               inputMessage [UDP_MTU_BUF_SZ];  /* input buffer */
    struct sockaddr_in srcAddr;      /* sender of the message */
    fd_set             readFds;      /* set containing bcast & unicast sockets*/
    int                msgLen;       /* length of msg received from socket */
    int                srcAddrLen;   /* length of the sender's address */
    DIST_NODE_ID       sourceNodeId; /* node ID of the sender */
    NET_HDR *          pUdpHdr;      /* ptr to msg's adapter specific header */
    DIST_TBUF *        pTBuf;        /* ptr to buffer msg will be copied into */

    /* Initialize source address. */

    bzero ((char *)&srcAddr, sizeof (srcAddr));
    srcAddr.sin_addr.s_addr = INADDR_ANY;
    srcAddr.sin_family =      AF_INET;

    srcAddrLen = sizeof (srcAddr);

    FOREVER 
        {
        FD_ZERO (&readFds);
        FD_SET (ioSocket, &readFds);

        if ((select (FD_SETSIZE, &readFds, NULL, NULL, NULL)) < 0)
            {
            close (ioSocket);
#ifdef DIST_DIAGNOSTIC
            printf ("distIfUdpInputTask() - select returned an error\n");
#endif
            return;
            }

        distStat.ifInReceived++;

        if ((msgLen = recvfrom (ioSocket, (caddr_t) &inputMessage, 
                                sizeof (inputMessage), 0, (struct sockaddr *) 
                                &srcAddr, &srcAddrLen)) < 0)
            {
#ifdef DIST_DIAGNOSTIC
            printf ("distIfUdpInputTask() - got an error from recvfrom\n");
#endif
            distStat.ifInDiscarded++;
            continue; /* Skip for now */
            }

        /* Check for buffer too small */

        if (msgLen < sizeof (NET_HDR))
            {
#ifdef DIST_DIAGNOSTIC
            printf ("distIfUdpInputTask() - got packet which is too small\n");
#endif
            distStat.ifInLength++;
            distStat.ifInDiscarded++;
            continue;
            }

        /* Get node ID */

        sourceNodeId = ntohl (srcAddr.sin_addr.s_addr);

        /* 
         * If this packet is a broadcast that was sent from this node, we
         * may get a copy of the packet if it isn't discarded by the 
         * ethernet driver.  If it isn't discarded by the ethernet driver, 
         * we need to discard broadcast packets sent from ourself.
         */

        pUdpHdr = (NET_HDR *) inputMessage;
        if ( ( ntohs (pUdpHdr->pktFlags) & DIST_TBUF_FLAG_BROADCAST) && 
            (myIpAddr == sourceNodeId))
            {
#ifdef DIST_DIAGNOSTIC
            printf ("distIfUdpInputTask() - discarded loop broadcast\n");
#endif
            distStat.ifInDiscarded++;
            continue;
            }

        /* Allocate Input buffer */

        if ((pTBuf = distTBufAlloc ()) == NULL)
            {
#ifdef DIST_DIAGNOSTIC
            printf ("distIfUdpInputTask() - failed to allocate a buffer\n");
#endif
            distStat.ifInDiscarded++;
            continue;
            }

        bcopy (inputMessage,
               ((char *)pTBuf->pTBufData - sizeof (NET_HDR)),
               msgLen);

        pUdpHdr = (NET_HDR *) ((char *)pTBuf->pTBufData - sizeof (NET_HDR));

        pTBuf->tBufId     = ntohs (pUdpHdr->pktId);
        pTBuf->tBufAck    = ntohs (pUdpHdr->pktAck);
        pTBuf->tBufSeq    = ntohs (pUdpHdr->pktFragSeq);
        pTBuf->tBufNBytes = ntohs (pUdpHdr->pktLen);
        pTBuf->tBufType   = ntohs (pUdpHdr->pktType);
        pTBuf->tBufFlags  = ntohs (pUdpHdr->pktFlags);

        /*
         * Check for the right packet length.  The packet length
         * should be equivalent to the sum of the number of bytes
         * in the data portion and the size of the adapter specific
         * header (NET_HDR for UDP). 
         */

        if ((pTBuf->tBufNBytes + sizeof (NET_HDR)) != msgLen)
            {
#ifdef DIST_DIAGNOSTIC
            printf ("distIfUdpInputTask() - invalid packet length\n");
#endif
            distTBufFree (pTBuf);
            distStat.ifInLength++;
            distStat.ifInDiscarded++;
            continue;
            }

#ifdef DIST_DIAGNOSTIC
        printf ("distIfUdpInputTask() - got a packet (type %d) from 0x%lx\n",
               pTBuf->tBufType, sourceNodeId);
#endif

        /* Send the packet up the stack */

        distNetInput (sourceNodeId, ntohs (pUdpHdr->priority), pTBuf);
        } /* end FOREVER */

#if 0   /* someday, we might actually break out of FOREVER */

    close (ioSocket); 

    return;

#endif
    }


/***************************************************************************
*
* distIfUdpSend - convert header to network byte order and send packet out (VxFusion option)
*
* This routine fills in the adapter specific header using the information
* in the TBuf and then sends the packet out.
*
* The calling routine will free the TBuf passed to distIfUdpSend().  While
* this implies that the TBuf does not need to be freed here, it also 
* implies that this routine must finish using the TBuf before it returns.
* 
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: OK, or ERROR if the operation fails 
*
* NOMANUAL
*/

LOCAL STATUS distIfUdpSend
    (
    DIST_NODE_ID    nodeIdDest,     /* destination node */
    DIST_TBUF *     pTBuf,          /* TBuf to send */
    int             priority        /* packet priority */
    )
    {
    NET_HDR *           pUdpHdr;
    struct  sockaddr_in dstAddr;

    distStat.ifOutReceived++;

    /* Access pre-allocated UDP NET_HDR header from data buffer */

    pUdpHdr = (NET_HDR *) ((char *)pTBuf->pTBufData - sizeof (NET_HDR));

    pUdpHdr->pktId = htons (pTBuf->tBufId);       /* packet ID */
    pUdpHdr->pktAck = htons (pTBuf->tBufAck);     /* last packet ID acked */
    pUdpHdr->pktFragSeq = htons (pTBuf->tBufSeq); /* fragmented seq number */
    pUdpHdr->pktLen = htons (pTBuf->tBufNBytes);  /* packet length */
    pUdpHdr->pktType = htons (pTBuf->tBufType);   /* pkt type(DATA,ACK,...)*/
    pUdpHdr->pktFlags = htons (pTBuf->tBufFlags); /* flgs: HDR,MORE_MF,BCAST*/
    pUdpHdr->priority = htons ((short)priority);  /* packet priority */

    dstAddr.sin_family = AF_INET;
    dstAddr.sin_addr.s_addr = htonl (nodeIdDest);
    dstAddr.sin_port   = htons (UDP_IO_PORT);

#ifdef DIST_DIAGNOSTIC
    printf ("distIfUdpSend() - sending a packet (type %d) to =0x%lx\n",
            pTBuf->tBufType, nodeIdDest);
#endif

    /* Send the packet out */

    if ((sendto (ioSocket, (caddr_t) pUdpHdr, 
                 (pTBuf->tBufNBytes + sizeof (NET_HDR)), 0, 
                 (struct sockaddr *) &dstAddr, sizeof (dstAddr))) < 0)
        {
#ifdef DIST_DIAGNOSTIC
        printf ("distIfUdpSend() - sendto() failed\n");
#endif
        return (ERROR);
        }

    return (OK);
    }


/***************************************************************************
*
* distIfUdpIoctl - I/O control function for the adapter (VxFusion option)
*
* This is the I/O control function for the UDP adapter.  Since the UDP
* adapter does not currently implement any control functions, this function
* will simply return ERROR in response to any call.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: ERROR only
*
* NOMANUAL
*/

LOCAL int distIfUdpIoctl
    (
    int func,   /* control function to perform */
    ...         /* optional arguments */
    )
    {
       
    UNUSED_ARG(func);
    
    return (ERROR);
    }


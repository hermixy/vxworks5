/* distIfUdp.h - UDP adapter initialization routine (VxFusion) */

/* Copyright 1999-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01d,16oct01,jws  ARM support
01c,11jun99,drm  Changing default ring buffer size to 256.
01b,29oct98,drm  removed maxTBufs argument from distIfUdpInit()
01a,31jul98,drm  initial version
*/

#ifndef __INCdistIfUdph
#define __INCdistIfUdph

#include "vxWorks.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * The NET_HDR structure encapsulates the fields needed by the Network Layer
 * (L3) of the VxFusion protocol stack.  These fields are encapsulated at a 
 * header at this level to allow for the flexibility of different types of 
 * interfaces.  Interfaces with small MTU sizes may wish to use smaller headers 
 * to increase throughput, while interfacs with larger MTU sizes may wish to 
 * use larger headers to allow larger messages to be sent.  The values that 
 * need to be set for the  fields of the structure below are passed to/from 
 * the Network Layer within the TBuf structure.
 *
 * The NET_HDR structure may also be used to store values such as priority
 * which may be lost if the transport doesn't support message priorities.
 */

/* defines */

/* get rid of this next when underscore issue is settled */

#ifndef _WRS_PACK_ALIGN
# define _WRS_PACK_ALIGN(m)  WRS_PACK_ALIGN(m)
#endif


#define UDP_IO_PORT         5011    /* UDP port for node to node comm */
#define UDP_MTU_BUF_SZ      1500    /* MTU size to use for packets */
#define UDP_RING_BUF_SZ      256    /* Window size used by network protocol */
#define UDP_MAX_FRAGS         10    /* Max # fragments msg can be broken into */

/* typedefs */

typedef struct
    {
    uint16_t pktId;           /* Packet ID */
    uint16_t pktAck;          /* Last Packet ID Acked */
    uint16_t pktFragSeq;      /* Fragmented packet sequence number */
    uint16_t pktLen;          /* Packet Length */
    uint16_t pktType;         /* Packet type (DATA, ACK,...) */
    uint16_t pktFlags;        /* Packet flags HDR,MORE_MF, and/or BROADCAST */
    uint16_t priority;        /* Priority */
    } _WRS_PACK_ALIGN(2) NET_HDR;  /* Network Header */


/* function declarations */

#if defined(__STDC__) || defined(__cplusplus)

STATUS distIfUdpInit
    (
    void    *pConf,      /* ptr to configuration struct */
    FUNCPTR *pStartup    /* Ptr to startup routine */
    );

#else   /* __STDC__ */

STATUS distIfUdpInit ();

#endif  /* __STDC__ */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __INCdistIfUdph */




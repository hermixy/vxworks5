/* distPktLibP.h - */

#ifndef __INCdistPktLibPh
#define __INCdistPktLibPh

#include "vxWorks.h"

#define DIST_PKT_TYPE_MSG_Q			DIST_ID_MSG_Q_SERV
#define DIST_PKT_TYPE_MSG_Q_GRP		DIST_ID_MSG_Q_GRP_SERV
#define DIST_PKT_TYPE_DNDB			DIST_ID_DNDB_SERV
#define DIST_PKT_TYPE_DGDB			DIST_ID_DGDB_SERV
#define DIST_PKT_TYPE_INCO			DIST_ID_INCO_SERV
#define DIST_PKT_TYPE_GAP			DIST_ID_GAP_SERV

/* subtypes of BOOTSTRAP telegram */
#define DIST_BOOTING_REQ			0

/*
 * The __DIST_PKT_HDR_END__ makro must end a structure representing
 * a packet header. The DIST_PKT_HDR_SIZEOF() makro looks for it!
 * This is because several ABIs require a structure to be rouded up
 * in length. Therefore sizeof() is useless here. We want the true
 * size so that we do not waste network bandwidth.
 */

#define __DIST_PKT_HDR_END__ \
	char __distPktHdrEnd__[0];	/* should evaluate to nothing */
#define DIST_PKT_HDR_SIZEOF(struct) \
	((int) &((struct *) NULL)->__distPktHdrEnd__)

/*
 * General node to build structures for sending on the network:
 *
 * Be sure that an element of type <t> is stored at an address that
 * is at least aligned to sizeof (<t>). This ensures, that there is
 * no space wasted in between and different compilers for different
 * architectures do not have different understandings of the structure.
 *
 * Put __DIST_PKT_HDR_END__ as the last element of a packet header
 * structure. Note: Each network header structure starts of with DIST_PKT.
 * Therefore DIST_PKT does not need __DIST_PKT_HDR_END__.
 */

typedef struct								/* DIST_PKT */
	{
	uint8_t				pktType;
	uint8_t				pktSubType;
	uint16_t			pktLen;
	} DIST_PKT;

typedef struct								/* DIST_PKT_BOOT */
	{
	uint8_t				pktBootType;
	} DIST_PKT_BOOT;

#endif	/* __INCdistPktLibPh */


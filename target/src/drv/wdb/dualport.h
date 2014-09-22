/*
**  File:       dualport.h
**  Description: Contains definitions and data structure defs. 
**               for NetROM dualport protocols
**
**      Copyright (c) 1996 Applied Microsystems Corp.
**                          All Rights Reserved
**
** Redistribution and use in source and binary forms are permitted 
** provided that the above copyright notice and this paragraph are 
** duplicated in all such forms and that any documentation,
** advertising materials, and other materials related to such
** distribution and use acknowledge that the software was developed 
** by Applied Microsystems Corp. (the Company).  The name of the 
** Company may not be used to endorse or promote products derived
** from this software without specific prior written permission. 
** THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR 
** IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
** WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE. 
**
**  Modification History:
   2/17/94 fdv add two out of band characters to readaddr protocol:
                RA_RESET and RA_RESYNC.
                RA_RESET lets the target trigger a tgtreset.
                RA_RESYNC lets the target trigger the netrom initialize
                    the dualport.
   4/15/94 fdv add one more out of band character to readaddr protocol:
		RA_MISC adds misc functions. The first being 
			receive interrupt acknowledge.
   6/17/94 fdv add yet another oob char
		EMOFFONWRITE  misc subcommand - turn emulation off on ra_write
   11/16/94 sch Add stuff for Virtual Ethernet
*/

#ifndef _dualport_h
#define	_dualport_h

/* general macros */
#define	READADDR_SIZE		256	/* # bytes in ra message area */
#define	DUALPORT_SIZE		2048	/* # byte in dualport ram */
#define	DP_DATA_SIZE		60	/* msg data size */
#define	DP_MAXCHANNELS		1	/* max number of channels */
#define DPF_SERIAL		0	/* use serial port, not dp ram */
#define DPF_READONLY_TGT	1	/* target can't write dp ram */
#define	DPF_ONECHANNEL		2	/* only one channel */
#define	DPACK_BASE		4	/* offset of ack bytes */
#define	DPACTIVE_BASE		8	/* offset of active bytes */

/* Number of message buffers assigned for receive when target has read 
 * and write access.  */
#define RW_REC_MSGS 15

/* Number of message buffers assigned for transmit when target has read 
 * and write access.  */
#define RW_TX_MSGS 16

/* special characters for the read-address protocol; up to 8 can be defined */
#define	RA_MAX_INDEX		0x08	/* 8 characters can be defined */
#define	RA_PACK_INDEX		0x00	/* ack for transmit */
#define	RA_ESC_INDEX		0x01	/* add 0xF0 to the next character */
#define	RA_SET_INDEX		0x02	/* start of rom write request pkt */
#define	RA_STARTMSG_INDEX	0x03	/* start of a packet */
#define	RA_ENDMSG_INDEX		0x04	/* end of a packet */
#define	RA_RESET		0x05	/* tgtreset sequence */
#define	RA_RESYNC		0x06	/* tgt dualport init sequence */
#define	RA_MISC			0x07	/* tgt misc command sequence */
#define RX_INTR_ACK		0x01	/* misc subcommand- receive interrupt ACK */
#define EMOFFONWRITE		0x02	/* misc subcommand- turn emulation off on ra_write*/
#define READADDR_CTLCHAR(cp, ch)  (((cp)->oobthresh + (ch)) * (cp)->numaccess)
#define	READADDR_DATACHAR(cp, ch) ((ch) * (cp)->numaccess)

/* offsets of protocol stuff outside readaddr memory */
#define	RA_RI		0x100		/* netrom-ready byte */
#define	RA_ACK		0x101		/* character-ack byte */

/* start of read-address messages */
#define	RA_MSGBASE	0x140		/* start of read-addr structures */

/* read-write protocol addresses */
#define	RW_MRI		0		/* message-ready byte */
#define	RW_TX		8		/* transmit channel active */
#define	RW_RX		9		/* receive channel active */
#define	RW_MSGBASE	0x40		/* start of readwrite messages */

/* message field offsets from the start of the message */
#define	DPM_FLAGS	0		/* offset of flags field */
#define	DPM_SIZE	2		/* offset of size field */
#define	DPM_DATA	4		/* offset of data field */
#define	DPM_DATASIZE	DP_DATA_SIZE	/* max bytes of data */
#define	DPM_MSGSIZE	(DP_DATA_SIZE + 4)	/* size of an entire message */

/* return status codes from the getmsg() routines */
#define	GM_NODATA	(-1)		/* no data present */
#define	GM_MSGCOMPLETE	0		/* message end read */
#define	GM_NOTDONE	1		/* data present, but not all */
#define	GM_MSGOVERFLOW	2		/* data didn't fit in buffer */

/* 
 * Messages in dual-port ram have the following format:
 *
 * typedef volatile struct _dpMsgStruct {
 *     uInt16	flags;
 *     uInt16	size;
 *     uChar	data[DP_DATA_SIZE];
 * } DpMsg;
 *
 * On multi-word targets, each byte of this structure must be read 
 * individually.  For example, on a target with a 16-bit word size, both
 * bytes of which are being emulated, the message will look like:
 *     Pod 0 byte		     Pod 1 byte
 *	Flags Hi			???
 *	Flags Lo			???
 *	Size Hi				???
 *	Size Lo				???
 *	Data 0				???
 *	...				...
 *	Data N				???
 */

/* size of a message structure */
#define	DP_MSGSTRUCT_SIZE	(4 + DP_DATA_SIZE)
#define	DP_FLAGINDEX		0
#define	DP_SIZEINDEX		2
#define	DP_DATAINDEX		4

/* structure used to manage character-at-a time i/o in a buffer */
typedef struct _bufIoStruct {
    uInt16	flags;			/* buffer flags */
    uInt16	bufsize;		/* size of data in the buffer */
    int		index;			/* i/o index */
    uChar	buf[DP_DATA_SIZE];	/* the buffer */
} BufIo;

#define	DPMSG_READY	0x8000		/* ready to be processed */
#define	DPMSG_START	0x0001		/* start of message buffer chain */
#define	DPMSG_END	0x0002		/* end of message buffer chain */
#define	DPMSG_WRAP	0x0004		/* end of message buffers */

/* Added for overflow buffers */
#define DPMSG_NR_DONE   0x0008          /* NetROM is done reading or
                                           writing pod memory */
#define DPMSG_TARG_RDY  0x0010          /* Target is running in RAM -
                                           NetROM can use pod memory */
#define DPMSG_1K_BIT    0x0400			/* 1K bit of msg length */

/* Overflow buffers are in pod memory just below dual port RAM.  There is
   one for each msg structure and it contains the data that won't fit
   in the msg structure (above 60 bytes). */

#define MAX_MSG_SIZE 1536  /* a full Ethernet packet */
#define MAX_OVF_MSG_SIZE (MAX_MSG_SIZE - DP_DATA_SIZE)

/* structure of a communication channel */
typedef struct _dpChannelStruct {
    int		chanflags;		/* flags */
    int		numaccess;		/* number of target accesses to rom */
    int		oobthresh;		/* out-of-band data threshold */
    int		width;			/* bytes in a rom word */
    int		index;			/* index of pod 0 in the word */
    uInt32 	tx;			/* transmit msg structures */
    uInt32 	txovf;			/* transmit overflow buffer */
    uInt32 	txlim;			/* oldest unacked tx msg structure */
    uInt32 	txbase;			/* base of transmit msg structures */
    uInt32 	txovfbase;		/* base of transmit overflow buffers */
    void      (*wait_nr_done_ptr)();    /* ptr to RAM routine */
    BufIo	txbuf;			/* transmit buffer structure */
    uInt32	rx;			/* receive msg structures */
    uInt32	rxovf;			/* receive overflow buffer */
    uInt32	rxlim;			/* next message location */
    uInt32	rxbase;			/* base of receive msg structures */
    uInt32	rxovfbase;		/* base of receive overflow buffers */
    BufIo	rxbuf;			/* receive buffer structure */
    uInt32	dpbase;			/* base of dualport ram */
    uChar	rxackval;		/* rx ack value to write in dp ram */
} DpChannel;

#define	CF_TXVALID	0x0001		/* transmit size valid */
#define	CF_RXVALID	0x0002		/* receive side valid */
#define	CF_NOWAITIO	0x0004		/* don't wait for buffers to be ready */

#endif	/* _dualport_h */


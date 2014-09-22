/*
**  File:     dualport.h
**  Version:  1.0.1
**
**  Description: dualport defines and structs common to NR-F/w & tgt
**
**      Copyright (c) 1996 Applied Microsystems Corp.
**                          All Rights Reserved
**
**
**  Modification History:
**        10/11/96...MPH...Modified NR4xx v1.3.1 dualport.h to NR5xx
**        01/20/97...MPH...Modified DpChannel struct to use Int16's 
*/
#ifndef _dualport_h
#define	_dualport_h

/* general macros */
#define	DUALPORT_SIZE		8192	/* # byte in dualport ram */
#define DP_CHAN_SIZE            2048    /* # bytes in one dualport chan */
#define DUALPORT_BASE           POD_0_ADDR
#define	DP_DATA_SIZE		60	/* msg data size */
#define	DP_MAXCHANNELS		1	/* max number of channels */
#define DPF_SERIAL              0       /* use serial port, not dp ram */
#define DPF_READONLY_TGT        1       /* target can't write dp ram */
#define DPF_ONECHANNEL          2       /* only one channel */
#define DPF_TWOCHANNEL          4       /* channel 2 */
#define DPF_THREECHANNEL        8       /* channel 3 */

/* Number of message buffers assigned for receive */
#define DP_REC_MSGS 15 

/* Number of message buffers assigned for transmit */
#define DP_TX_MSGS 16

/* Dualport protocol addresses */
#define	DP_MRI		0x00		/* message-ready byte */
#define	DP_TX		0x08		/* transmit channel active */
#define	DP_RX		0x09		/* receive channel active */
#define DP_NR_DONE      0x20            /* NR finished w/ PodMem write */
#define DP_OOBFLAG      0x21            /* Currently, only for resync */
#define	DP_MSGBASE	0x40		/* start of readwrite messages */
 
/* "Special" out-of-band flag, used for resync */
#define OOBFLAG_RESYNC  0x01            /* Write to DP_OOBFLAG to resync */

/* Note: vether protocol uses bytes 0x0A through 0x0F */

/* Read-read protocol on/off switches */
/* There are four versions of each RR const for speed--see dptartget.h   */
#define RR_ENABLE_ADR   0x10              /* RR protocol enable */
#define RR_DISABLE_ADR  0x18              /* RR protocol enable */

/* Message field offsets from the start of the message */
#define	DPM_FLAGS	0x00		/* offset of flags field */
#define	DPM_SIZE	0x02		/* offset of size field */
#define	DPM_OOB_CMD	0x04		/* offset of OOB command field */
#define	DPM_DATA	0x04		/* offset of data field */
#define	DPM_DATASIZE	DP_DATA_SIZE	/* max bytes of data */
#define	DPM_MSGSIZE	(DP_DATA_SIZE + 4)	/* size of an entire message */
#define DPM_OOBDATA     0x06            /* offset of data in OOB msgs */

/* read-write protocol addresses */
#define	RW_MRI		0		/* message-ready byte */
#define	RW_TX		8		/* transmit channel active */
#define	RW_RX		9		/* receive channel active */
#define RW_NR_DONE      0x20            /* Netrom Done OOB action */
#define RW_OOBFLAG      0x21            /* Currently, only for resync */

/* Number of message buffers assigned for receive when target has read 
 * and write access.  */
#define RW_REC_MSGS 15

/* Number of message buffers assigned for transmit when target has read 
 * and write access.  */
#define RW_TX_MSGS 16

/* "Special" out-of-band flag, used for resync */
#define OOBFLAG_RESYNC  0x01            /* Write to DP_OOBFLAG to resync */

#define	DPM_DATASIZE	DP_DATA_SIZE	/* max bytes of data */
#define	DPM_MSGSIZE	(DP_DATA_SIZE + 4)	/* size of an entire message */

/* Message flags */
#define	DPMSG_READY	0x8000		/* ready to be processed */
#define	DPMSG_START	0x0001		/* start of message buffer chain */
#define	DPMSG_END	0x0002		/* end of message buffer chain */
#define	DPMSG_WRAP	0x0004		/* end of message buffers */
#define	DPMSG_OOB	0x0020		/* Out-of-band message */

#define	CF_TXVALID	0x0001		/* transmit size valid */
#define	CF_RXVALID	0x0002		/* receive side valid */
#define	CF_NOWAITIO	0x0004		/* don't wait for bufs to be ready */

/* Out-of-band command numbers */
#define DP_OOB_RESYNC         0x0000
#define DP_OOB_SETMEM         0x0001    /* NetROM sets PodMem for tgt */
#define DP_OOB_RESET          0x0002    /* NetROM does tgtreset */
#define DP_OOB_CPUTS          0x0003    /* Print buf on NR console */
#define DP_OOB_EMOFFONWRITE   0x0004    /* Set emulation off, then setmem */
#define DP_OOB_INTACK         0x0005    /* Tgt acks previous interrupt */
#define DP_OOB_MAXCMD         0x0005    /* Same val as previous command */
#define DP_OOB_RESYNC_ALL     0x0005
#define DP_OOB_CMD            0x0006
#define DP_OOB_ESC            0xFFFF    /* Not used; placeholder */

/* return status codes from nr_GetMsg() */
#define	GM_NODATA	(-1)		/* no data present */
#define	GM_MSGCOMPLETE	0		/* message end read */
#define	GM_NOTDONE	1		/* data present, but not all */
#define	GM_MSGOVERFLOW	2		/* data didn't fit in buffer */

/* size of a message structure */
#define	DP_MSGSTRUCT_SIZE	(4 + DP_DATA_SIZE)
#define	DP_FLAGINDEX		0
#define	DP_SIZEINDEX		2
#define	DP_DATAINDEX    	4

/* Added for overflow buffers */
#define DPMSG_NR_DONE   0x0008          /* NR done Read/Write pod mem */
#define DPMSG_TARG_RDY  0x0010          /* Tgt running in RAM -
                                           NetROM can use pod memory */
#define DPMSG_1K_BIT    0x0400		/* 1K bit of msg length */

/* Overflow buffers are in pod memory just below dual port RAM.  There is
   one for each msg structure and it contains the data that will not fit
   in the msg structure (above 60 bytes). */
#define MAX_MSG_SIZE 1536  /* a full Ethernet packet */
#define MAX_OVF_MSG_SIZE (MAX_MSG_SIZE - DP_DATA_SIZE)

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

/* Structure used to manage character-at-a time i/o in a buffer */
typedef struct _bufIoStruct {
    uInt16	flags;			/* buffer flags */
    uInt16	bufsize;		/* size of data in the buffer */
    int	        index;			/* i/o index */
    uChar	buf[DP_DATA_SIZE];	/* the buffer */
} BufIo;


/* Structure of a communication channel */
typedef struct _dpChannelStruct {
    int		chanflags;	      /* flags */
    int		numaccess;	      /* number of target accesses to rom */
    int		oobthresh;	      /* out-of-band data threshold */
    int		width;		      /* bytes in a rom word */
    int		index;		      /* index of pod 0 in the word */
    Int16 	tx;		      /* transmit msg structures */
    uInt16 	txovf;		      /* transmit overflow buffer */
    Int16 	txlim;		      /* oldest unacked tx msg structure */
    uInt16 	txbase;		      /* base of transmit msg structures */
    uInt16 	txovfbase;	      /* base of transmit overflow buffers */
    void      (*wait_nr_done_ptr)();  /* ptr to RAM routine */
    BufIo	txbuf;		      /* transmit buffer structure */
    Int16	rx;		      /* receive msg structures */
    uInt16	rxovf;		      /* receive overflow buffer */
    Int16	rxlim;		      /* next message location */
    uInt16	rxbase;		      /* base of receive msg structures */
    uInt16	rxovfbase;	      /* base of receive overflow buffers */
    BufIo	rxbuf;		      /* receive buffer structure */
    uInt32	dpbase;		      /* base of dualport ram */
    uChar	rxackval;	      /* rx ack value to write in dp ram */
    uInt32      dpbase_plus_index;    /* added to speed up pod READ/WRITE */
    uInt32      rr_enable;            /* read-read enable for this chan */
    uInt32      rr_disable;           /* read-read disable for this chan */
} DpChannel;

#endif	/* _dualport_h */

/*
**  File:   dptarget.c
**  Description:
**    This file contains routines which are meant to be used by target
**    systems when communicating with the NetROM emulator through Pod 0's
**    dualport ram.
**
**      Copyright (c) 1996 Applied Microsystems Corporation
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
   2/18/94 fdv add two out of band characters to readaddr protocol:
                RA_RESET and RA_RESYNC.
                RA_RESET lets the target trigger a tgtreset.
                RA_RESYNC lets the target trigger the netrom initialize
                    the dualport.

   4/7/94 fdv 1. fixed ra_reset and ra_resync. 2. modified read addr reads
              to "read away" cause of problem seen with ev960ca demo board.
 
   5/4/94 fdv added MISC out of band character to readaddr:
		subfunction is receive interrupt ack.
   5/27/94 fdv added MISC out of band character to readaddr:
		subfunction is turn off emulate before ra_setmem.
   6/16/94 fdv fix to emoffonwrite, it was being called on every set_mem
   2/23/95 sch Add virtual ethernet, both ReadAddr and ReadWrite
   8/21/95 sch Add read after write check to dp_readint().  This
               fixes a data corruption problem when sending large
               amounts of data to the host with little traffic from
               the host.
	   sch Cleanup: Change xchannels back to channels, delete unused
               OVERFLOW_MSGS code.
	   sch Add protocol to end of ra_resync().  Wait for NetROM to
	       turn off RI and then turn it back on before exiting, so
	       the target will not try to send or receive until NetROM
	       is done with the resync.
	   sch More cleanup.  Fix declarations of several routines so 
	       they will agree with their usage.
	   sch In config_dpram, avoid divides when setting oobthresh
	       for readaddr.
	   sch In dp_bread and dp_bwrite, call READ_POD and WRITE_POD
	       if HASCHACHE is true.  These routines will deal with
	       turning off the cache.
    10/12/95 sch Clean up the ra_resync mod.  Take out delay at start.
           Only wait for RI to go on before returning (NetROM will turn
           it off before acking the last RE_RESYNC char).
*/

/**********************************************************************
    Porting notes:
                                OVERVIEW
        This file contains routines which are meant to be used by target
    systems when communicating with the NetROM emulator through Pod 0's
    dualport ram.  These routines are byte-oriented for both transmit and
    receive; however, multibytes messages are passed between target and
    NetROM whenever possible, to reduce handshaking overhead.  Data is
    collected at the target/NetROM interface a byte at a time and passed
    over in messages.
        Messages are fixed-size, fixed format data structures, whose format
    is as follows:
                -----------------------------------------
                | Flags | Size |         Data           |
                -----------------------------------------

                    ^      ^               ^
                    |      |               |
                  2 bytes  |               |
                        2 bytes            |
                                       60 bytes
    Flag values are:
        0x8000          Message ready bit
        0x0001          Start of message bit (for multiple structure messages)
        0x0002          End of message bit (for multiple structure messages)
        0x0004          Message structure Wrap bit
    The Start and End bits are both set in single structure messages.
    The Wrap bit is used to mark the end of an array of structures (for
    example, the transmit array).
    The size field of the structure indicates how many of the data bytes
    are valid.  The size does not include the Flags or the Size field.
    The message structure is described in more detail in the include file,
    dualport.h.
        The protocol between target and NetROM has two distinct flavors;
    one for targets which are capable of writing dualport ram and one for
    targets which can only read dualport ram.  In general, the target will
    poll dualport ram for messages; messages will be ready to process when the
    Ready bit is set.  The scenarios for transmitting data to NetROM are
    different for targets which have write capability and those which do
    not; they are described below.  A key feature of the transmit path
    from target to NetROM is the target's ability to cause an interrupt
    to the NetROM by *reading* special areas of dualport ram.  When NetROM
    receives the interrupt, it uses the target's read address to extract
    an eight-bit value.  The value is equal to the offset of the target's
    read address from the start of the interrupt-causing area of dualport
    ram.
        *** Note *** an important issue in communicating with the NetROM is
    avoiding collisions when accessing dualport ram.  In situations where
    the target and NetROM both access the same dualport address, the target
    will lose.  That is, it will read garbage, or its writes may fail.  This
    is an unavoidable issue, caused by the asynchronous nature of target
    accesses to emulation pod memory, including dualport ram.
        Avoiding access collisions when the target sends data to NetROM is
    accomplished by the NetROM not polling for messages written by the
    target.  Rather, when the target is done writing a message, it sends an
    interrupt to the NetROM by reading a special address in dualport ram.
    Only when the NetROM receives the interrupt will it check dualport ram for
    messages from the target.  If the target cannot write dualport ram,
    all data is passed by read-address interrupts, so access collisions are
    not a problem.
        Access collisions when the NetROM writes messages are unavoidable
    because the target must poll dualport ram for messages.  However, the
    NetROM will only write each byte of ram one time, so the target can
    verify the Ready bit being set in a message, simply by reading the flags
    field twice.  Since NetROM will only set the Ready bit after it has
    written the message data, the target only needs to perform a "verification"
    on the flags fields of message structures.

                            READ-WRITE TARGETS
        Targets which can write dualport ram will use a dualport memory layout
    consisting of an array of 64-byte structures.  There will be 15 receive
    structures and 16 transmit data structures.  Receive structures will
    contain messages written by NetROM and transmit structure will be
    used to send data to NetROM.  The configuration/status structure is
    used to indicate whether or not NetROM is ready to process messages,
    and reading parts of the configuration/status structure will send
    interrupts to NetROM.
                --------------------------------------------
                |       Configuration/Status Structure (*) |
                --------------------------------------------
                |       RX Message Structure 0             |
                --------------------------------------------
                |               ...                        |
                --------------------------------------------
                |       RX Message Structure 14            |
                --------------------------------------------
                |       TX Message Structure 0             |
                --------------------------------------------
                |               ...                        |
                --------------------------------------------
                |       TX Message Structure 15            |
                --------------------------------------------

        The configuration/status structure for targets with write
    capability is simple:
                --------------------------------------------
                | Interrupt Area | TX | RX |   Reserved    |
                --------------------------------------------
                        ^           ^    ^          ^
                        |           |    |          |
                     8 bytes        |    |          |
                                  1 byte |          |
                                       1 byte       |
                                                54 bytes
    The target may read from the interrupt area to generate an interrupt
    to the NetROM.  Upon receipt of this interrupt, the NetROM will check
    the dualport message area for messages from the target.  The TX and
    RX bytes are used to indicate when the NetROM is ready to process
    messages and interrupts.  *** The target should not cause interrupts
    unless these bytes are set, because NetROM will not detect a missed
    interrupt. ***

                            READ-ONLY TARGETS
	Targets which can read but not write dualport ram must use a
    different mechanism to send data to the NetROM.  NetROM
    contains a special area of memory in Pod 0 which, when read by the
    target, causes an interrupt to be generated to NetROM's processor.
    NetROM can determine which address within the 256-byte area was
    read, and uses this address to calculate an 8-bit number.  Using
    this mechanism, it is possible for the target to send 8-bit data to
    the NetROM.  Sending certain 8-bit values is done in two steps.
    This is because there are situations in which it is necessary to
    send out-of-band data to the NetROM; that is, data which is
    relevant to the protocol but not the the content of messages the
    target wants to send.  For example, when the target reads a message
    structure written by the NetROM, it cannot clear the Ready bit in
    the message's flags.  Thus, it uses a special value (that is,
    address) to acknowlege messages received from the NetROM.
    Distinguishing between ACKs (0xF8) and equivalent values in message
    data would require a character-stuffing protocol, which is not
    currently supported.  Thus data values greater than 0xF8 are sent
    in two or more pieces. The first, an "escape" character (0xF9),
    tells NetROM to add 0xF8 to the next character received. The second
    piece is the original value minus 0xF8.  This "character stuffing"
    only needs to be done for values greater than 0xF8.
	The SET character (0xF2) is used when the target system wishes to 
    request that NetROM modify the contents of emulation memory.  After
    receiving the SET character, NetROM will interpret the next four
    characters as an offset into emulation memory, received most-significant
    byte first.  Following the offset is an 8-bit value to be written.  Note
    that the target software should run from RAM from the time it transmits
    the value character to the time that NetROM acknowleges the value
    character.  This is to prevent memory contention problems.  The set
    function is implemented by ra_setmem() in this file.
	The START and END characters are used to delineate a complete message
    being sent via the read-address protocol.  They are analogous to the START
    and END bits in a dualport message structure.  These out-of-band
    characters are useful when using a UDP transport between NetROM and the
    host system, since each UDP datagram will contain a complete message.
    Sending messages is implmented in the ra_putmsg() routine in this file.
        There is an added subtlety in the read-address protocol, which is
    that NetROM is unable to detect missed interrupts.  As a result, prior
    to sending each character, the target must be sure that NetROM is
    listening for interrupts, and that the character that was sent was
    received.  There are two fields in the configuration/status structure
    which accomplish this; one which is set when the NetROM is listening
    for interrupts, and one which is incremented for each character
    (including control characters) that the target sends.  These are described
    below.  Note that the values of all escape characters are different for 
    target systems which perform burst reads from emulation pod 0.  Consult
    your netrom manual for information on these types of systems.
        The target receives messages from the NetROM using a similar
    protocol to that used by the read-write target.  However, it must
    acknowlege each message with a special character, since it cannot
    clear Ready bits itself.

    /--------  256-byte read-address area; target reads of this area
    |       generate interrupts to the NetROM.
    |
    |   /--     --------------------------------------------
    |   |       |                                          |
    |   |       --     248-byte ASCII Read-address Data   --
    --->|       |                                          |
        |       --------------------------------------------
        |       |    Control Read-address Data (8 bytes)  |
        \--     --------------------------------------------
                |       Configuration/Status Structure     |
                --------------------------------------------
                |       RX Message Structure 0             |
                --------------------------------------------
                |               ...                        |
                --------------------------------------------
                |       RX Message Structure 26            |
                --------------------------------------------

     The layout of the control read-address data area.
     --------------------------------------------------------------
     | PACK | ESC | SET | START | END | RESET | RESYNC | MISC |
     --------------------------------------------------------------
        ^      ^     ^      ^      ^      ^        ^       ^
        |      |     |      |      |      |        |       |
     1 byte    |     |      |      |      |        |       |
            1 byte   |      |      |      |        |       | 
	          1 byte    |      |      |        |       |
		         1 byte    |      |        |       |
			        1 byte    |        |       |
				       1 byte      |       |
						1 byte     |
                                                         1 byte

    The PACK byte is used to tell NetROM that a single message has been
    received from dualport RAM.  This byte should be read after *every*
    dualport message.  The ESC byte is used to tell NetROM that the data
    being transmitted is greater or equal in value than 248 (0xF8).  When
    NetROM receives an ESC character, it will add 0xF8 to the character
    which follows, and pass the result to higher level protocols as if it
    were received in a single transmission. RESET lets the target trigger 
    a tgtreset. RESYNC lets the target trigger the netrom initialize the 
    dualport. MISC allows more control by parsing the next char for a
    subfunction. The subfuntions are:
	0x1  Receive Interrupt Acknowledge.
	0x2  turn off emulate mode before performing ra_setmem.


        The layout of the configuration/status structures.
                --------------------------------------------
                | RI | ACK |           Reserved            |
                --------------------------------------------
                   ^    ^                  ^
                   |    |                  |
                1 byte  |                  |
                     1 byte                |
                                       62 bytes

    The RI bit indicates that NetROM is ready to handle interrupts.
    The ACK byte is an 8-bit counter, which is incremented for each character
    which the NetROM receives, including control characters such as
    acks.

                                ABOUT THIS FILE
        This file contains routines which communicate with the NetROM.  To
    be able to send and receive characters, target-side programmers need
    only port the clearly marked section at the top of the file.  A hook
    is provided for tasking systems which need to allow processes to run;
    this is the YIELD_CPU macro.  The routine c_dpcons_hdlr() handles
    receipt of messages from the NetROM.  Since the NetROM cannot cause
    target-side interrupts, it is called when the target checks for data.
        The entry points that the target programmer needs to use are:
            config_dpram        initializes control structures and configures
                        the target to use dualport in a read-only or read-
                        write fashion.
            set_dp_blockio      allows the target programmer to set or
                        clear a bit in the control structure.  When set,
                        the interface routines merely poll for data and
                        return if none is present.  Otherwise they will
                        wait for data to appear.
            dp_chanready        returns 1 if the NetROM is ready to
                        process messages; 0 otherwise.
	    chan_kbhit		returns 1 if a character is waiting at the 
			dualport interface; 0 otherwise.
            ra_putch            sends a character to the NetROM using the
                        read-address interface.  This routine handles all
                        of the appropriate software handshaking.
            chan_putch          sends a character to the NetROM.  Actually,
                        it stores characters until the message structure is
                        full or chan_flushtx() is called.  This reduces
                        protocol overhead.
            chan_flushtx        sends any characters which have been stored
                        but not yet passed to the NetROM.
            ra_getch            reads a character from the NetROM, if one
                        is present, using the RX message structures of the 
			read-address protocol.
            chan_getch          reads a character from the NetROM, if one
                        is present, using the RX message structures of the
			readwrite protocol.
	    ra_getmsg		reads a message from dualport ram, when using
			the read-address protocol.
	    chan_getmsg		reads a message from dualport ram, when using
			the readwrite protocol.
	    ra_putmsg		sends a complete message delineated by the 
			START and END out-of-band characters.
	    chan_putmsg		sends a complete message delineated by the
	    		START and END bits in the dualport message structures.
			This differs from the ra_putch() routine, which treats
			each dualport message structure as a complete
			message.
	    ra_setmem		requests that NetROM write a byte of 
			emulation memory.
			if ra_emoffonwrite has been called, emulation memory is
			turned off before the byte is written.
	    ra_reset		requests that NetROM reset the target.	
            ra_resync		requests that NetROM re-initialize  it's 
			dualport parameters.
	    ra_rx_intr_ack	acknowledges a receive interrupt
	    ra_emoffonwrite	see ra_setmem

	Note that the getmsg() routines are used to incrementally fill an
    input buffer.  When used in a polling mode, they return one of four 
    status values:  GM_NODATA indicates that no data has arrived since the
    last poll; GM_MSGCOMPLETE indicates that new data has arrived and that the
    input buffer now holds the complete message; GM_NOTDONE indicates that
    data has arrived but that the message is not yet complete; GM_MSGOVERFLOW
    indicates that more data has arrived, but that it has overflown the input
    buffer.  In a non-polling mode, these routines will return either
    GM_MSGCOMPLETE or GM_MSGOVERFLOW.

*********************************************************************/
/* Local Defines */
#define DUMMY_READ

#include "wdb/dpconfig.h"     /* This file must be ported to the new target */
#include "wdb/dptarget.h"	     
#include "wdb/dualport.h"
#include "stdio.h"


/* declarations */
STATIC void c_dpcons_hdlr();
void chan_flushtx();
void set_dp_blockio();
void ra_putch();
int do_emoffonwrite();

/* global data */
STATIC DpChannel channels[DP_MAXCHANNELS];

#ifdef READONLY_TARGET
STATIC int emoffonwrite_set;
#endif


#ifndef RAWRITES_ONLY
STATIC uInt16 dp_readint();
void dp_bwrite();
void dp_bread();

#ifndef	READONLY_TARGET
/* writes an int to dp ram */
STATIC void
dp_writeint(addr,val)
uInt32 addr;
uInt16 val;
{	

    /* write in network byte order, lsb first */
    WRITE_POD(&channels[0], addr + 1, (uInt32)val & 0xFF);
    WRITE_POD(&channels[0], addr, (uInt32)(val >> 8) & 0xFF);

    /* Check that the write was sucessful.  See change log dated 8/21/95
       at the top of this file for details. */
    while (val != dp_readint(addr)) {
        WRITE_POD(&channels[0], addr + 1, (uInt32)val & 0xFF);
        WRITE_POD(&channels[0], addr, (uInt32)(val >> 8) & 0xFF);
    }
}
#endif

/* read an int from dpram */
STATIC uInt16
dp_readint(addr)
uInt32 addr;
{
    uInt16 val;

    /* read the msb first, in case it contains flags */

    val = READ_POD(&channels[0], addr);
    val <<= 8;
    val += READ_POD(&channels[0], addr + 1);

    /* return, in host byte order */
    return(val);
}


/* copy bytes to dual-port memory (optimized version - move stuff
   out of the loop )*/
void
dp_bwrite(src, addr, size)
uChar *src;
uInt32 addr;
int size;
{
    register int inc;
    register volatile uChar *dst;
    register int i;
    DpChannel *cp = &channels[0];

    inc = cp->width;
    dst = (volatile uChar *)(cp->dpbase + (cp->width * addr) + cp->index);

    for(i = 0; i < size; i++) {
#if (HASCACHE == True)
    WRITE_POD(cp, addr++, *src);
#else
        *dst = *src;
        dst += inc;
#endif
        src++;
    }
}
/* copy bytes from dual-port memory (optimized version - move stuff
   out of the loop) */
void
dp_bread(addr, buf,  size)
uInt32 addr;
volatile uChar *buf;
int size;
{
    register int inc;
    register volatile uChar *src;
    register int i;
    DpChannel *cp = &channels[0];

    inc = cp->width;
    src = (volatile uChar *)(cp->dpbase + (cp->width * addr) + cp->index);

    for(i = 0; i < size; i++) {
#if (HASCACHE == True)
	uChar tmp = READ_POD(cp, addr++);
        *buf = tmp;
#else
        *buf = *src;
        src += inc;
#endif
        buf ++;
    }
}
#endif	/* RAWRITES_ONLY */

#ifdef READONLY_TARGET

#ifndef RAWRITES_ONLY
#ifdef USE_MSGS
/* this routine reads message from dualport ram.  It returns different
 * statuses, depending on whether or not message data is present, the message
 * is complete, the message is not complete but data was present, or
 * the message overflows the buffer.  The number of bytes read into the 
 * message buffer is changed by side effect. 
 * 
 * This routine doesn't use the buffer pointers in the channel structure. 
 * Note that GM_NODATA will only be returned if we are not in a blocking I/O
 * mode. */
int
ra_getmsg(buf, len, bytesread)
uChar *buf;
int len, *bytesread;
{
    int done = 0;
    int nbytes = 0;
    register DpChannel *cp = &channels[0];
    register uInt16 flags, size;
    register uInt16 msg_size;
    volatile uChar dummy;

    /* look for the whole message */
    while(done == 0) {
	/* check whether a buffer has become valid */
	while(cp->rx == (-1)) {
	    /* poll for new buffers */
	    c_dpcons_hdlr(0, RA_MSGBASE);

	    /* did we get a message? */
	    if(cp->rx == (-1)) {
#ifdef VETHER
		if((cp->chanflags & CF_NOWAITIO) || (nbytes == 0)) {
#else
		if(cp->chanflags & CF_NOWAITIO) {
#endif
		    *bytesread = nbytes;
		    return(nbytes == 0 ? GM_NODATA : GM_NOTDONE);
		}
		YIELD_CPU();
	    }
	}

	/* synchronize ACK values */
	cp->rxackval = READ_POD(cp, RA_ACK);

	/* get the status and length of the message */
	flags = dp_readint(cp->rx + DPM_FLAGS);
	size = dp_readint(cp->rx + DPM_SIZE);
	/* Msg length may be in top 10 bits of size field of 1st msg */
#ifdef VETHER
	if (nbytes == 0) {
	    msg_size = (size >> 6) | (flags & DPMSG_1K_BIT);
        }
#endif
	size = size & 0x3f;

	/* copy data into the buffer */
	if(size > len) {
	    *bytesread = nbytes;
	    /*printf("ra_getmsg: msglen=%d, buf=%d, read=%d ", size,len,nbytes);*/
	    return(GM_MSGOVERFLOW);
	}

	/* copy the message into the buffer */
	dp_bread(cp->rx + DPM_DATA, buf, (int) size);

	/* update our buffer pointers */
	buf += size;
	nbytes += size;
	len -= size;

	/* return the buffer */
	dummy = READ_POD(cp, READADDR_CTLCHAR(cp, RA_PACK_INDEX));

	/* wait for our packet ack to be acked in its turn; read the ack
	 * from NetROM twice to make sure it's valid */
	cp->rxackval++;
	while(READ_POD(cp, RA_ACK) != cp->rxackval && 
	      READ_POD(cp, RA_ACK) != cp->rxackval) {
#ifdef DUMMY_READ
	    dummy = READ_POD(cp, RA_ACK+8 );
#endif
	    ;	/* do nothing */
	}

	/* advance the read pointer */
	if(flags & DPMSG_WRAP) {
	    cp->rx = cp->rxbase;
	} else {
	    cp->rx += DPM_MSGSIZE;
	}

	/* see if there are more messages waiting */
	if(cp->rx == cp->rxlim) {
	    cp->rx = (-1);
	}

	/* was this the end of the message? */
	if((flags & DPMSG_END) != 0) done = 1;
    }

#ifdef VETHER
    /* Error check if msg len was in 1st buffer */
    /* If we have a console for printf to use, do this check
    if (msg_size) {
	if (msg_size != nbytes)
	    printf("ra_getmsg: 1st buf len=%d, actual=%d\n",msg_size,nbytes);
    } */
#endif
    *bytesread = nbytes;
    return(GM_MSGCOMPLETE);
}
#endif /* USE_MSGS */

int
ra_getch()
/* This routine attempts to read a character from the receive msg buffers of 
 * the readaddress channel.  */
{
   DpChannel *cp = &channels[0];
   BufIo *bp = &cp->rxbuf;
   int ch;
   volatile uChar dummy;

    /* see if there is a character in this buffer */
    if(bp->index < 0) {
        /* wait for the buffer to become valid */
        while(cp->rx == (-1)) {
            /* poll for new buffers */
            c_dpcons_hdlr(0, RA_MSGBASE);

	    /* did we get a message? */
	    if(cp->rx == (-1)) {
		if(cp->chanflags & CF_NOWAITIO) return(-1);
		YIELD_CPU();
	    }
        }

	/* synchronize ACK values */
	cp->rxackval = READ_POD(cp, RA_ACK);

        /* make sure that the message is ready by reading the flags byte 
	 * of the next receive msg. */
        bp->flags = dp_readint(cp->rx + DPM_FLAGS);

	/* Re-read the flags because the NetROM may have been
	 * modifying them while we read them. */
        if(bp->flags != dp_readint(cp->rx + DPM_FLAGS)) {
            /* read failed on the verify, NetROM must be writing */
            bp->flags = dp_readint(cp->rx + DPM_FLAGS);
        }
        if((bp->flags & DPMSG_READY) == 0) {
            return(-1);
        }

        /* set up the i/o buffer for the message */
        bp->bufsize = dp_readint(cp->rx + DPM_SIZE);
        if(bp->bufsize > DP_DATA_SIZE) {
            bp->bufsize = DP_DATA_SIZE;
        }
        dp_bread(cp->rx + DPM_DATA, bp->buf, (int) bp->bufsize);
        bp->index = 0;

	/* return the buffer */
	dummy = READ_POD(cp, READADDR_CTLCHAR(cp, RA_PACK_INDEX));

	/* wait for our packet ack to be acked in its turn; read the ack
	 * from NetROM twice to make sure it's valid */
	cp->rxackval++;
	while(READ_POD(cp, RA_ACK) != cp->rxackval && 
		READ_POD(cp, RA_ACK) != cp->rxackval) {
		/* read different address to avoid contention on some targets*/
#ifdef DUMMY_READ
             dummy = READ_POD(cp, RA_ACK+8 );
#endif
	}

        /* advance the read pointer */
        if(bp->flags & DPMSG_WRAP) {
            cp->rx = cp->rxbase;
        } else {
            cp->rx += DPM_MSGSIZE;
        }

        /* see if there are more messages waiting */
        if(cp->rx == cp->rxlim) {
            cp->rx = (-1);
        }
    }

    /* extract the character */
    ch = (unsigned int) bp->buf[bp->index++];

    /* check whether we finished the buffer */
    if(bp->index == bp->bufsize) {
        /* invalidate the buffer */
        bp->index = (-1);
    }

    return(ch);
}
#endif	/* ! RAWRITES_ONLY */

static void
ra_sendchar(ch)
uChar ch;
{
    register DpChannel *cp = &channels[0];
    register volatile uChar val, dummy;
    register uChar expval;

    /* reading a character sends an interrupt to NetROM */
    dummy = READ_POD(cp, READADDR_DATACHAR(cp, ch));

    /* wait for it to be acked */
    expval = ++cp->rxackval;
    while(1) {
	    val = READ_POD(cp, RA_ACK);
#ifdef DUMMY_READ
	    dummy = READ_POD(cp, RA_ACK+8); 
#endif
	    if(val == expval && val == READ_POD(cp, RA_ACK)) {
		/* got an ack for our last character */
		break;
	    }
    }
}

void
ra_putch(ch)
uChar ch;
/* 
	This routine sends a character to NetROM over the ReadAddress
	channel.
*/
{
    register volatile uChar val;
    register DpChannel *cp = &channels[0];

    /* wait for the NetROM to be ready */
    while(1) {
        val = READ_POD(cp, RA_RI);
        if(val == 1 && val == READ_POD(cp, RA_RI)) {
            /* NetROM is receiving */
	    cp->rxackval = READ_POD(cp, RA_ACK);	/* AMP */
            break;
        }
    }

    /* make sure we can send the character */
    while(ch >= cp->oobthresh) {
	/* send the escape character */
	ra_sendchar(cp->oobthresh + RA_ESC_INDEX);

	/* adjust the character we're sending */
	ch -= cp->oobthresh;
    }

    /* send the real character */
    ra_sendchar(ch);
}
#ifndef RAWRITES_ONLY

#ifdef USE_MSGS
/* This routine sends a complete message to NetROM over the ReadAddress
 * channel.  */
void
ra_putmsg(buf, len)
uChar *buf;
int len;
{
    register volatile uChar val;
    register DpChannel *cp = &channels[0];

    /* wait for the NetROM to be ready */
    while(1) {
        val = READ_POD(cp, RA_RI);
        if(val == 1 && val == READ_POD(cp, RA_RI)) {
            /* NetROM is receiving */
	    cp->rxackval = READ_POD(cp, RA_ACK);	/* AMP */
            break;
        }
    }

    /* send the start message delimiter */
    ra_sendchar(cp->oobthresh + RA_STARTMSG_INDEX);

    /* send the message */
    while(len != 0) {
	ra_putch(*buf);
	buf++;
	len--;
    }
    /* send the end message delimiter */
    ra_sendchar(cp->oobthresh + RA_ENDMSG_INDEX);
}
#endif /* USE_MSGS */
#endif	/* ! RAWRITES_ONLY */

/* this routine runs from ram to avoid conflicts with NetROM as it sets
 * memory */
void
ra_setmem_sendval(ch)
uChar ch;
{
    register volatile uChar dummy, expval, val;
    register DpChannel *cp = &channels[0];

    /* make sure we can send the character */
    while(ch >= cp->oobthresh) {
	/* reading a character sends an interrupt to NetROM */
	dummy = READ_POD(cp, READADDR_CTLCHAR(cp, RA_ESC_INDEX));

	/* wait for it to be acked */
	expval = ++cp->rxackval;
	while(1) {
	    val = READ_POD(cp, RA_ACK);
	    if(val == expval && val == READ_POD(cp, RA_ACK)) {
		/* got an ack for our last character */
		break;
	    }
	}

	/* adjust the character we're sending */
	ch -= cp->oobthresh;
    }

    /* reading a character sends an interrupt to NetROM */
    dummy = READ_POD(cp, READADDR_DATACHAR(cp, ch));

    /* wait for it to be acked */
    expval = ++cp->rxackval;
    while(1) {
        val = READ_POD(cp, RA_ACK);
        if(val == expval && val == READ_POD(cp, RA_ACK)) {
            /* got an ack for our last character */
            break;
        }
    }
}

/* sends a request to NetROM to write a byte of memory */
void
ra_setmem(ch, addr, buf)
uChar ch;
uInt32 addr;
uChar *buf;	/* should be RA_SETMEM_RTN_SIZE bytes, 32 bit aligned */
{
    register volatile uChar dummy, expval, val;
    register DpChannel *cp = &channels[0];
    uChar *srcp;
    int tmp;
    void (*sendvalp)();

    if(emoffonwrite_set == 1) { 
	do_emoffonwrite(); 
	emoffonwrite_set = 0;
    }
    /* wait for the NetROM to be ready */
    while(1) {
        val = READ_POD(cp, RA_RI);
        if(val == 1 && val == READ_POD(cp, RA_RI)) {
            /* NetROM is receiving */
	    cp->rxackval = READ_POD(cp, RA_ACK);	/* AMP */
            break;
        }
    }

    /* send the set memory character */
    dummy = READ_POD(cp, READADDR_CTLCHAR(cp, RA_SET_INDEX));

    /* wait for it to be acked */
    expval = ++cp->rxackval;
    while(1) {
        val = READ_POD(cp, RA_ACK);
        if(val == expval && val == READ_POD(cp, RA_ACK)) {
            /* got an ack for our last character */
            break;
        }
    }

    /* send the offset */
    ra_putch((uChar) ((addr >> 24) & 0xFF));
    ra_putch((uChar) ((addr >> 16) & 0xFF));
    ra_putch((uChar) ((addr >> 8) & 0xFF));
    ra_putch((uChar) (addr & 0xFF));

    /* copy our sendval routine to ram */
    tmp = (uInt32) buf & 0x03;		/* force 32-bit alignment */
    if(tmp != 0)  buf += (4 - tmp);
    srcp = (uChar *) ra_setmem_sendval;
    sendvalp = (void (*)()) buf;
    for(tmp = 0; tmp < RA_SETMEM_RTN_SIZE; tmp++) {
	*buf = *srcp;
	buf++;
	srcp++;
    }

    /* call the sendval routine */
    (*sendvalp)(ch);
}

/* sends a request to NetROM to reset the target
      returns 0 if NetROM not ready
*/
int
ra_reset()
{
    register volatile uChar dummy, expval, val;
    register DpChannel *cp = &channels[0];

    /* check for the NetROM to be ready for interrupts */
        val = READ_POD(cp, RA_RI);
        if(val == !1 || val != READ_POD(cp, RA_RI)) {
            /* NetROM is not receiving */
		return(0);
	}
	cp->rxackval = READ_POD(cp, RA_ACK);        /* AMP */

    /* send  the 1st sequence character */
    dummy = READ_POD(cp, READADDR_CTLCHAR(cp, RA_RESET));

    /* wait for it to be acked */
    expval = ++cp->rxackval;
    while(1) {
        val = READ_POD(cp, RA_ACK);
        if(val == expval && val == READ_POD(cp, RA_ACK)) {
            /* got an ack for our last character */
            break;
        }
    }
    /* send the 2nd sequence char  */
    ra_putch((uChar) (0x01));

    /* send  the 3rd sequence character */
    dummy = READ_POD(cp, READADDR_CTLCHAR(cp, RA_RESET));

    /* wait for it to be acked */
    expval = ++cp->rxackval;
    while(1) {
        val = READ_POD(cp, RA_ACK);
        if(val == expval && val == READ_POD(cp, RA_ACK)) {
            /* got an ack for our last character */
            break;
        }
    }
    /* send the 4th sequence char  */
    dummy = READ_POD(cp, READADDR_CTLCHAR(cp, RA_RESET));
    ra_putch((uChar) (0x02));

    /* send  the last sequence character */
    dummy = READ_POD(cp, READADDR_CTLCHAR(cp, RA_RESET));
    /*  don't wait for it to be acked */
    return(1);
}

/* sends a request to NetROM to re-initalize (resync) the dualport parameters
      returns 0 if NetROM not ready, returns 1 when done
 */
int
ra_resync()
{
    register volatile uChar dummy, expval, val;
    register DpChannel *cp = &channels[0];

    /* check for the NetROM to be ready for interrupts */
        val = READ_POD(cp, RA_RI);
        if(val == !1 || val != READ_POD(cp, RA_RI)) {
            /* NetROM is not receiving */
		return(0);
	}
	cp->rxackval = READ_POD(cp, RA_ACK);        /* AMP */

    /* send  the 1st sequence character */
    dummy = READ_POD(cp, READADDR_CTLCHAR(cp, RA_RESYNC));

    /* wait for it to be acked */
    expval = ++cp->rxackval;
    while(1) {
        val = READ_POD(cp, RA_ACK);
	dummy = READ_POD(cp, RA_ACK+8); 
        if(val == expval && val == READ_POD(cp, RA_ACK)) {
            /* got an ack for our last character */
            break;
        }
    }

    /* send the 2nd sequence char  */
    ra_putch((uChar) (0x01));

    /* send  the 3rd sequence character */
    dummy = READ_POD(cp, READADDR_CTLCHAR(cp, RA_RESYNC));

    /* wait for it to be acked */
    expval = ++cp->rxackval;
    while(1) {
        val = READ_POD(cp, RA_ACK);
	dummy = READ_POD(cp, RA_ACK+8);
        if(val == expval && val == READ_POD(cp, RA_ACK)) {
            /* got an ack for our last character */
            break;
        }
    }

    /* send the 4th sequence char  */
    ra_putch((uChar) (0x02));

    /* send  the last sequence character */
    dummy = READ_POD(cp, READADDR_CTLCHAR(cp, RA_RESYNC));
    /* wait for it to be acked */
    expval = ++cp->rxackval;
    while(1) {
        val = READ_POD(cp, RA_ACK);
	dummy = READ_POD(cp, RA_ACK+8); 
        if(val == expval && val == READ_POD(cp, RA_ACK)) {
            /* got an ack for our last character */
            break;
        }
    }

    /* Wait for NetROM to turn RA_RI back on before completing,
       so that we won't try anything else before NetROM has resynced. 
       Requires NetROM firmware version 1.3.1 or later. */
    while(1) {
        val = READ_POD(cp, RA_RI);
        if(val == 1 && val == READ_POD(cp, RA_RI)) {
            break;
        }
    }
    return(1);
}


/* acknowledges a receive interrupt 
      returns 0 if NetROM not ready, returns 1 when done
 */
int
ra_rx_intr_ack()
{
    register volatile uChar dummy, expval, val;
    register DpChannel *cp = &channels[0];


    /* check for the NetROM to be ready for interrupts */
        val = READ_POD(cp, RA_RI);
        if(val == !1 || val != READ_POD(cp, RA_RI)) {
            /* NetROM is not receiving */
		return(0);
	}
	cp->rxackval = READ_POD(cp, RA_ACK);        /* AMP */

    /* send  the 1st sequence character */
    dummy = READ_POD(cp, READADDR_CTLCHAR(cp, RA_MISC));

    /* wait for it to be acked */
    expval = ++cp->rxackval;
    while(1) {
        val = READ_POD(cp, RA_ACK);
#ifdef DUMMY_READ
	dummy = READ_POD(cp, RA_ACK +8);
#endif
        if(val == expval && val == READ_POD(cp, RA_ACK)) {
            /* got an ack for our last character */
            break;
        }
    }
    /* send the 2nd sequence char  */
    ra_putch((uChar) (RX_INTR_ACK));

    return(1);
}

/* tells netrom to turn off emulation before writing to memory 
      returns 0 if NetROM not ready, returns 1 when done
 */
void
ra_emoffonwrite()
{

	emoffonwrite_set=1; 
	/* set global so ra_setmem can first call do_emoffonwrite just in case 
        	debug connection was not connected at the time
		of this call */
}
int
do_emoffonwrite()
{
    register volatile uChar dummy, expval, val;
    register DpChannel *cp = &channels[0];

    /* check for the NetROM to be ready for interrupts */
        val = READ_POD(cp, RA_RI);
        if(val == !1 || val != READ_POD(cp, RA_RI)) {
            /* NetROM is not receiving */
		return(0);
	}
	cp->rxackval = READ_POD(cp, RA_ACK);        /* AMP */

    /* send  the 1st sequence character */
    dummy = READ_POD(cp, READADDR_CTLCHAR(cp, RA_MISC));

    /* wait for it to be acked */
    expval = ++cp->rxackval;
    while(1) {
        val = READ_POD(cp, RA_ACK);
#ifdef DUMMY_READ
	dummy = READ_POD(cp, RA_ACK+8);
#endif
        if(val == expval && val == READ_POD(cp, RA_ACK)) {
            /* got an ack for our last character */
            break;
        }
    }
    /* send the 2nd sequence char  */
    ra_putch((uChar) (EMOFFONWRITE));

    return(1);
}
#endif	/* READONLY_TARGET */

#ifdef READWRITE_TARGET
#ifdef USE_MSGS
/* this routine reads message from dualport ram.  It returns different
 * statuses, depending on whether or not message data is present, the message
 * is complete, the message is not complete but data was present, or
 * the message overflows the buffer.  The number of bytes read into the 
 * message buffer is changed by side effect. 
 * 
 * This routine doesn't use the buffer pointers in the channel structure. 
 * Note that GM_NODATA will only be returned if we are not in a blocking I/O
 * mode. */
int
chan_getmsg(chan, buf, len, bytesread)
int chan;
uChar *buf;
int len, *bytesread;
{
    register DpChannel *cp = &channels[0];
    int done = 0;
    int nbytes = 0;
    register uInt16 flags, size;
    int msg_len = 0;

    /* look for the whole message */
    while(done == 0) {
        /* wait for the buffer to become valid */
        while(cp->rx == (-1)) {
            /* poll for new buffers */
            c_dpcons_hdlr(chan, RW_MSGBASE);

	    /* did we get a message? */
	    if(cp->rx == (-1)) {
#ifdef VETHER
		if((cp->chanflags & CF_NOWAITIO) || (nbytes == 0)) {
#else
		if(cp->chanflags & CF_NOWAITIO) {
#endif
		    *bytesread = nbytes;
		    return(nbytes == 0 ? GM_NODATA : GM_NOTDONE);
		}
		YIELD_CPU();
	    }
        }

	/* read the status and size of the new message block */
        flags = dp_readint(cp->rx + DPM_FLAGS);
        size = dp_readint(cp->rx + DPM_SIZE);

#ifdef VETHER
	/* get whole msg len from first pkt of msg */
	if (nbytes == 0) {
	    /* if ((flags & DPMSG_START) == 0)
		 printf("no START: %04x ", flags & 0xffff); */
	    msg_len = (size >> 6) | (flags & DPMSG_1K_BIT);
	    size = size & 0x3f;
	}
#endif
	/* copy data into the buffer */
	if(size > len) {
	    *bytesread = nbytes;
	    return(GM_MSGOVERFLOW);
	}
        dp_bread(cp->rx + DPM_DATA, buf, (int) size);

        /* return the buffer */
        dp_writeint(cp->rx + DPM_FLAGS, flags & ~DPMSG_READY);

	/* update our buffer pointers */
	buf += size;
	nbytes += size;
	len -= size;

        /* advance the read pointer */
        if(flags & DPMSG_WRAP) {
            cp->rx = cp->rxbase;
        } else {
            cp->rx += DPM_MSGSIZE;
        }

        /* see if there are more messages waiting */
        if(cp->rx == cp->rxlim) {
            cp->rx = (-1);
        }

	/* was this the end of the message? */
	if((flags & DPMSG_END) != 0) done = 1;
    }
    *bytesread = nbytes;
#ifdef VETHER
    if (nbytes != msg_len) {
	/* printf("chan_getmsg: 1st pkt len=%d, got %d\n", msg_len, nbytes); */
	return(GM_NOTDONE);
    } else 
#endif
    	return(GM_MSGCOMPLETE);
}
int
chan_getch(chan)
int chan;
/* This routine attempts to read a character from the receive msg buffers 
 * of the readwrite channel.  */
{
    DpChannel *cp = &channels[chan];
    BufIo *bp = &cp->rxbuf;
    int ch;

    /* see if there is a character in this buffer */
    if(bp->index < 0) {
        /* wait for the buffer to become valid */
        while(cp->rx == (-1)) {
            /* poll for new buffers */
            c_dpcons_hdlr(chan, RW_MSGBASE);

	    /* did we get a message? */
	    if(cp->rx == (-1)) {
		if(cp->chanflags & CF_NOWAITIO) return(-1);
		YIELD_CPU();
	    }
        }

        /* make sure that the message is ready by reading the flags byte 
	 * of the next receive msg.  */
        bp->flags = dp_readint(cp->rx + DPM_FLAGS);

	/* Re-read the flags because the NetROM may have been modifying 
	 * them while we read them. */
        if(bp->flags != dp_readint(cp->rx + DPM_FLAGS)) {
            /* read failed on the verify, NetROM must be writing */
            bp->flags = dp_readint(cp->rx + DPM_FLAGS);
        }
        if((bp->flags & DPMSG_READY) == 0) {
            return(-1);
        }

        /* set up the i/o buffer for the message */
        bp->bufsize = dp_readint(cp->rx + DPM_SIZE);
        if(bp->bufsize > DP_DATA_SIZE) {
            bp->bufsize = DP_DATA_SIZE;
        }
        dp_bread(cp->rx + DPM_DATA, bp->buf, (int) bp->bufsize);
        bp->index = 0;

        /* return the buffer */
        dp_writeint(cp->rx + DPM_FLAGS, (uInt16)bp->flags & ~DPMSG_READY);

        /* advance the read pointer */
        if(bp->flags & DPMSG_WRAP) {
            cp->rx = cp->rxbase;
        } else {
            cp->rx += DPM_MSGSIZE;
        }

        /* see if there are more messages waiting */
        if(cp->rx == cp->rxlim) {
            cp->rx = (-1);
        }
    }

    /* extract the character */
    ch = (unsigned int) bp->buf[bp->index++];

    /* check whether we finished the buffer */
    if(bp->index == bp->bufsize) {
        /* invalidate the buffer */
        bp->index = (-1);
    }

    return(ch);
}


/* send a message to netrom */
void
chan_putmsg(chan, buf, len)
int chan;
uChar *buf;
register int len;
{
    register DpChannel *cp = &channels[chan];
    register int num_sent = 0;
    int size, num_left = len, ovf_size;
    volatile uChar dummy;
    register uInt16 flags;

    while(num_sent < len) {
        /* wait for the buffer */
        while(1) {
            flags = dp_readint(cp->tx + DPM_FLAGS);

            if(flags != dp_readint(cp->tx + DPM_FLAGS)) {
                /* read failed on the verify, NetROM must be writing */
                continue;
            }
            if((flags & DPMSG_READY) == 0) 
                break;
#ifdef DUMMY_READ
	    dummy = READ_POD(cp, RA_ACK+8 );
#endif
            YIELD_CPU(); 
        }

	/* get the number of bytes to send in DP struct and overflow */
	if(num_left >= DP_DATA_SIZE) {
	    size = DP_DATA_SIZE;
	    if (num_left >= MAX_MSG_SIZE)
               	ovf_size = MAX_OVF_MSG_SIZE;
	    else
               	ovf_size = num_left - DP_DATA_SIZE;
	} else {
	    size = num_left;
            ovf_size = 0;
	}

	/* fill the currently available DP buffer */
	/* Put full msg size in upper bits of first size field */
	if (num_sent == 0) { 
	    dp_writeint(cp->tx + DPM_SIZE, (len << 6) | size);
	} else
	    dp_writeint(cp->tx + DPM_SIZE, size); 

	dp_bwrite(buf, cp->tx + DPM_DATA, size);

	/* get the flags to write */
	flags = (flags & DPMSG_WRAP) | DPMSG_READY;
	if(num_sent == 0) {
            flags |= DPMSG_START;
	    /* or in 1K bit of msg len */
	    flags =  flags | (len & DPMSG_1K_BIT); 
	}
	num_sent += size;
	num_left -= size;

	if(num_sent >= len) 
            flags |= DPMSG_END;
	dp_writeint(cp->tx + DPM_FLAGS, flags);
	dp_writeint(cp->tx + DPM_FLAGS, flags); /* try writing it twice */

	/* notify NetROM that the buffer is ready */
	dummy = READ_POD(cp, READADDR_CTLCHAR(cp , RW_MRI));

	/* adjust pointers */
	buf += size;

	/* advance the msg pointer */
	if(flags & DPMSG_WRAP) {
	    cp->tx    = cp->txbase;
	    cp->txovf = cp->txovfbase;
	} else {
	    cp->tx    += DPM_MSGSIZE;
	    cp->txovf += MAX_OVF_MSG_SIZE;
	}
    }
}


/* flush the current character-oriented tx channel */
void
chan_flushtx(chan)
int chan;
{
    DpChannel *cp = &channels[chan];
    BufIo *bp = &cp->txbuf;
    volatile uChar dummy;

    /* if we don't own the message or there's nothing in it, just return */
    bp->flags = dp_readint(cp->tx + DPM_FLAGS);
    if((bp->flags & DPMSG_READY) != 0 || bp->index == 0) {
        return;
    }

    /* send the buffer */
    dp_writeint(cp->tx + DPM_SIZE, (uInt16)bp->bufsize);
    dp_bwrite(bp->buf, cp->tx + DPM_DATA, (int) bp->bufsize);
    bp->flags &= DPMSG_WRAP;
    dp_writeint(cp->tx + DPM_FLAGS,
        (uInt16)bp->flags | (DPMSG_READY|DPMSG_START|DPMSG_END));

    /* notify NetROM that the buffer is ready */
    dummy = READ_POD(cp, READADDR_CTLCHAR(cp, RW_MRI));

    /* advance the msg pointer */
    if(bp->flags & DPMSG_WRAP) {
        cp->tx = cp->txbase;
    } else {
        cp->tx += DPM_MSGSIZE;
    }

    /* clear the buffer structure */
    bp->index = 0;
    bp->bufsize = 0;
}

int
chan_putch(chan, ch)
int chan;
uChar ch;
/*
	Write a character into the transmit area of the readwrite
	channel.
*/
{
    DpChannel *cp = &channels[chan];
    BufIo *bp = &cp->txbuf;

    /* if the current tx channel is owned by the target, wait for it */
    bp->flags = dp_readint(cp->tx + DPM_FLAGS);
    if(bp->flags & DPMSG_READY) {
        /* wait for the buffer */
        while(1) {
            bp->flags = dp_readint(cp->tx + DPM_FLAGS);
            if(bp->flags != dp_readint(cp->tx + DPM_FLAGS)) {
                /* read failed on the verify, NetROM must be writing */
                continue;
            }
            if((bp->flags & DPMSG_READY) == 0) break;
            if(cp->chanflags & CF_NOWAITIO) return(-1);
            YIELD_CPU();
        }

        /* initialize the buffer structure */
        bp->index = 0;
        bp->bufsize = 0;
    }

    /* write the character into the buffer */
    bp->buf[bp->index++] = ch;
    bp->bufsize++;

    /* if the buffer is full, send it */
    if(bp->index == DP_DATA_SIZE) {
        chan_flushtx(chan);
    }

    return(1);
}
#endif /* USE_MSGS */
#endif	/* READWRITE_TARGET */

#ifndef RAWRITES_ONLY

/* check if a character is waiting at the channel */
int
chan_kbhit(chan)
int chan;
{
    DpChannel *cp = &channels[chan];
    BufIo *bp = &cp->rxbuf;
    int retval;

    /* see if there is a character in this buffer */
    if(bp->index < 0) {
        /* poll for new buffers */
	if(cp->chanflags & CF_TXVALID)
        	c_dpcons_hdlr(chan,RW_MSGBASE);
	else
        	c_dpcons_hdlr(chan,RA_MSGBASE);

        /* no, check for a new buffer arriving */
        if(cp->rx != (-1)) {
            retval = 1;
        } else {
            retval = 0;
        }
    } else {
        retval = 1;
    }

    return(retval);
}

#endif	/* ! RAWRITES_ONLY */

/* returns 1 if the "active" byte for a particular channel's transmit
 * and/or receive paths are set, returns 0 otherwise */
int
dp_chanready(chan)
int chan;
{
    DpChannel *cp = &channels[chan];
    uChar val;

    if(cp->chanflags & CF_TXVALID) {
        /* check that the rx channel is active */
        val = READ_POD(cp, RW_RX);
        if(val != READ_POD(cp, RW_RX)) {
            return(0);
        } else if(val != 1) {
            return(0);
        }

        /* check that the tx channel is active */
        val = READ_POD(cp, RW_TX);
        if(val != READ_POD(cp, RW_TX)){
            return(0);
        } else if(val != 1) {
            return(0);
        }
    } else {
        /* check that the NetROM is ready for interrupts */
        val = READ_POD(cp, RA_RI);
        if(val != 1 || val != READ_POD(cp, RA_RI)) {
            /* NetROM is not receiving */
            return(0);
        }
    }

    /* NetROM is ready */
    return(1);
}

#ifndef RAWRITES_ONLY
/* if the CF_NOWAITIO bit is set in a channel's flags, the channel's get
 * and put character methods, will return immediately if there are no
 * available message structures in dualport ram.  This routine sets or
 * clears that bit. */
void
set_dp_blockio(chan, val)
int chan, val;
{
    DpChannel *cp = &channels[chan];

    if(val == 0) {
        cp->chanflags |= CF_NOWAITIO;
    } else {
        cp->chanflags &= ~CF_NOWAITIO;
    }
}

/* This routine handles any messages received from NetROM. */
STATIC void 
c_dpcons_hdlr(chan,base)
int chan;
uInt32 base;
{
    register DpChannel *cp;
    register uInt16 flags;
    /* volatile uChar dummy; */

    /* get a pointer to the channel */
    cp = &channels[0];

    /* a received message indicator */
    while(cp->rx != cp->rxlim) {
        flags = dp_readint(cp->rxlim + DPM_FLAGS);
        if(flags != dp_readint(cp->rxlim + DPM_FLAGS)) {
                /* NetROM must be writing, try again */
                continue;
        }

        if(flags & DPMSG_READY) {
            /* a new message is ready */
            if(cp->rx == (-1)) {
                /* record that the message is ready to be read */
                cp->rx = cp->rxlim;
            }

            /* advance the read limit pointer */
            if(flags & DPMSG_WRAP) {
                cp->rxlim = cp->rxbase;
            } else {
                cp->rxlim += DPM_MSGSIZE;
            }

	    /* process one message at a time if using readaddr protocol */
	    if((cp->chanflags & CF_TXVALID) == 0) {
		break;
	    }
        } else {
            /* no more messages */
            break;
        }
    }
}
#endif	/* ! RAWRITES_ONLY */


/* configure the dual-port ram to be used in a read-only or a read-write
 * fashion. */
int
config_dpram(base, width, index, flags, numaccesses)
uInt32 base;
int width, index, flags, numaccesses;
{
    DpChannel *cp = &channels[0];

    /* set the address of dualport ram in pod 0 */
    cp->dpbase = base;
    cp->width = width;
    cp->index = index;

    /* initialize the buffer system */
    cp->txbuf.bufsize = 0;
    cp->txbuf.index = 0;
    cp->rxbuf.bufsize = 0;
    cp->rxbuf.index = (-1);
    cp->numaccess = numaccesses;

    /* initialize the ack counter */
    cp->rxackval = 0;

    /* use the flags to set up */
    switch(flags) {
#ifdef READONLY_TARGET
    case DPF_READONLY_TGT:
        /* we can only read dualport */
        cp->chanflags = CF_RXVALID | CF_NOWAITIO;

        /* start the receive area after the read-address area, and the
         * status/control area */
        cp->rxbase = cp->rxlim = RA_MSGBASE;
        cp->rx = (-1);

	/* to avoid runtime divides, make this a case stmt */
	switch(cp->numaccess) {
	  case 4:
	    cp->oobthresh = (READADDR_SIZE / 4) - RA_MAX_INDEX;
	    break;
	  case 2:
	    cp->oobthresh = (READADDR_SIZE / 2) - RA_MAX_INDEX;
	    break;
	  case 1:
	  default:
	    cp->oobthresh = (READADDR_SIZE / 1) - RA_MAX_INDEX;
	    break;
	}
	/* Original code with the divide
	cp->oobthresh = (READADDR_SIZE / cp->numaccess) - RA_MAX_INDEX; */

        /* all done */
        break;
#endif	/* READONLY_TARGET */

#ifdef READWRITE_TARGET
    case DPF_ONECHANNEL:
        /* we can both read and write dualport */
        cp->chanflags = CF_TXVALID | CF_RXVALID | CF_NOWAITIO;

        /* start the receive area after the status/control area */
        cp->rxbase = cp->rxlim = RW_MSGBASE;
        cp->rx = (-1);

        /* start the transmit area after the receive area */
        cp->txbase = cp->tx = (RW_REC_MSGS + 1) * DPM_MSGSIZE;
	cp->oobthresh = 0;

        /* all done */
        break;
#endif	/* READWRITE_TARGET */
    
    default:
        return(-1);
        break;
    }

    return(0);
}



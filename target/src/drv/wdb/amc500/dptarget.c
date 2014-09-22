/*
**  File:     dptarget.c
**  Version:  1.0.1
**
**  Description: Target-side driver functions for NR5xx
**
**      Copyright (c) 1996 Applied Microsystems Corporation
**                          All Rights Reserved
**
**  Modification History:
**       10/04/96, MPH...Created from nrdrive.c
**       01/20/97, MPH...Fixed nr_ReadBuf and nr_ProcessMsgs for PPC603
**       02/04/97, MS....fixed numerous compiler warnings
*/
#include "wdb/amc500/dpconfig.h"
#include "wdb/amc500/dptarget.h"
#include "wdb/amc500/dualport.h"

/* Local definitions */
#define DEBUG_ON False  /* Optional: Include nr_TestComm() if True */

#ifndef	NULL
#define NULL  0
#endif

/* Private routines */
STATIC void   nr_WriteInt     ( uInt16, uInt32, uInt16 );
STATIC uInt16 nr_ReadInt      ( uInt16, uInt32 );
STATIC void   nr_WriteBuf     ( uInt16, uChar*, uInt32, uInt16 );
STATIC void   nr_ReadBuf      ( uInt16, uInt32, volatile uChar*, Int16 );
STATIC Int16  nr_ProcessMsgs  ( uInt16 );
STATIC Int16  nr_PutOOBMsg    ( uInt16, uInt16, char*, uInt16 );
STATIC void   nr_Wait         ( DpChannel* );
STATIC void   nr_WaitEnd      ( void );   

/* Global data */
STATIC DpChannel channels[DP_MAXCHANNELS];
STATIC volatile  uChar RR_dummy; /* See dptarget.h for explanation */
STATIC volatile  uChar dummy;  
STATIC uChar     wait_ftn_buf[MAX_WAIT_FTN_SIZE]; /* For RAM-based wait ftn */
STATIC Int16     emoffonwrite_set;

/*
**     Function:  nr_ReadByte
**  Description:  Reads a byte from DP RAM
**         Note:  With caching, we need to use a function
**                Without caching, we can use a (faster) macro
**      Warning:  Assumes 'cp' is valid
**
**  Parameters:
**    cp          pointer-to-channel to read from
**    addr        address for read
**
**  Returns:
**     uChar from DP RAM
*/
#if( nr_HasCache == True )
uChar 
nr_ReadByte(DpChannel *cp, uInt32 addr)
{
  register uChar ch;

  nr_DataCacheOff();
  ch = NR_READ_BYTE( (cp), (addr) );
  nr_DataCacheOn();
  return ch;
}
#else /* nr_HasCache == True */
#define nr_ReadByte(cp,addr) NR_READ_BYTE( (cp), (addr) )
#endif /* nr_HasCache == True */

/*
**     Function:  nr_WriteByte
**  Description:  Writes a byte to DP RAM
**         Note:  With caching, we need to use a function
**                Without caching, we can use a (faster) macro
**      Warning:  Assumes 'cp' is valid
**
**  Parameters:
**    cp          pointer-to-channel to write to
**    addr        address for write
**
**  Returns:
**     note
*/
#if( nr_HasCache == True )
void
nr_WriteByte(DpChannel *cp, uInt32 addr, uInt16 val)
{
  nr_DataCacheOff();
  NR_WRITE_BYTE( (cp), (addr), (val) );
  nr_DataCacheOn();
}
#else /* nr_HasCache == True */
#define nr_WriteByte\
(cp, addr, val) { NR_WRITE_BYTE((cp), (uInt32)(addr), (uInt16)(val)); }
#endif /* nr_HasCache == True */

/*
**  Macro: nr_InROM
**  Description: Determines if an address is w/in NR emulation RAM
**  Note: ROMSTART and ROMEND are defined in dpconfig.h
**  Evaluates to TRUE if address is within NetROM overlay, otherwise FALSE
*/
#define nr_InROM(adr) ((adr) >= (uInt32)ROMSTART && (adr) < (uInt32)ROMEND)


/*
**  Function: nr_ConfigDP
**  Description: Configure dualport RAM communication structures
**
**  Parameters:
**    base        First address of dualport RAM 
**    width       Width of ROM(s) emulated: 1-8 bits 2-16 bits 4-32 bits
**    index       Location of pod0 within podgroup
**
**  Returns:
**   Err_BadLength   If MAX_WAIT_FTN_SIZE is too small to hold nr_Wait()
**   Err_NoError     Otherwise
*/
Int16
nr_ConfigDP( uInt32 base, Int16 width, Int16 index )
{
    DpChannel *cp;
    Int16 chan;
    uInt32 tmp, pWait, wait_ftn_size; 

    /* Calc size of nr_Wait for copy to RAM...will it fit? */
    /* 'wait_ftn_size+3' because of 32bit align code, below */
    wait_ftn_size = (uInt32)((uInt32)nr_WaitEnd - (uInt32)nr_Wait);
    if( wait_ftn_size+3 > MAX_WAIT_FTN_SIZE )
      return Err_BadLength;
    if( wait_ftn_size < 10 )  /* Sanity check */
      return Err_BadLength;

    /* Copy nr_Wait routine to RAM */
    pWait = (uInt32)wait_ftn_buf;
    tmp = (uInt32) wait_ftn_buf & 0x03; /* force 32bit align */
    if(tmp != 0)  pWait += (4 - tmp);
    memcpy( (uChar*)pWait, (uChar*)nr_Wait, wait_ftn_size );

    /* Load channel structs */
    for( chan = 0; chan < DP_MAXCHANNELS; chan++ ) {

      /* get a pointer to the channel */
      cp = &channels[chan];

      /* set the addresses for dualport ram */
      cp->dpbase = base + chan * DP_CHAN_SIZE * width;
      cp->dpbase_plus_index = cp->dpbase + index;  /* Time saver const */
      cp->width = width;
      cp->index = index;

      /* rr_data_offset includes "index" to speed up RR_xxx macros */
      /* For writes to Pods0-2, rr_data_offset will cause us to read */
      /* from the next Pod.  For pod3, we'll read from pod0 area */
      /* This is to avoid reading addresses that the hardware is */
      /* latching for the RR protocol. */
      if( 3 == chan ) {
	cp->rr_enable  = base + index + RR_ENABLE_ADR  * width;
	cp->rr_disable = base + index + RR_DISABLE_ADR * width;
      }
      else {
	cp->rr_enable  =  cp->dpbase 
                       + index + RR_ENABLE_ADR  * width;
	cp->rr_disable =  cp->dpbase + DP_CHAN_SIZE * width
                       + index + RR_DISABLE_ADR * width;
      }

      /* initialize the buffer system */
      cp->txbuf.bufsize = 0;
      cp->txbuf.index = 0;
      cp->rxbuf.bufsize = 0;
      cp->rxbuf.index = (-1);
      /*      cp->numaccess = numaccesses; */

      /* initialize the ack counter */
      cp->rxackval = 0;

      /* we can both read and write dualport */
      cp->chanflags = CF_TXVALID | CF_RXVALID | CF_NOWAITIO;

      /* start the receive area after the status/control area */
      cp->rxbase = cp->rxlim = DP_MSGBASE;
      cp->rx = (-1);

      /* start the transmit area after the receive area */
      cp->txbase = cp->tx = (DP_REC_MSGS + 1) * DPM_MSGSIZE;
      cp->oobthresh = 0; /* xxx */

      /* Points to RAM-based wait ftn; used w/ nr_SetMem() */
      cp->wait_nr_done_ptr = (void (*)()) pWait;
    } 
    emoffonwrite_set = True;  /* Set emulate off on write */

    /* This function will hang on the following line until the 
    ** communication channel is up.  If you find that the code
    ** is hanging here, either 1) NetROM is not configured correctly for
    ** this debug channel; 2) config_dpram() was called with the wrong
    ** parameters; or 3) the telnet session is not connecting to the 
    ** NetROM debug port.
    */
    while( !nr_ChanReady(chan) ) {}

    /* Request that NetROM synchronize comm buffers with the target */
    nr_Resync(0);

  return Err_NoError;
}

/*
**  Function: nr_SetBlockIO
**  Description:  This ftn affects nr_Getch(), nr_Putch() and nr_GetMsg()
**                val == 0 --> non-blocking I/O
**                val != 0 --> blocking I/O
**
**  Parameters:
**    chan       channel to configure, 0-3
**     val       0 for non-blocking, non-zero for blocking
**
**  Returns:
**   Err_BadChan   if the chan is invalid (0,1,2,3 are valid on NR5xx)
**   Err_NoError   otherwise
*/
Int16
nr_SetBlockIO( uInt16 chan, uInt16 val )
{
    register DpChannel *cp;

    if( chan >= DP_MAXCHANNELS )  /* Invalid channel! */
      return Err_BadChan;

    /* get a pointer to the channel */
    cp = &channels[chan];

    if(val == 0) {
        cp->chanflags |= CF_NOWAITIO;
    } else {
        cp->chanflags &= ~CF_NOWAITIO;
    }
    return Err_NoError;
}

/*
**  Function: nr_ChanReady
**  Description:  Test to see if a chan is ready to transmit & receive
**
**  Parameters:
**    chan       channel to test, 0-3
**
**  Returns:
**   Err_BadChan   if chan is invalid (0,1,2,3 are valid on NR5xx)
**   True          if the channel is ready
**   False         if the channel is not ready
*/
Int16
nr_ChanReady( uInt16 chan )
{
  uChar val;
  register DpChannel *cp;

  if( chan >= DP_MAXCHANNELS )  /* Invalid channel! */
    return Err_BadChan;

  /* get a pointer to the channel */
  cp = &channels[chan];

  /* check that the RX channel is active */
  val = nr_ReadByte(cp, DP_RX);
  if(val != nr_ReadByte(cp, DP_RX)) {
    return False;
  } else if(val != 1) {
    return False;
  }

  /* check that the TX channel is active */
  val = nr_ReadByte(cp, DP_TX);
  if(val != nr_ReadByte(cp, DP_TX)){
    return False;
  } else if(val != 1) {
    return False;
  }

  /* Channel is ready */
  return True;
}

/*
**  Function: nr_Poll
**  Description: check if a character is waiting at the channel
**
**  Parameters:
**    chan       channel to poll, 0-3
**
**  Returns:
**    True          if there is a character waiting
**    False         if there is NOT a character waiting
**    Err_BadChan   if the chan is invalid (0,1,2,3 are valid on NR5xx)
*/
Int16
nr_Poll( uInt16 chan )
{
    DpChannel *cp;
    BufIo *bp;
    Int16 retval;

    if( chan >= DP_MAXCHANNELS )  /* Invalid channel! */
      return Err_BadChan;

    /* get a pointer to the channel */
    cp = &channels[chan];
    bp = &cp->rxbuf;

    /* see if there is a character in this buffer */
    if(bp->index < 0) {
      /* poll for new buffers */
      nr_ProcessMsgs(chan);

      /* no, check for a new buffer arriving */
      if(cp->rx !=  (-1) ) {
	retval = True;
      } else {
	retval = False;
      }
    } else {
      retval = True;
    }
    return(retval);
}

/*
**  Function: nr_Getch
**  Description: Gets one char from channel
**
**  Parameters:
**    chan       channel to read from, 0-3
**
**  Returns:
**   Err_WouldBlock   if channel is non-blocking and there is no char
**   Err_BadChan      if the chan is invalid
**   char             otherwise
*/
Int16
nr_Getch( uInt16 chan )
{
    DpChannel *cp;
    BufIo *bp;
    Int16 ch;

    if( chan >= DP_MAXCHANNELS )  /* Invalid channel! */
      return Err_BadChan;

    /* get a pointer to the channel */
    cp = &channels[chan];
    bp = &cp->rxbuf;

    /* see if there is a character in this buffer */
    if(bp->index < 0) {
        /* wait for the buffer to become valid */
        while(cp->rx ==  (-1) ) {
            /* poll for new buffers */
            nr_ProcessMsgs(chan);

	    /* did we get a message? */
	    if(cp->rx ==  (-1) ) {
		if(cp->chanflags & CF_NOWAITIO) return Err_WouldBlock;
		nr_YieldCPU();
	    }
        }

        /* make sure that the message is ready by reading the flags byte 
	 * of the next receive msg.  */
        bp->flags = nr_ReadInt(chan, cp->rx + DPM_FLAGS);

	/* Re-read the flags because the NetROM may have been modifying 
	 * them while we read them. */
        if(bp->flags != nr_ReadInt(chan, cp->rx + DPM_FLAGS)) {
            /* read failed on the verify, NetROM must be writing */
            bp->flags = nr_ReadInt(chan, cp->rx + DPM_FLAGS);
        }
        if((bp->flags & DPMSG_READY) == 0) {
            return(Err_WouldBlock);
        }

        /* set up the i/o buffer for the message */
        bp->bufsize = nr_ReadInt(chan, cp->rx + DPM_SIZE);


        if(bp->bufsize > DP_DATA_SIZE) {
            bp->bufsize = DP_DATA_SIZE;
        }

        nr_ReadBuf( chan, 
		    cp->rx + DPM_DATA, 
		    bp->buf, 
		    (Int16)(bp->bufsize) );
        bp->index = 0;

        /* return the buffer */
        nr_WriteInt(chan,cp->rx+DPM_FLAGS,(uInt16)bp->flags & ~DPMSG_READY);

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
    ch = (Int16)bp->buf[bp->index++];

    /* check whether we finished the buffer */
    if(bp->index == bp->bufsize) {
        /* invalidate the buffer */
        bp->index = (-1);
    }

    return(ch);
}

/*
**  Function: nr_Putch
**  Description: send one character via dp protocol
**
**  Parameters:
**    chan       channel to write to, 0-3
**
**  Returns:
**    Err_NoError      if the char was sent successfully
**    Err_WouldBlock   if we can't send the char
**    Err_BadChan      if the chan is invalid
*/
Int16
nr_Putch( uInt16 chan, char ch )
{
    DpChannel *cp;
    BufIo *bp;

    if( chan >= DP_MAXCHANNELS )  /* Invalid channel! */
      return Err_BadChan;

    /* get a pointer to the channel */
    cp = &channels[chan];
    bp = &cp->txbuf;

    /* if the current tx channel is owned by the target, wait for it */
    bp->flags = nr_ReadInt(chan, cp->tx + DPM_FLAGS);
    if(bp->flags & DPMSG_READY) {
        /* wait for the buffer */
        while(1) {
            bp->flags = nr_ReadInt(chan, cp->tx + DPM_FLAGS);
            if(bp->flags != nr_ReadInt(chan, cp->tx + DPM_FLAGS)) {
                /* read failed on the verify, NetROM must be writing */
                continue;
            }
            if((bp->flags & DPMSG_READY) == 0) break;
            if(cp->chanflags & CF_NOWAITIO) return( Err_WouldBlock );
            nr_YieldCPU();
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
        nr_FlushTX(chan);
    }
    return Err_NoError;
}

/*
**  Function: nr_FlushTX
**  Description: Transmits any chars pending in this channel
**
**  Parameters:
**    chan       channel to flush, 0-3
**
**  Returns:
**   Err_NoError   if successful
**   Err_BadChan   if the chan is invalid
*/
Int16
nr_FlushTX( uInt16 chan )
{
    DpChannel *cp;
    BufIo *bp;

    if( chan >= DP_MAXCHANNELS )  /* Invalid channel! */
      return Err_BadChan;

    /* get a pointer to the channel */
    cp = &channels[chan];
    bp = &cp->txbuf;

    /* if we don't own the message or there's nothing in it, just return */
    bp->flags = nr_ReadInt(chan, cp->tx + DPM_FLAGS);
    if((bp->flags & DPMSG_READY) != 0 || bp->index == 0) {
        return Err_NoError;
    }

    /* send the buffer */
    nr_WriteInt(chan, cp->tx + DPM_SIZE, (uInt16)bp->bufsize);
    nr_WriteBuf(chan, bp->buf, cp->tx + DPM_DATA, (Int16) bp->bufsize);
    bp->flags &= DPMSG_WRAP;
    nr_WriteInt(chan, cp->tx + DPM_FLAGS,
        (uInt16)bp->flags | (DPMSG_READY|DPMSG_START|DPMSG_END));

    /* notify NetROM that the buffer is ready */
    dummy = nr_ReadByte(cp, DP_MRI);

    /* advance the msg pointer */
    if(bp->flags & DPMSG_WRAP) {
        cp->tx = cp->txbase;
    } else {
        cp->tx += DPM_MSGSIZE;
    }

    /* clear the buffer structure */
    bp->index = 0;
    bp->bufsize = 0;
    return Err_NoError;
}

/*
**  Function: nr_GetMsg
**  Description: This routine reads message from dualport ram.
**      It returns different statuses, depending on whether or not
**      message data is present, or the message overflows the buffer. 
**      The number of bytes read into the  message buffer is changed
**      by side effect. 
** 
**  Parameters:
**    chan        channel to read from, 0-3
**    buf         buffer to read into
**    len         number of bytes expected, or max bytes for this buffer
**    bytesread   number of bytes actually received (nr_GetMsg will set this)
**
**  Returns:
**   Err_BadChan           if the chan is invalid
**   GM_NODATA             only returned when not in blocking I/O mode
**   GM_MSGCOMPLETE
**   GM_NOTDONE
**   GM_MSGOVERFLOW
*/
Int16
nr_GetMsg( uInt16 chan, char* buf, uInt16 len, uInt16* bytesread )
{
    register DpChannel *cp;
    Int16 done = 0;
    Int16 nbytes = 0;
    register uInt16 flags, size;
#ifdef VETHER
    Int16 msg_len = 0;
#endif

    if( chan >= DP_MAXCHANNELS )  /* Invalid channel! */
      return Err_BadChan;

    /* get a pointer to the channel */
    cp = &channels[chan];

    /* look for the whole message */
    while(done == 0) {
        /* wait for the buffer to become valid */
        while(cp->rx ==  (-1) ) {
            /* poll for new buffers */
            nr_ProcessMsgs(chan);

	    /* did we get a message? */
	    if(cp->rx ==  (-1) ) {

#ifdef VETHER
		if((cp->chanflags & CF_NOWAITIO) || (nbytes == 0)) {
#else
		if(cp->chanflags & CF_NOWAITIO) {
#endif
		    *bytesread = nbytes;
		    return(nbytes == 0 ? GM_NODATA : GM_NOTDONE);
		}
		nr_YieldCPU();
	    }
        }

	/* read the status and size of the new message block */
        flags = nr_ReadInt(chan, cp->rx + DPM_FLAGS);
        size =  nr_ReadInt(chan, cp->rx + DPM_SIZE);

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
        nr_ReadBuf(chan, cp->rx + DPM_DATA, (uChar*)buf, (Int16)size);

        /* return the buffer */
        nr_WriteInt(chan, cp->rx + DPM_FLAGS, flags & ~DPMSG_READY);

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

/*
**  Function: nr_PutMsg
**  Description: send a message to NetROM
**
**  Parameters:
**    chan       channel to write to, 0-3
**    buf        buffer to xmit
**    len        length of buffer to xmit
**
**  Returns:
**   Err_NoError    if the message was sent successfully
**   Err_NotReady   if NetROM is not ready to process messages
**   Err_BadChan    if the chan is invalid
*/
Int16
nr_PutMsg( uInt16 chan, char *buf, uInt16 len )
{
    register DpChannel *cp;
    register Int16 num_sent = 0;
    uInt16 size, num_left = len, ovf_size;
    register uInt16 flags;

    if( chan >= DP_MAXCHANNELS )  /* Invalid channel! */
      return Err_BadChan;

    if( !nr_ChanReady(chan) )   /* NetROM isn't ready to process msgs */
      return Err_NotReady;

    /* get a pointer to the channel */
    cp = &channels[chan];

    while(num_sent < len) {
      /* wait for the buffer */
      while(1) {
	flags = nr_ReadInt(chan, cp->tx + DPM_FLAGS);

	if(flags != nr_ReadInt(chan, cp->tx + DPM_FLAGS)) {
	  /* read failed on the verify, NetROM must be writing */
	  continue;
	}
	if((flags & DPMSG_READY) == 0) 
	  break;
	else
	  {
	  nr_YieldCPU(); 
	  }
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
	nr_WriteInt(chan, cp->tx + DPM_SIZE, (len << 6) | size);
      } else
	nr_WriteInt(chan, cp->tx + DPM_SIZE, size); 

      nr_WriteBuf(chan, (uChar*)buf, cp->tx + DPM_DATA, size);

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

      nr_WriteInt(chan, cp->tx + DPM_FLAGS, flags);

      /* notify NetROM that the buffer is ready */
      dummy = nr_ReadByte(cp, DP_MRI);

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
    return Err_NoError;
}

/*
**  Function: nr_Resync
**  Description: Requests that NetROM clear out TX & RX buffers for all chans
**               Blocks until NetROM is finished
**      Note: nr_Resync must be paired with nr_ConfigDP; if not, 
**            NetROM and the target will get out of sync
**
**  Parameters:
**    chan       channel to use for resync
**
**  Returns:
**    Err_BadChan     if channel is not in range 0 <= chan < MAXCHANNELS
**    Err_NotReady    if NetROM is not ready to process requests
**    Err_NoError     otherwise
*/
Int16
nr_Resync( uInt16 chan )
{
  register DpChannel *cp;

  if( chan >= DP_MAXCHANNELS )  /* Invalid channel! */
    return Err_BadChan;

  if( !nr_ChanReady(chan) )   /* Avoid hang if NetROM is not ready */
    return Err_NotReady;

  cp = &channels[chan];

  nr_WriteByte( cp, DP_NR_DONE, False ); /* Force flag to False */

  /* Request that NetROM resync the chans */
  do{
    nr_WriteByte(cp, DP_OOBFLAG, OOBFLAG_RESYNC );
  } while( nr_ReadByte(cp, DP_OOBFLAG) != OOBFLAG_RESYNC );

  /* Notify NetROM that the it has something to read */
  dummy = nr_ReadByte(cp, DP_MRI);

  /* Block until NetROM finishes the resync */
  /* If your code hangs here, NetROM is not resyncing when asked to */
  while( False == nr_ReadByte(cp, DP_NR_DONE) ) {}
  return Err_NoError;
}

/*
**  Function: nr_Reset
**  Description: Requests that NetROM reset the target
**
**  Parameters:
**    chan       channel used for reset message 
**
**  Returns:
**   Err_NoError     if the OOB msg was sent successfully
**   Err_NotReady    if NetROM is not ready to process messages
**   Err_BadChan     if the chan is invalid (0,1,2,3 are valid on NR5xx)
*/
Int16
nr_Reset( uInt16 chan )
{
  return nr_PutOOBMsg(chan, DP_OOB_RESET, NULL, 0);
}

/*
**  Function: nr_IntAck
**  Description: Acknowledges a previous interrupt to the target
**               
**  Parameters:
**    chan       channel to acknowledge interrupt on
**
**  Returns:
**   Err_NoError     if the OOB msg was sent successfully
**   Err_NotReady    if NetROM is not ready to process messages
**   Err_BadChan     if the chan is invalid (0,1,2,3 are valid on NR5xx)
*/
Int16
nr_IntAck( uInt16 chan )
{
  return nr_PutOOBMsg(chan, DP_OOB_INTACK, NULL, 0);
}


/*
**  Function: nr_Cputs
**  Description: Puts a string to the NetROM console; the
**               message won't be transmitted to the host
**
**  Parameters:
**    chan       channel to write message through
**    len        length of message, less than 56
**
**  Returns:
**   Err_NoError     if the OOB msg was sent successfully
**   Err_NotReady    if NetROM is not ready to process messages
**   Err_BadChan     if the chan is invalid (0,1,2,3 are valid on NR5xx)
**   Err_BadLength   if the "size" of the buffer is too big
**   Err_BadCommand  if the command is invalid
*/
Int16
nr_Cputs( uInt16 chan, char *buf, uInt16 len )
{
  char localbuf[60];
  if(len > 55) return Err_BadLength;

  /* Ensure zero-termination */
  memcpy( (uChar*)localbuf, (uChar*)buf, len );
  localbuf[len] = NULL;
  return nr_PutOOBMsg(chan, DP_OOB_CPUTS, localbuf, len+1);
}

#if (READONLY_TARGET == True )
/*
**  Function: nr_SetMem
**  Description: Requests that NetROM set a block of overlay memory
**  Note: 1. target must run from RAM while NetROM sets the memory
**        2. Total msg size = len+6 = len+sizeof(addr)+sizeof(len)
**
**  Parameters:
**    chan       channel to write to
**    addr       start address for write
**    buf        data to write
**    len        length of buf; must be less than 54 bytes
**
**  Returns:
**   Err_NoError      if successful
**   Err_BadLength    if length is invalid; max block size is 54 bytes
**   Err_BadChan      if the chan is invalid
**   Err_BadAddress   if the address is invalid, i.e. outside ROM
*/
Int16
nr_SetMem( uInt16 chan, uInt32 addr, char* buf, uInt16 len )
{
  DpSetMemBuf setmem_buf;
  register DpChannel *cp;
  uInt16 size;

  if( chan >= DP_MAXCHANNELS )  /* Invalid channel! */
    return Err_BadChan;

  if( len == 0 || len > DP_DATA_SIZE-6 )  /* Invalid length! */
    return Err_BadLength;

  if( !nr_InROM(addr) )  /* Invalid address! */
    return Err_BadAddress;

  addr -= (uInt32)ROMSTART;   /* NetROM references addresses from ROMSTART */

  cp = &channels[chan]; /* Get a pointer to the channel */

  setmem_buf.addr = swap32(addr); /* Fix big/little -endian */

  memcpy( setmem_buf.data, (uChar*)buf, len );
  size = len + 4; /* buf length + sizeof(addr) */

  if(emoffonwrite_set == True) {
    nr_PutOOBMsg(chan, DP_OOB_EMOFFONWRITE, NULL, 0);
    emoffonwrite_set = False;
  }

  /* The following nr_PutOOBMsg call should be atomic 
  ** If it is not, make sure that you don't write other messages 
  ** to this channel after you've begun the SetMem sequence! 
  */
  nr_InterruptsOFF();
  nr_PutOOBMsg(chan, DP_OOB_SETMEM, (char*)&setmem_buf, size);
  nr_InterruptsON();
  return Err_NoError;
}

#else /* READONLY_TARGET != True */
/*
**  Function: nr_SetMem
**  Description: Sets a block of overlay memory, up to 54 bytes
**               This target can write its own memory.
**               length, channel, and address are checked to
**               keep this routine consisten with the read-only version
**
**  Parameters:
**    chan       channel to write to
**    addr       start address for write
**    buf        data to write
**    len        length of buf; must be less than 54 bytes
**
**  Returns:
**   Err_NoError      if successful
**   Err_BadLength    if length is invalid
**   Err_BadChan      if the chan is invalid
**   Err_BadAddress   if the address is invalid, i.e. outside ROM
*/
Int16
nr_SetMem( uInt16 chan, uInt32 addr, char *buf, uInt16 len )
{
  if( !nr_InROM(addr) )  /* Invalid address! */
    return Err_BadAddress;

  if( chan >= DP_MAXCHANNELS )  /* Invalid channel! */
    return Err_BadChan;

  if( len == 0 || len > DP_DATA_SIZE-6 )  /* Invalid length! */
    return Err_BadLength;

  memcpy( (uChar*)addr, (uChar*)buf, (long)len );
}
#endif /* READONLY_TARGET == True */


/*
**  Function: nr_SetEmOffOnWrite
**  Parameters:
**
**  Description: After calling this, NetROM will set emulation off
**               before writing to PodMem.  See nr_SetMem().
**               The default, set in nr_ConfigDP(), is
**               emoffonwrite_set = False.
**  Returns:
*/
void
nr_SetEmOffOnWrite(void)
{
  emoffonwrite_set = True; 
}


/*
**  Function: nr_WriteInt
**  Description: Writes a uInt16 to dp RAM
**    Warning:  Assumes 'chan' is valid!
**
**  Parameters:
**    chan       channel to write to
**    addr       start address for write
**    val        data to write
**
**  Returns:
*/
STATIC void
nr_WriteInt( uInt16 chan, uInt32 addr, register uInt16 val )
{	
  register DpChannel *cp = &channels[chan];
  register uInt16 valLSB = (uInt16)val & 0xFF;
  register uInt16 valMSB = (uInt16)(val >> 8) & 0xFF;

  /* Write in network byte order, LSB first 
  ** This avoids writing the message-ready bit before other flags,
  ** which could lead to garbled messages.  Make sure that the
  ** write succeeded--NetROM and target may have been in contention.
  */
  do  {
    nr_WriteByte(cp, addr + 1, valLSB );
    nr_WriteByte(cp, addr, valMSB );
  } while (val != nr_ReadInt(chan, addr) );
}


/*
**  Function: nr_ReadInt
**  Description: Reads a uInt16 from DP RAM
**  Warning: Assumes 'chan' is valid
**
**  Parameters:
**    chan       channel to read from
**    addr       start address for read
**
**  Returns:
**     Int16 from DP RAM
*/
STATIC uInt16
nr_ReadInt( uInt16 chan, uInt32 addr )
{
  register uInt16 val;
  register DpChannel *cp = &channels[chan];

  /* read the msb first, in case it contains flags */
  val = (uChar)nr_ReadByte(cp, addr);
  val <<= 8;
  val += (uChar)nr_ReadByte(cp, addr + 1);

  /* return, in host byte order */
  return(val);
}

/*
**  Function: nr_WriteBuf
**  Description: Copies bytes to dual-port RAM
**  Warning: Assumes 'chan' is valid
**
**  Parameters:
**    chan       channel to write to
**    src        data to write
**    addr       start address for write
**    len        length of buf
**
**  Returns:
*/
STATIC void
nr_WriteBuf( uInt16 chan, uChar *src, uInt32 addr, uInt16 len )
{
    register Int16 i;
    register DpChannel *cp = &channels[chan];
#if (nr_HasCache != True && READONLY_TARGET == False)
    register Int16 inc;
    register volatile uChar *dst;
#endif

#if (nr_HasCache == True && READONLY_TARGET == False) /* RW tgt with cache */
    for(i = 0; i < len; i++) {
      nr_WriteByte(cp, addr++, *src);
      src++;
    }
#elif (READONLY_TARGET == True)  /* Read only tgt, either way */
    RR_ENABLE(cp, addr);            /* Enable RR & latch start addr */
    for(i = 0; i < len; i++) {
      RR_WRITE_BYTE(cp, *src);     /* Write & autoincrement dest addr */
      src++;
    }
    RR_DISABLE(cp);
#else                            /* RW tgt without cache */
    inc = cp->width;
    dst = (volatile uChar *)(cp->dpbase + (cp->width * addr) + cp->index);
    for(i = 0; i < len; i++) {
        *dst = *src;
        dst += inc;
        src++;
    }
#endif
}


/*
**  Function: nr_ReadBuf
**  Description: Copies bytes from dualport RAM
**
**  Parameters:
**    chan       channel to read from
**    addr       start address for read
**    buf        buffer to read into
**    len        length of buf
**
**  Returns:
*/
STATIC void
nr_ReadBuf( uInt16 chan, uInt32 addr, volatile uChar *buf, Int16 len )
{
    register Int16 i;
    volatile uChar ch;
    register DpChannel *cp = &channels[chan];

    for(i = 0; i < len; i++, buf++, addr++) {
      *buf = (ch = nr_ReadByte(cp, addr));
    }
}

/*
**  Function: nr_PutOOBMsg
**  Description: sends out-of-band message to NetROM
**
**  Parameters:
**    chan       channel to write to
**    cmd        command number, see dptarget.h for commands
**    buf        data, if any, required by this command
**    size       size of buf; must be less than 58 bytes
**
**  Returns:
**   Err_NoError     if the OOB msg was sent successfully
**   Err_NotReady    if NetROM is not ready to process messages
**   Err_BadChan     if the chan is invalid (0,1,2,3 are valid on NR5xx)
**   Err_BadLength   if the "size" of the buffer is too big
**   Err_BadCommand  if the command is invalid
*/
STATIC Int16
nr_PutOOBMsg( uInt16 chan, uInt16 cmd, register char *buf, uInt16 size )
{
  register DpChannel *cp;
  uInt16 flags;

  if( !nr_ChanReady(chan) )  /* NetROM not ready */
    return Err_NotReady;

  if( chan >= DP_MAXCHANNELS )  /* Invalid channel! */
    return Err_BadChan;

  if( size > DP_DATA_SIZE-2 )  /* Includes cmd and buf */
    return Err_BadLength;

  if( cmd > DP_OOB_MAXCMD ) /* Invalid command! */
    return Err_BadCommand;

  /* get a pointer to the channel */
  cp = &channels[chan];

  /* Wait for the buffer */
  while(1) {
    flags = nr_ReadInt(chan, cp->tx + DPM_FLAGS);
    
    if(flags != nr_ReadInt(chan, cp->tx + DPM_FLAGS)) {
      /* read failed on the verify, NetROM must be writing */
      continue;
    }
    if((flags & DPMSG_READY) == 0) 
      break;

    nr_YieldCPU(); 
  }

  /* Write size field to DP buf */
  nr_WriteInt(chan, cp->tx + DPM_SIZE, size+sizeof(cmd));

  /* Write out-of-band command number to DP buf */
  nr_WriteInt(chan, cp->tx + DPM_OOB_CMD, cmd);

  if( size > 0 )
    nr_WriteBuf(chan, (uChar*)buf, cp->tx + DPM_OOBDATA, size);

  /* Write flags for OOB msg */
  /* If doing SetMEM, do _not_ set Ready flag! */
  flags = (flags & DPMSG_WRAP) | DPMSG_OOB | DPMSG_START | DPMSG_END;
  if( DP_OOB_SETMEM != cmd )
    flags |= DPMSG_READY;
  nr_WriteInt(chan, cp->tx + DPM_FLAGS, flags);

  /* Notify NetROM that the buffer is ready, unless it's nr_SetMem */
  /* nr_SetMem will read DP_MRI from a routine in RAM */
  if( DP_OOB_SETMEM != cmd )
    dummy = nr_ReadByte(cp, DP_MRI);
  else
    (*(cp->wait_nr_done_ptr))(cp);    /* Wait in RAM until NR is finished */

  /* Advance the msg pointer */
  if(flags & DPMSG_WRAP) {
    cp->tx    = cp->txbase;
    cp->txovf = cp->txovfbase;
  } else {
    cp->tx    += DPM_MSGSIZE;
    cp->txovf += MAX_OVF_MSG_SIZE;
  }
  return Err_NoError;
}


/*
**  Function: nr_Wait
**  Note: 1. This ftn must run from RAM
**        2. nr_WaitEnd()   must be defined immediately after nr_Wait
**        3. (2) assumes that the compiler keeps ftns in order
**  Description: Finishes writing OOB_SetMem msg, waits for NR to
**               write memory, then exits
**
**  Parameters:
**    cp         Pointer to channel to use
**
**  Returns:
*/
STATIC void
nr_Wait( DpChannel *cp )
{
  uChar flagsMSB;

  /* Warning: cp is assumed to be valid  */

  do{
    nr_WriteByte( cp, DP_NR_DONE, False ); /* Force flag to False */
  } while( nr_ReadByte(cp, DP_NR_DONE) != False );

  /* Finish writing flags: write READY bit */
  /* Assumes Ready-bit is in MSB */
  flagsMSB = nr_ReadByte(cp, cp->tx + DPM_FLAGS); 
  flagsMSB |= (DPMSG_READY >> 8);              
  nr_WriteByte(cp, cp->tx + DPM_FLAGS, flagsMSB );

  dummy = nr_ReadByte(cp, DP_MRI); /* Ask NR to process messages */

  /* Wait for NetROM to finish the PodMem write 
  ** Note: when emulation is OFF, target will see 0xFF in 
  ** this location.  This is not a problem, because the target
  ** will hang in wait loop, below, until the flag == True.
  ** Flag will equal True after NetROM sets it True and turns
  ** emulation back on.
  **
  **   If your code is hanging here, NetROM is not completing
  **   its write to Pod memory
  */
  while( nr_ReadByte( cp, DP_NR_DONE ) != True ) {}
}

/*
**  Function: nr_WaitEnd
**  Description: This empty ftn marks the end of nr_Wait()
**  Note: wait_fnt_end() must be defined immediately after nr_Wait
**
**  Parameters:
**
**  Returns:
*/
STATIC void 
nr_WaitEnd( void )
{
}


/*
**  Function: nr_ProcessMsgs
**  Description: Processes any messages received from NetROM
**
**  Parameters:
**    chan         Channel to process
**
**  Returns:
**   Err_BadChan    if the chan is invalid
**   Err_NoError    otherwise
*/
STATIC Int16 
nr_ProcessMsgs( uInt16 chan )
{
    register DpChannel *cp;
    register uInt16 flags;

    if( chan >= DP_MAXCHANNELS )  /* Invalid channel! */
      return Err_BadChan;

    /* get a pointer to the channel */
    cp = &channels[chan];

    /* a received message indicator */
    while(cp->rx != cp->rxlim) {
        flags = nr_ReadInt(chan, cp->rxlim + DPM_FLAGS);
        if(flags != nr_ReadInt(chan, cp->rxlim + DPM_FLAGS)) {
                /* NetROM must be writing, try again */
                continue;
        }

        if(flags & DPMSG_READY) {

	  /* a new message is ready */
	  if(cp->rx ==  (-1) ) {
	    /* record that the message is ready to be read */
	    cp->rx = cp->rxlim;
	  }

	  /* advance the read limit pointer */
	  if(flags & DPMSG_WRAP) {
	    cp->rxlim = cp->rxbase;
	  } else {
	    cp->rxlim += DPM_MSGSIZE;
	  }
        } else {
	  /* no more messages */
	  break;
        }
    }
    return Err_NoError;
}


#if (DEBUG_ON == True)
/*
** Function:    nr_TestComm
** Description: Determines if NR Comm is working
** Notes:
** 1. Tests 1 through 6 test low-level reads & writes to dualport RAM
**    Before executing the tests, execute a 'fill 0 dpmem' and a 
**    'tgtreset' on NetROM.
**    After executing the tests, verify their success by typing
**    'di dpmem 0x50 0x80' at the NetROM prompt.  Your NetROM display 
**    should look like:
**  
**  0050  6e72 5f57 7269 7465 - 4279 7465 0000 0000    nr_WriteByte....
**  0060  6e72 5f52 6561 6442 - 7974 6500 0000 0000    nr_ReadByte.....
**  0070  6e72 5f57 7269 7465 - 496e 7420 0000 0000    nr_WriteInt ....
**  0080  0000 0000 0000 0000 - 0000 0000 0000 0000    ................
**  0090  6e72 5f52 6561 6449 - 6e74 2020 0000 0000    nr_ReadInt  ....
**  00a0  6e72 5f57 7269 7465 - 4275 6600 0000 0000    nr_WriteBuf.....
**  00b0  6e72 5f52 6561 6442 - 7566 2000 0000 0000    nr_ReadBuf .....
**
** 2. Tests 7 and higher test higher-level char and msg passing thru DP RAM
**    for these tests, you'll need to telnet to 
**    the NetROM debugport (1235 is the default)
**    Example: telnet netrom 1235
**    The name of the function tested should appear in the telnet window.
**
**  Parameters:
**    chan        Channel to test, 0-3
**    lastTest    See tests, below to select
**    endian      0 for Intel (little-endian), 1 for Motorola (big-endian)
**
**  Returns:
**    Err_BadChan  if chan is invalid 
**    Err_NoError  otherwise
*/
Int16 nr_TestComm( uInt16 chan, uInt16 lastTest, uInt16 endian )
{
  Int32 i,j;
  uChar ch;
  uChar testNum = 1;
  static char *wb_buf = "nr_WriteByte";
  static char *rb_buf = "nr_ReadByte";
  static char *wi_buf[2] = {"rnW_iretnI t", "nr_WriteInt "}; /* intel, mot */
  static char *ri_buf[2] = {"rnR_aeIdtn  ", "nr_ReadInt  "}; /* intel, mot */
  static char *wbuf = "nr_WriteBuf";
  static char *rbuf = "nr_ReadBuf ";
  static char *putch_buf = "\r\n\n\nnr_Putch\r\n";
  static  char putmsg_buf[1024] = "\r\nnr_PutMsg\r\n";
  char* getch_buf = "\r\nnr_Getch test\r\n";
  char* getmsg_buf = "\r\nnr_GetMsg test\r\n";
  char* query_buf = "Type a message, then press <ret> \r\n";
  char buf[81]; 
  uInt16 bytesread; 
  DpChannel *cp;
  BufIo *bp;

  if( chan >= DP_MAXCHANNELS )  /* Invalid channel! */
    return Err_BadChan;

  /* get a pointer to the channel */
  cp = &channels[chan];
  bp = &cp->txbuf;


  /* Test 1
  ** Test nr_ConfigDP params & nr_WriteByte
  ** Write 'nr_WriteByte ' to DPRAM, starting at DP_BASE + 0x50
  ** Assuming your hardware is correctly installed, and 
  ** this test fails, either you are calling nr_ConfigP with the
  ** wrong parameters, or your NetROM is not set up correctly
  */
  for(i=0; i<strlen(wb_buf); i++) {
    nr_WriteByte(cp, 0x50+i, wb_buf[i]); 
  } 
  if(testNum++ >= lastTest) return Err_NoError;


  /* Test 2
  ** Test nr_ReadByte macro
  ** Read nr_WriteByte, convert, write nr_ReadByte
  */
  for(i=0; i<strlen(rb_buf); i++ ) {
    ch = nr_ReadByte(cp, 0x50+i) + rb_buf[i] - wb_buf[i];
    nr_WriteByte(cp, 0x60+i, ch);
  }
  if(testNum++ >= lastTest) return Err_NoError;


  /* Test 3
  ** Test nr_WriteInt ftn
  ** Write 'nr_WriteInt'
  */
  for(i=0; i<strlen(wi_buf[endian]); i+=2) {
    nr_WriteInt(chan, 0x70+i, *(uInt16*)(&wi_buf[endian][i] ));
  }
  if(testNum++ >= lastTest) return Err_NoError;


  /* Test 4
  ** Test nr_ReadInt ftn 
  ** Read nr_WriteInt, convert, write as nr_ReadInt
  */
  for(i=0; i<strlen(wi_buf[endian]); i+=2) {
    j = nr_ReadInt(chan, 0x70+i)
      + *(uInt16*)(&ri_buf[endian][i])
      - *(uInt16*)(&wi_buf[endian][i]);
    nr_WriteInt(chan, 0x90+i, j );
  }
  if(testNum++ >= lastTest) return Err_NoError;


  /* Test 5
  ** Test nr_WriteBuf ftn 
  ** Write 'nr_WriteBuf'
  */
  nr_WriteBuf( chan, (uChar*)wbuf, 0xA0, strlen(wbuf) );
  if(testNum++ >= lastTest) return Err_NoError;


  /* Test 6
  ** Test nr_ReadBuf ftn 
  ** Read 'nr_WriteBuf', convert, then write 'nr_ReadBuf'
  */
  nr_ReadBuf( 0, 0xA0, (uChar*)buf, sizeof(buf) );

  for(i=0; i<strlen(rbuf); i++ )
    rbuf[i] = buf[i] + rbuf[i] - wbuf[i];
  nr_WriteBuf( 0, (uChar*)rbuf, 0xB0, strlen(rbuf) );

  if(testNum++ >= lastTest) return Err_NoError;

  /* Test 7
  ** Test nr_Putch ftn and nr_FlushTX ftn
  ** Send the chars 'n' 'r' '_' 'P' 'u' 't' 'c' 'h' to the NR debug port
  ** Do you see 'nr_Putch' on the debug window?
  */
  for( i=0; i<strlen(putch_buf); i++) 
    nr_Putch(chan, putch_buf[i]);
  nr_FlushTX(0);
  if(testNum++ >= lastTest) return Err_NoError;


  /* Test 8
  **  Test nr_PutMsg ftn
  ** Send the message "nr_PutMsg\r\n"
  */
  nr_PutMsg( chan, putmsg_buf, strlen(putmsg_buf) );
  if(testNum++ >= lastTest) return Err_NoError;


  /* Test 9
  ** Test nr_OOBMsg ftn via nr_Cputs
  ** You will see "nr_Cputs working" in the NetROM console window on NR5xx
  ** On NR4xx, OOB msgs pass thru like normal in-band msgs
  */
  nr_Cputs( chan, "nr_Cputs\r\n", 10 );
  if(testNum++ >= lastTest) return Err_NoError;


  /* Test A
  ** Test nr_GetMsg ftn
  ** Get user input, then echo.
  */
  nr_SetBlockIO(chan, True);
  nr_PutMsg(chan, getmsg_buf, strlen(getmsg_buf) );
  nr_PutMsg(chan, query_buf, strlen(query_buf) );
  nr_GetMsg(chan, buf, 80, &bytesread );
  nr_PutMsg(chan, buf, bytesread );
  if(testNum++ >= lastTest) return Err_NoError;


  /* Test B
  ** Test nr_Getch ftn
  ** Get user input, then echo.
  */
  nr_PutMsg(chan, getch_buf, strlen(getch_buf) );
  nr_PutMsg(chan, query_buf, strlen(query_buf) );
  do{
    ch = nr_Getch(chan);
    if(ch > 0)nr_Putch(chan, ch);
  }while( ch != '\n' );
  nr_FlushTX(chan); 
  if(testNum++ >= lastTest) return Err_NoError;


  /* Test C
  ** Test nr_SetMem ftn
  ** Write "SetMem"
  ** Note: Move address so that SetMem writes to regular podmem,
  **       NOT dualport RAM!  Don't stomp code or data!
  */
  nr_SetMem( 0, ROMSTART + 0x2000, "SetMem", 6 );
  if(testNum++ >= lastTest) return Err_NoError;

  /* Test D
  ** Stress test nr_PutMsg ftn
  ** Send 1k buffer 100 times
  */
  nr_PutMsg(0, "\r\n*** nr_PutMsg Stress Test ***\r\n\r\n", 36);
    for(j=0; j<=1023; j++) {
      if( (j%2) ) putmsg_buf[j] = ' ';
      else      putmsg_buf[j] = 0x08;  /* BS */
    }
  putmsg_buf[0] = ' '; putmsg_buf[1] = '[';  
  putmsg_buf[6] = ']'; putmsg_buf[7] = ' ';

  for(i=1; i <= 9000; i++) {
    int a,b,c,d;
    a = i/1000;
    b = (i - a*1000)/100;
    c = (i - a*1000 - b*100)/10;
    d = (i - a*1000 - b*100 - c*10);
    putmsg_buf[2] = '0' + a;
    putmsg_buf[3] = '0' + b;
    putmsg_buf[4] = '0' + c;
    putmsg_buf[5] = '0' + d;
    nr_PutMsg( chan, putmsg_buf, 1024);
  }
  return Err_NoError;

}
#else /* DEBUG_ON */
Int16 nr_TestComm( uInt16 chan, uInt16 lastTest, uInt16 endian )
{
  return -1;
}
#endif /* DEBUG_ON */

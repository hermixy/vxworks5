/* wdbSlipPktDrv.c - a serial line packetizer for the WDB agent */

/* Copyright 1994-2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01g,18oct01,jhw  Fixed documentation build errors.
01f,18sep01,jhw  Fixed compiler warnings from gnu -pedantic option.
		 Added Copyright.
01e,16jan98,dbt  modified overflow test in wdbCharRcv() (SPR #8625).
01d,14oct95,jdi  doc: cleanup.
01c,26sep95,ms   docs
01b,15jun95,ms	 updated for new serial device interface
01a,02dec94,ms   written.
*/

/*
DESCRIPTION
This is a lightweight SLIP driver that interfaces with the WDB agents
UDP/IP interpreter.  It is the lightweight equivalent of the VxWorks
SLIP netif driver, and uses the same protocol to assemble serial
characters into IP datagrams (namely the SLIP protocol).
SLIP is a simple protocol that uses four token characters
to delimit each packet:

    - FRAME_END (0300)
    - FRAME_ESC (0333)
    - FRAME_TRANS_END (0334)
    - FRAME_TRANS_ESC (0335)

The END character denotes the end of an IP packet.  The ESC character is used
with TRANS_END and TRANS_ESC to circumvent potential occurrences of END or ESC
within a packet.  If the END character is to be embedded, SLIP sends
"ESC TRANS_END" to avoid confusion between a SLIP-specific END and actual
data whose value is END.  If the ESC character is to be embedded, then
SLIP sends "ESC TRANS_ESC" to avoid confusion.  (Note that the SLIP ESC is
not the same as the ASCII ESC.)

On the receiving side of the connection, SLIP uses the opposite actions to
decode the SLIP packets.  Whenever an END character is received, SLIP
assumes a full packet has been received and sends on.

This driver has an MTU of 1006 bytes.  If the host is using a real SLIP
driver with a smaller MTU, then you will need to lower the definition
of WDB_MTU in configAll.h so that the host and target MTU match.
If you are not using a SLIP driver on the host, but instead are using
the target server's wdbserial backend to connect to the agent, then you do not
need to worry about incompatabilities between the host and target MTUs.

INTERNAL
The END character is also sent at the begining of a packet. Although
this is not part of the SLIP spec, it helps remove any "noise characters"
that may have accumulated.

Only a single input packet can arrive at a time. Once that packet has
arrived, the input buffer is marked "locked", and any further
characters received are dropped.  When the buffer is finally read
by the agent, then it is marked "unlocked" again.
*/

#include "string.h"
#include "errno.h"
#include "sioLib.h"
#include "intLib.h"
#include "wdb/wdbMbufLib.h"
#include "drv/wdb/wdbSlipPktDrv.h"


#if 0
/*
 * SLMTU is now a hard limit on input packet size.
 * SLMTU must be <= MCLBYTES - sizeof(struct ifnet *).
 */
#define SLMTU           1006    /* be compatible with BSD 4.3 SLIP */
#endif

/* SLIP protocol characters */

#define FRAME_END          (char)0300	/* Frame End */
#define FRAME_ESCAPE       (char)0333	/* Frame Esc */
#define TRANS_FRAME_END    (char)0334	/* transposed frame end */
#define TRANS_FRAME_ESCAPE (char)0335	/* transposed frame esc */

/* states of the input/ouput buffers (bitwise or'ed together) */

#define STATE_BUSY	1	/* I/O is being performed */
#define STATE_ESC	2	/* in the middle of a SLIP escape seq */
#define STATE_START	4	/* output just started */

/* some handy macros */

#define RESET_INPUT(pPktDev)	{pPktDev->inBufPtr = pPktDev->inBufBase; \
				 pPktDev->inputState = 0;}

/* forward declarations */

static STATUS	wdbCharRcv (void *pPktDev, char thisChar);
static STATUS	wdbCharTx  (void *pPktDev, char *thisChar);

static STATUS	wdbPktPoll (void *pPktDev);
static STATUS	wdbPktTx   (void *pPktDev, struct mbuf *pMbuf);
static void	wdbPktFree (void *pPktDev);
static STATUS	wdbPktModeSet (void *pPktDev, uint_t newMode);

static STATUS	mbufChainCharGet (struct mbuf *, int, char *);
static void	mbufChainFree	 (struct mbuf *);

/******************************************************************************
*
* wdbPktResetOutput - reset the output packet state machine
*/ 

static void wdbPktResetOutput
    (
    WDB_SLIP_PKT_DEV * pPktDev
    )
    {
    struct mbuf * pMbuf;

    pPktDev->outputIx	 = 0;
    pPktDev->outputState = 0;

    while (pPktDev->pMbuf != NULL)
	{
	pMbuf = pPktDev->pMbuf->m_act;
	mbufChainFree (pPktDev->pMbuf);
	pPktDev->pMbuf = pMbuf;
	}
    }

/******************************************************************************
*
* wdbSlipPktDevInit - initialize a SLIP packet device for a WDB agent
*
* This routine initializes a SLIP packet device on one of the BSP's
* serial channels.  It is typically called from usrWdb.c when the WDB
* agent's lightweight SLIP communication path is selected.
*
* RETURNS: N/A
*/

void wdbSlipPktDevInit
    (
    WDB_SLIP_PKT_DEV *pPktDev,	/* SLIP packetizer device */
    SIO_CHAN *	pSioChan,	/* underlying serial channel */
    void	(*stackRcv)()	/* callback when a packet arrives */
    )
    {
    pPktDev->wdbDrvIf.mode	= WDB_COMM_MODE_POLL | WDB_COMM_MODE_INT;
    pPktDev->wdbDrvIf.mtu	= SLMTU;
    pPktDev->wdbDrvIf.stackRcv	= stackRcv;

    pPktDev->wdbDrvIf.devId	= (WDB_SLIP_PKT_DEV *)pPktDev;
    pPktDev->wdbDrvIf.pollRtn	= wdbPktPoll;
    pPktDev->wdbDrvIf.pktTxRtn	 = wdbPktTx;
    pPktDev->wdbDrvIf.modeSetRtn = wdbPktModeSet;

    pPktDev->pSioChan		 = pSioChan;
    pPktDev->inBufEnd	= pPktDev->inBufBase + SLMTU;
    pPktDev->pMbuf	= 0;

    sioCallbackInstall (pSioChan, SIO_CALLBACK_GET_TX_CHAR,
			(FUNCPTR) wdbCharTx, (void *)pPktDev);
    sioCallbackInstall (pSioChan, SIO_CALLBACK_PUT_RCV_CHAR,
			(FUNCPTR) wdbCharRcv, (void *)pPktDev);

    wdbPktModeSet (pPktDev, WDB_COMM_MODE_INT);
    }

/******************************************************************************
*
* wdbCharRcv - input a character
*
* This routine is called by the driver's input ISR (or poll routine) to
* give the next character for the input packet.
* If this is the "FRAME_END" character, then a whole packet has arrived,
* and we call the "pktRcvRtn" routine to hand off the packet.
*
* RETURNS: OK, or ERROR if input is locked or the input buffer overflows.
*/

static STATUS wdbCharRcv
    (
    void *	pDev,
    char 	thisChar
    )
    {
    WDB_SLIP_PKT_DEV	* pPktDev = (WDB_SLIP_PKT_DEV *)pDev;

    /* input buffer already in use by another packet - just return */

    if (pPktDev->inputState & STATE_BUSY)
	{
	return (ERROR);
	}

    switch (thisChar)
	{
	case FRAME_END:
	    /* don't process zero byte packets */

	    if (pPktDev->inBufPtr == pPktDev->inBufBase)
		{
		RESET_INPUT(pPktDev);
		return (OK);
		}
	    else			/* hand off the packet */
		{
		/* grab an mbuf and use it as a cluster pointer */

		struct mbuf * pMbuf = wdbMbufAlloc();
		if (pMbuf == NULL)
		    {
		    RESET_INPUT(pPktDev);
		    return (ERROR);
		    }
		wdbMbufClusterInit (pMbuf, pPktDev->inBufBase,
				pPktDev->inBufPtr - pPktDev->inBufBase,
				(int (*)())wdbPktFree, (int)pPktDev);
		pPktDev->inputState |= STATE_BUSY;
		(*pPktDev->wdbDrvIf.stackRcv) (pMbuf);
		return (OK);
		}
	case FRAME_ESCAPE:
	    pPktDev->inputState |= STATE_ESC;
	    return (OK);
	default:
	    if (pPktDev->inputState & STATE_ESC)
		{
		pPktDev->inputState &= (~STATE_ESC);
		switch (thisChar)
		    {
		    case TRANS_FRAME_ESCAPE:
			thisChar = FRAME_ESCAPE;
			break;
		    case TRANS_FRAME_END:
			thisChar = FRAME_END;
			break;
		    default:
			return (ERROR);
		    }
		}

	    /* If overflow, reset the input buffer */

	    if (pPktDev->inBufPtr >= pPktDev->inBufEnd)
		{
		RESET_INPUT(pPktDev);
		return (ERROR);
		}

	    /* put the character in the input buffer */

	    *pPktDev->inBufPtr = thisChar;
	    pPktDev->inBufPtr++;
	    break;
	}
    return (OK);
    }


/******************************************************************************
*
* wdbCharTx - transmit a character
*
* This routine is called by a driver's output ISR to get the next character
* for transmition.
*
* RETURNS: OK, or ERROR if there are no more characters in the buffer.
*/

static STATUS wdbCharTx
    (
    void *	pDev,
    caddr_t	thisChar
    )
    {
    WDB_SLIP_PKT_DEV * pPktDev = (WDB_SLIP_PKT_DEV *)pDev;

    /* Return ERROR if there are no characters left to output */

    if (!(pPktDev->outputState & STATE_BUSY))
	{
	return (ERROR);
	}

    /* transmit a FRAME_END at the end of a packet */

    if (mbufChainCharGet (pPktDev->pMbuf, pPktDev->outputIx, thisChar) == ERROR)
	{
	struct mbuf * pNextMbuf = pPktDev->pMbuf->m_act;
	*thisChar = FRAME_END;			/* output end-of-frame */
	mbufChainFree (pPktDev->pMbuf);		/* free the mbuf chain */
	pPktDev->pMbuf = pNextMbuf;		/* queue up the next chain */
	pPktDev->outputIx = 0;
	pPktDev->outputState = STATE_START | STATE_BUSY;
	if (pNextMbuf == NULL)			/* reset if no more packets */
	    wdbPktResetOutput(pPktDev);
	}

    /* transmit a FRAME_END at the start of a packet */

    else if (pPktDev->outputState & STATE_START)
	{
	*thisChar = FRAME_END;
	pPktDev->outputState &= ~STATE_START;
	}

    /* transpose the character if we are in the middle of an escape sequence */

    else if (pPktDev->outputState & STATE_ESC)
	{
	pPktDev->outputState &= (~STATE_ESC);

	switch (*thisChar)
	    {
	    case FRAME_END:
		*thisChar = TRANS_FRAME_END;
		break;
	    case FRAME_ESCAPE:
		*thisChar = TRANS_FRAME_ESCAPE;
		break;
	    default:			/* this can't happen! */
		wdbPktResetOutput (pPktDev);
		return (ERROR);
		}
	pPktDev->outputIx++;
	}

    /* else output the next character in the buffer */

    else switch (*thisChar)
	{
	case FRAME_END:
	case FRAME_ESCAPE:
	    *thisChar = FRAME_ESCAPE;
	    pPktDev->outputState |= STATE_ESC;
	    break;
	default:
	    pPktDev->outputIx++;
	    break;
	}

    return (OK);
    }

/******************************************************************************
*
* wdbPktTx - transmit a packet
*
* This routine can only be called by the WDB agent.
*
* RETURNS: OK, or ERROR if a packet is currently being transmitted, or
* the packet is too large to send.
*/

static STATUS wdbPktTx
    (
    void *	pDev,
    struct mbuf * pMbuf
    )
    {
    WDB_SLIP_PKT_DEV *pPktDev = pDev;
    int 	lockKey;

    /* if we are in polled mode, transmit the packet in a loop */

    if (pPktDev->mode == WDB_COMM_MODE_POLL)
	{
	char thisChar;

	pPktDev->outputState = STATE_BUSY | STATE_START;
	pPktDev->pMbuf	     = pMbuf;
	pPktDev->outputIx    = 0;

	while (wdbCharTx (pPktDev, &thisChar) != ERROR)
	    {
	    while (sioPollOutput (pPktDev->pSioChan, thisChar) == EAGAIN)
		;
	    }

	return (OK);
	}

    lockKey = intLock();

    /* if no mbufs are queued for output, queue this one and start trasnmit */

    if (pPktDev->pMbuf == NULL)
	{
	pPktDev->outputState = STATE_BUSY | STATE_START;
	pPktDev->pMbuf	     = pMbuf;
	pPktDev->outputIx    = 0;

	sioTxStartup (pPktDev->pSioChan);

	intUnlock (lockKey);
	return (OK);
	}

    /* queue up to one packet - no more */

    if (pPktDev->pMbuf->m_act == NULL)
	{
	pPktDev->pMbuf->m_act = pMbuf;
	intUnlock (lockKey);
	return (OK);
	}

    intUnlock (lockKey);

    mbufChainFree (pMbuf);
    return (ERROR);
    }

/******************************************************************************
*
* wdbPktFree - free the input buffer
*
* This routine is called after the agent has read in the packet to indicate
* that the input buffer can be used again to receive a new packet.
*/

static void wdbPktFree
    (
    void *	pDev
    )
    {
    WDB_SLIP_PKT_DEV *	pPktDev = pDev;

    RESET_INPUT(pPktDev);
    }

/******************************************************************************
*
* wdbPktModeSet - set the communications mode to INT or POLL
*/

static STATUS wdbPktModeSet
    (
    void *	pDev,
    uint_t	newMode
    )
    {
    u_int	  sioMode;
    WDB_SLIP_PKT_DEV * pPktDev = pDev;

    RESET_INPUT  (pPktDev);
    wdbPktResetOutput (pPktDev);

    if (newMode == WDB_COMM_MODE_INT)
	sioMode = SIO_MODE_INT;
    else if (newMode == WDB_COMM_MODE_POLL)
	sioMode = SIO_MODE_POLL;
    else
	return (ERROR);

    if (sioIoctl (pPktDev->pSioChan, SIO_MODE_SET, (void *)sioMode) == OK)
	{
	pPktDev->mode = newMode;
	return (OK);
	}

    return (ERROR);
    }

/******************************************************************************
*
* wdbPktPoll - poll for a packet
*
* RETURNS: OK if a character has arrived; otherwise ERROR.
*/ 

static STATUS wdbPktPoll
    (
    void *	pDev
    )
    {
    char	thisChar;
    WDB_SLIP_PKT_DEV * pPktDev = pDev;

    if (sioPollInput (pPktDev->pSioChan, &thisChar) == OK)
	{
	wdbCharRcv ((void *)pPktDev, thisChar);
	return (OK);
	}

    return (ERROR);
    }

/******************************************************************************
*
* mbufChainFree - free a chain of mbufs
*/ 

static void mbufChainFree
    (
    struct mbuf *	pMbuf		/* mbuf chain to free */
    )
    {
    struct mbuf * pNextMbuf;

    while (pMbuf != NULL)
	{
	pNextMbuf = pMbuf->m_next;
	wdbMbufFree (pMbuf);
	pMbuf = pNextMbuf;
	}
    }

/******************************************************************************
*
* mbufChainCharGet - get a character from an mbuf chain
*
* This routine makes a chain of mbufs apprear as a contingurous block
* of memory, and fetches character number <charNum>.
*
* RETURNS: OK, or ERROR if the <charNum> is outside the buffer range.
*/ 

static STATUS mbufChainCharGet
    (
    struct mbuf * pMbuf,		/* mbuf chain */
    int		  charNum,		/* character index */
    char *	  pChar			/* where to put character */
    )
    {
    int bytesRead = 0;
    struct mbuf * pThisMbuf;

    if (charNum < 0)
	return (ERROR);

    for (pThisMbuf = pMbuf; pThisMbuf != NULL; pThisMbuf = pThisMbuf->m_next)
	{
	if (bytesRead + pThisMbuf->m_len > charNum)
	    {
	    *pChar = *(mtod (pThisMbuf, char *) + charNum - bytesRead);
	    return (OK);
	    }

	bytesRead += pThisMbuf->m_len;
	}

    return (ERROR);
    }



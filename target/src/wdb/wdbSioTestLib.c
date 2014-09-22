/* wdbSioTestLib.c - test the serial line connection to the agent */

/* Copyright 1995-2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01b,14sep01,jhw  Fixed warnings from compiling with gnu -pedantic flag
		 Added copyright.
01a,19jun95,ms   written.
*/

/*
DESCRIPTION

This module is used to test the serial connection to the agent.
If the macro INCLUDE_SIO_TEST is defined in configAll.h, then
this library will be called to test the serial connection.
It does the following:

   prints "WDB READY" over the serial line when VxWorks comes up,
   thus verifying that the serial transmitter works.

   optionally echos characters until some designated character (e.g., EOF
   for SLIP) arrives, thus verifying that both the transmitter and
   receiver work.
*/

#include "wdb/wdb.h"
#include "wdb/wdbRtIfLib.h"
#include "errno.h"
#include "string.h"
#include "sioLib.h"

/* locals */

static void *	wdbSioTestSem;
static char *	msg = "WDB READY\n\r";	/* greeting message */
static int	msgSize;		/* size of message */
static int	msgIx;			/* index into the message */
static char	lastInChar;		/* last input character recieved */
static char	lastInCharValid;	/* lastInChar is valid */
static char	eofChar;		/* end of test character */

/* forward declarations */

static STATUS wdbSioTestRcv (void *pDev, char thisChar);
static STATUS wdbSioTestTx  (void *pDev, char * pThisChar);

/******************************************************************************
*
* wdbSioTest - test a serial channel
*/

void wdbSioTest
    (
    SIO_CHAN *	pSioChan,	/* serial channel to test */
    int		mode,		/* mode in which to test the device */
    char	eof		/* character to terminate test */
    )
    {
    msgSize = strlen (msg);
    eofChar = eof;

    if (mode == SIO_MODE_POLL)
	{
	if (sioIoctl (pSioChan, SIO_MODE_SET, (void *)SIO_MODE_POLL) != OK)
	    return;

	/* send the greeting */

	for (msgIx = 0; msgIx < msgSize; msgIx++)
	    {
	    while (sioPollOutput (pSioChan, msg[msgIx]) == EAGAIN)
                ;
	    }

	/* echo characters until eofChar is recieved */

	if (eofChar == 0)
	    return;

	for (;;)
	    {
	    char thisChar;

	    while (sioPollInput (pSioChan, &thisChar) == EAGAIN)
                ;

	    if (thisChar == eofChar)
		return;

	    while (sioPollOutput (pSioChan, thisChar) == EAGAIN)
                ;
	    }
	}

    else if (mode == SIO_MODE_INT)
	{
	/*
	 * The WDB agent will take control of the serial port once
	 * we return, so we must block until we finish the serial test.
	 * Here we create a semaphore to block on.
	 */

	if ((pWdbRtIf->semCreate == NULL) ||
	    (pWdbRtIf->semGive == NULL) ||
	    (pWdbRtIf->semTake == NULL))
	    return;
	wdbSioTestSem = pWdbRtIf->semCreate();

	/* install our interrupt handlers */

	sioCallbackInstall (pSioChan, SIO_CALLBACK_GET_TX_CHAR,
			    (FUNCPTR) wdbSioTestTx, (void *)pSioChan);
	sioCallbackInstall (pSioChan, SIO_CALLBACK_PUT_RCV_CHAR,
			    (FUNCPTR) wdbSioTestRcv, (void *)pSioChan);

	/* put the device in interupt mode and start transmitting */

	if (sioIoctl (pSioChan, SIO_MODE_SET, (void *)SIO_MODE_INT) != OK)
	    return;
	sioTxStartup (pSioChan);

	/* wait until the test is done */

	pWdbRtIf->semTake (wdbSioTestSem, NULL);
	}
    }

/******************************************************************************
*
* wdbSioTestRcv - recieve characters in interrupt mode.
*/ 

static STATUS wdbSioTestRcv
    (
    void *      pDev,
    char        thisChar
    )
    {
    /* end the test when an EOF char arrives */

    if (thisChar == eofChar)
	{
	pWdbRtIf->semGive (wdbSioTestSem);
	return (ERROR);
	}

    /* don't start echoing until we finish printing the greeting msg */

    if (msgIx < msgSize)
	return (ERROR);

    /* else store away the character and start the transmitter */

    lastInChar = thisChar;
    lastInCharValid = TRUE;

    sioTxStartup ((SIO_CHAN *)pDev);
    return (OK);
    }

/******************************************************************************
*
* wdbSioTestTx - transmit characters in interrupt mode.
*/

static STATUS wdbSioTestTx
    (
    void *	pDev,
    char *	pThisChar
    )
    {
    /* first transmit the greeting message */

    if (msgIx < msgSize)
	{
	*pThisChar = msg[msgIx];
	msgIx++;
	return (OK);
	}

    /* end the test after the greeting if no eofChar is specified */

    if (eofChar == 0)
	{
	pWdbRtIf->semGive (wdbSioTestSem);
	return (ERROR);
	}

    /* echo recieved characters */

    if (lastInCharValid)
	{
	*pThisChar = lastInChar;
	lastInCharValid = FALSE;
	return (OK);
	}

    return (ERROR);
    }



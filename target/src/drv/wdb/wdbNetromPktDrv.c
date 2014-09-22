/* wdbNetromPktDrv.c - NETROM packet driver for the WDB agent */

 /* Copyright 1995-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01i,18oct01,jhw  Fixed documentation build errors.
01h,25jul96,ms   more debug support.
01g,29feb96,ms   call config_dpram first in init code (SPR 5759).
01f,24oct95,ms   changed to udpscrmode. Allowed start before kernel.
01e,14oct95,jdi  doc: cleanup.
01d,09oct95,ms   made it work with an external mode agent (SPR #4494).
01c,26sep95,ms   docs.
01b,27jun95,ms	 modified for tornado.
	   + tpr added pollDelay variable in wdbNetromPollTask() and
		 wdbNetromPktDevInit()
01a,06Jun95,sub  written.
*/

/*
DESCRIPTION
This is a lightweight NETROM driver that interfaces with the WDB agent's
UDP/IP interpreter.  It allows the WDB agent to communicate with the host
using the NETROM ROM emulator.  It uses the emulator's read-only protocol
for bi-directional communication.
It requires that NetROM's udpsrcmode option is on.
*/

#include "vxWorks.h"
#include "wdb/wdbLib.h"
#include "wdb/wdbCommIfLib.h"
#include "wdb/wdbMbufLib.h"
#include "drv/wdb/wdbNetromPktDrv.h"
#include "netinet/in_systm.h"
#include "netinet/in.h"
#include "netinet/ip.h"
#include "netinet/udp.h"
#include "intLib.h"
#include "sysLib.h"
#include "taskLib.h"
#include "private/funcBindP.h"
#include "wdb/dptarget.c"

/* some handy macros */

#undef  DBG_PUT
#define DBG_PUT	if (wdbIsNowTasking() && wdbNetromDebug && _func_printErr) \
		     _func_printErr
#define RESET_INPUT(pDev) {pDev->bytesRead=UDP_IP_HDR_SIZE-UDPSRC_HDR_SIZE; \
		     pDev->inBufFull=FALSE;}
#define IP_HDR_SIZE     20
#define UDP_HDR_SIZE    8
#define UDP_IP_HDR_SIZE IP_HDR_SIZE + UDP_HDR_SIZE
#define UDPSRC_HDR_SIZE 6

/* forward static declarations */

static STATUS wdbNetromPoll	(void *pDev);
static STATUS wdbNetromPktTx	(void *pDev, struct mbuf * pMbuf);
static STATUS wdbNetromModeSet	(void *pDev, uint_t newMode);

/* user configurable variables */

int wdbNetromPollPri = 200;

int wdbNetromDebug = 0;		/* print debug message to the console
				 * for all NetROM I/O packets */

int wdbNetromTest = 0;		/* print "WDB NetROM communication ready" to
				 * NetROM's debug port during initialization.
				 * To see this message, you must telnet to
				 * the NetROM's debug port:
				 *	telnet netromIp 1235
				 * Note: VxWorks initialization will halt
				 * until you have connected to the debug port!
				 * If you don't see this message, then the
				 * NetROM connection is no good.
				 */

/******************************************************************************
*
* wdbNetromPollTask - fake an input ISR when a packet arrives
*/ 

static void wdbNetromPollTask
    (
    void *	pDev,
    int		pollDelay
    )
    {
    int key;

    for (;;)
	{
	taskDelay (pollDelay);

	key = intLock ();
	wdbNetromPoll (pDev);
	intUnlock (key);
	}
    }

/******************************************************************************
*
* wdbNetromPktDevInit - initialize a NETROM packet device for the WDB agent
*
* This routine initializes a NETROM packet device.  It is typically
* called from usrWdb.c when the WDB agents NETROM communication path
* is selected.
* The <dpBase> parameter is the address of NetROM's dualport RAM.
* The <width> parameter is the width of a word in ROM space, and can be
* 1, 2, or 4 to select 8-bit, 16-bit, or 32-bit width
* respectivly (use the macro WDB_NETROM_WIDTH in configAll.h
* for this parameter).
* The <index> parameter refers to which byte of the ROM contains pod zero.
* The <numAccess> parameter should be set to the number of accesses to POD
* zero that are required to read a byte. It is typically one, but some boards
* actually read a word at a time.
* This routine spawns a task which polls the NetROM for incomming
* packets every <pollDelay> clock ticks.
*
* RETURNS: N/A
*/ 

void wdbNetromPktDevInit
    (
    WDB_NETROM_PKT_DEV *pPktDev,	/* packet device to initialize */
    caddr_t 	dpBase,			/* address of dualport memory */
    int		width,			/* number of bytes in a ROM word */
    int		index,			/* pod zero's index in a ROM word */
    int		numAccess,		/* to pod zero per byte read */
    void	(*stackRcv)(),		/* callback when packet arrives */
    int		pollDelay		/* poll task delay */
    )
    {
    pPktDev->wdbDrvIf.mode      = WDB_COMM_MODE_POLL | WDB_COMM_MODE_INT;
    pPktDev->wdbDrvIf.mtu       = NETROM_MTU;
    pPktDev->wdbDrvIf.stackRcv  = stackRcv;
    pPktDev->wdbDrvIf.devId     = (void *)pPktDev;
    pPktDev->wdbDrvIf.pollRtn   = wdbNetromPoll;
    pPktDev->wdbDrvIf.pktTxRtn  = wdbNetromPktTx;
    pPktDev->wdbDrvIf.modeSetRtn = wdbNetromModeSet;
    RESET_INPUT (pPktDev);

    DBG_PUT ("calling config_dpram with dpBase = 0x%x, width = %d,\n"
	 "index = %d, numAccess = %d\n", dpBase, width, index, numAccess);
    config_dpram (dpBase, width, index, DPF_READONLY_TGT, numAccess);

    DBG_PUT ("calling set_dp_blockio\n");
    set_dp_blockio (0, 0);		/* non-blocking I/O */

    DBG_PUT ("calling ra_resync\n");
    ra_resync();

    if (wdbNetromTest)
	{
	char * msg = "\nWDB NetROM communication ready\n";
	DBG_PUT ("calling ra_putmsg\n");
	ra_putmsg (msg, strlen(msg));
	}

    DBG_PUT ("spawning tNetromPoll\n");
    if (taskIdCurrent && (pollDelay != -1))
	taskSpawn ("tNetromPoll", wdbNetromPollPri, 0, 5000,
		(int (*)()) wdbNetromPollTask,
		(int)pPktDev, pollDelay,0,0,0,0,0,0,0,0);
    }

/******************************************************************************
*
* wdbNetromPktFree - clear a receive packet
*/ 

static void wdbNetromPktFree
    (
    WDB_NETROM_PKT_DEV *	pDev
    )
    {
    RESET_INPUT (pDev);
    }

/******************************************************************************
*
* wdbNetromPoll - poll device for data
*
* NOMANUAL
*/ 

STATUS wdbNetromPoll
    (
    void *	devId
    )
    {
    WDB_NETROM_PKT_DEV * pDev = (WDB_NETROM_PKT_DEV *)devId;
    struct mbuf * 	pMbuf;
    int		  	numBytes;
    struct udphdr *     pUdpHdr;
    struct ip *         pIpHdr;
    char *		pUdpsrcHdr;

    if (pDev->inBufFull)
	{
	DBG_PUT ("netrom: multiple packets recieved - dropping last packet\n");
	return (ERROR);
	}

    switch (ra_getmsg(pDev->inBuf + pDev->bytesRead,
		NETROM_MTU - pDev->bytesRead, &numBytes))
	{
	case GM_NODATA:
	    return (ERROR);
	case GM_MSGCOMPLETE:
	    pDev->bytesRead += numBytes;

	    /* convert the NETROM udpsrcmode header into a UDP/IP header */

	    pUdpsrcHdr = (char *)pDev->inBuf + UDP_IP_HDR_SIZE
					- UDPSRC_HDR_SIZE;

	    pIpHdr = (struct ip *)pDev->inBuf;
	    pUdpHdr = (struct udphdr *)((int)pIpHdr + IP_HDR_SIZE);

	    bcopy (pUdpsrcHdr, (char *)&pIpHdr->ip_src, 4);
	    pUdpHdr->uh_sport = *(short *)(pUdpsrcHdr + 4);
	    pIpHdr->ip_p = IPPROTO_UDP;
	    pUdpHdr->uh_dport = htons(WDBPORT);

	    DBG_PUT ("ra_getmsg: %d byte packet from IP 0x%x port 0x%x\n",
			pDev->bytesRead, pIpHdr->ip_src, pUdpHdr->uh_sport);

	    pMbuf = wdbMbufAlloc();
	    if (pMbuf == NULL)
		{
		return (ERROR);
		}
	    wdbMbufClusterInit (pMbuf, pDev->inBuf, pDev->bytesRead,
				(int (*)())wdbNetromPktFree, (int)pDev);
	    pDev->inBufFull = TRUE;
	    (*pDev->wdbDrvIf.stackRcv) (pMbuf);
	    return (OK);
	case GM_NOTDONE:
	    DBG_PUT ("ra_getmsg: partial read\n");
	    pDev->bytesRead += numBytes;
	    return (OK);
	case GM_MSGOVERFLOW:
	    DBG_PUT ("ra_getmsg: input buffer overflow ignored\n");
	    RESET_INPUT (pDev);
	    return (ERROR);
	default:
	    DBG_PUT ("ra_getmsg: unknown return value\n");
	    RESET_INPUT (pDev);
	    return (ERROR);
	}

    return (OK);
    }


/******************************************************************************
*
* wdbNetromModeSet - set device mode
*/ 

static STATUS wdbNetromModeSet
    (
    void *	pDev,
    uint_t	newMode
    )
    {
    WDB_NETROM_PKT_DEV * pPktDev = (WDB_NETROM_PKT_DEV *)pDev;
    RESET_INPUT(pPktDev);
    return (OK);
    }


/******************************************************************************
*
* wdbNetromPktTx - transmit a packet
*/ 

static STATUS wdbNetromPktTx
    (
    void *pDev,
    struct mbuf * pMbuf
    )
    {
    struct udphdr *     pUdpHdr;
    struct ip *         pIpHdr;
    struct mbuf *       pFirstMbuf = pMbuf;
    char		data [NETROM_MTU];
    int			len = 0;

    /* convert the UDP/IP header into a NETROM udpsrcmode header */
    /* XXX - assume UDP/IP header is in first mbuf */

    pIpHdr = mtod (pMbuf, struct ip *);
    pUdpHdr = (struct udphdr *)((int)pIpHdr + IP_HDR_SIZE);
    bcopy ((char *)&pIpHdr->ip_dst, &data[0], 4);
    bcopy ((char *)&pUdpHdr->uh_dport, &data[4], 2);
    len = 6;
    pMbuf = pMbuf->m_next;

    while (pMbuf != NULL)
        {
	if (len + pMbuf->m_len > NETROM_MTU)
	    break;
        bcopy (mtod (pMbuf, char *), &data[len], pMbuf->m_len);
        len += pMbuf->m_len;
        pMbuf = pMbuf->m_next;
        }

    wdbMbufChainFree (pFirstMbuf);

    DBG_PUT ("packet to IP 0x%x port 0x%x\n", pIpHdr->ip_dst,
		pUdpHdr->uh_dport);

    ra_putmsg (data, len);

    return (OK);
    }


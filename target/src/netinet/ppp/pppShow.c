/* pppShow.c - Point-to-Point Protocol show routines */

/* Copyright 1995-1999 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01j,14mar99,jdi  doc: removed refs to config.h and/or configAll.h (SPR 25663).
01i,11jul95,dzb  more doc tweaks.
01h,06jul95,dzb  doc tweaks.
01g,23jun95,dzb  changed to pppInfoGet() to copy out structures.
                 added PAP stats to pppInfoShow().
01f,15jun95,dzb  header file consolidation.
01e,08may95,dzb  Added pppSecretShow().
01d,07mar95,dzb  Changed "ip packets" tp be  a real count of IP packets.
                 additional doc/formatting.
01c,09feb95,dab  changed pppInfoShow() format.  removed lcp_echo_fails_reached.
                 included VJ, mtu, and mru info (SPR #4045).
01b,13jan95,dzb  warnings cleanup.  changed to include pppShow.h.  ANSI-fied.
01a,21dec94,dab  VxWorks port - first WRS version.
	   +dzb  added: path for ppp header files, WRS copyright.
*/

/*
DESCRIPTION
This library provides routines to show Point-to-Point Protocol (PPP) link
status information and statistics.  Also provided are routines that
programmatically access this same information.

This library is automatically linked into the VxWorks system image when
the configuration macro INCLUDE_PPP is defined.

INCLUDE FILES: pppLib.h

SEE ALSO: pppLib,
.pG "Network"
*/

/* includes */

#include <vxWorks.h>
#include <stdio.h>
#include <ioctl.h>
#include <net/mbuf.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include "pppLib.h"

/* pointer to the per task variables */

extern PPP_TASK_VARS *ppp_if[];
extern struct ppp_softc *ppp_softc[];

static char *link_phase[] =
    {
    "DEAD",
    "ESTABLISH",
    "AUTHENTICATE",
    "NETWORK",
    "TERMINATE"
    };

static char *link_state[] =
    {
    "INITIAL",
    "STARTING",
    "CLOSED",
    "STOPPED",
    "CLOSING",
    "STOPPING",
    "REQUEST SENT",
    "ACK RECEIVED",
    "ACK SENT",
    "OPENED"
    };

static char *client_pap_state[] =
    {
    "INITIAL",
    "CLOSED",
    "PENDING",
    "AUTHENTICATION REQ",
    "OPEN",
    "BAD AUTHENTICATION"
    };

static char *server_pap_state[] =
    {
    "INITIAL",
    "CLOSED",
    "PENDING",
    "LISTEN",
    "OPEN",
    "BAD AUTHENTICATION"
    };

static char *client_chap_state[] =
    {
    "INITIAL",
    "CLOSED",
    "PENDING",
    "LISTEN",
    "RESPONSE",
    "OPEN"
    };

static char *server_chap_state[] =
    {
    "INITIAL",
    "CLOSED",
    "PENDING",
    "INITIAL CHALLENGE",
    "OPEN",
    "RECHALLENGE",
    "BAD AUTHENTICATION"
    };

/******************************************************************************
*
* pppShowInit - initialize the PPP show facility
*
* This routine links the PPP show facility into the VxWorks system image.
* It is called from usrNetwork.c when the configuration macro INCLUDE_PPP
* is defined.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void pppShowInit (void)
    {
    }

/*******************************************************************************
*
* pppInfoShow - display PPP link status information
*
* This routine displays status information pertaining to each initialized
* Point-to-Point Protocol (PPP) link, regardless of the link state.
* State and option information is gathered for the Link Control Protocol
* (LCP), Internet Protocol Control Protocol (IPCP), Password Authentication
* Protocol (PAP), and Challenge-Handshake Authentication Protocol (CHAP).
*
* RETURNS: N/A
*
* SEE ALSO: pppLib
*/

void pppInfoShow (void)
    {
    FAST int i;
    FAST PPP_TASK_VARS *tmp;    /* per task PPP variables */
    int link_found = 0;
    int n;

    for (i = 0; i < NPPP; i++)
	{
	if ((tmp = ppp_if[i]) == NULL)
	    continue;

	link_found = 1;

	printf ("ppp%d\r\n", i);

	printf ("\tLCP Stats\r\n");
	printf ("\t\t%-30.30s  %s\r\n", "LCP phase", link_phase[tmp->phase]);
	printf ("\t\t%-30.30s  %s\r\n", "LCP state",
		link_state[tmp->lcp_fsm.state]);
	printf ("\t\t%-30.30s  %s\r\n", "passive",
	        tmp->lcp_wantoptions.passive ? "ON" : "OFF");
	printf ("\t\t%-30.30s  %s\r\n", "silent",
	        tmp->lcp_wantoptions.silent ? "ON" : "OFF");
	printf ("\t\t%-30.30s  %s\r\n", "restart",
	        tmp->lcp_wantoptions.restart ? "ON" : "OFF");

        if (tmp->lcp_fsm.state == OPENED)
            {
            ppptioctl (i, PPPIOCGMRU, (caddr_t) &n);
            printf ("\t\t%-30.30s  %d\r\n", "mru", n);

            ppptioctl (i, SIOCGIFMTU, (caddr_t) &n);
            printf ("\t\t%-30.30s  %d\r\n", "mtu", n);

            ppptioctl (i, PPPIOCGASYNCMAP, (caddr_t) &n);
            printf ("\t\t%-30.30s  0x%x\r\n", "async map", n);

            printf ("\t\t%-30.30s  0x%x\r\n", "local magic number",
                    tmp->lcp_gotoptions.magicnumber);

            printf ("\t\t%-30.30s  %s\r\n", "protocol field compression",
                    tmp->lcp_gotoptions.neg_pcompression ? "ON" : "OFF");
            printf ("\t\t%-30.30s  %s\r\n", "addr/ctrl field compression",
                    tmp->lcp_gotoptions.neg_accompression ? "ON" : "OFF");
            }

	printf ("\t\t%-30.30s  %s\r\n", "lcp echo timer",
	        tmp->lcp_echo_timer_running ? "ON" : "OFF");

        if (tmp->lcp_echo_timer_running)
            {
	    printf ("\t\t%-30.30s  %d\r\n", "lcp echos pending",
	            tmp->lcp_echos_pending);
	    printf ("\t\t%-30.30s  %d\r\n", "lcp echo number",
	            tmp->lcp_echo_number);
    	    printf ("\t\t%-30.30s  %d\r\n", "lcp echo interval",
	            tmp->lcp_echo_interval);
            printf ("\t\t%-30.30s  %d\r\n", "lcp echo fails",
	            tmp->lcp_echo_fails);
            }

        printf ("\tIPCP Stats\r\n");
        printf ("\t\t%-30.30s  %s\r\n", "IPCP state",
                link_state[tmp->ipcp_fsm.state]);
     
        if (tmp->ipcp_fsm.state == OPENED)
            {
            printf ("\t\t%-30.30s  %s\r\n", "local IP address",
                    ip_ntoa(tmp->ipcp_gotoptions.ouraddr));
            printf ("\t\t%-30.30s  %s\r\n", "remote IP address",
                    ip_ntoa(tmp->ipcp_hisoptions.hisaddr));
            printf ("\t\t%-30.30s  %s\r\n", "vj compression protocol",
                     tmp->ipcp_gotoptions.neg_vj ? "ON" : "OFF");
            }

	printf ("\tPAP Stats\r\n");
	printf ("\t\t%-30.30s  %s\r\n", "client PAP state",
	        client_pap_state[tmp->upap.us_clientstate]);
	printf ("\t\t%-30.30s  %s\r\n", "server PAP state",
	        server_pap_state[tmp->upap.us_serverstate]);

	printf ("\tCHAP Stats\r\n");
	printf ("\t\t%-30.30s  %s\r\n", "client CHAP state",
	        client_chap_state[tmp->chap.clientstate]);
	printf ("\t\t%-30.30s  %s\r\n", "server CHAP state",
	        server_chap_state[tmp->chap.serverstate]);

	printf ("\n");
        }

    if (!link_found)
	printf ("No PPP links are present\r\n");
    }

/*******************************************************************************
*
* pppInfoGet - get PPP link status information
*
* This routine gets status information pertaining to the specified
* Point-to-Point Protocol (PPP) link, regardless of the link state.
* State and option information is gathered for the Link Control Protocol
* (LCP), Internet Protocol Control Protocol (IPCP), Password Authentication
* Protocol (PAP), and Challenge-Handshake Authentication Protocol (CHAP).
*
* The PPP link information is returned through a PPP_INFO structure, which
* is defined in h/netinet/ppp/pppShow.h.
*
* RETURNS: OK, or ERROR if <unit> is an invalid PPP unit number.
*
* SEE ALSO: pppLib
*/

STATUS pppInfoGet
    (
    int unit,		/* PPP interface unit number to examine */
    PPP_INFO *pInfo	/* PPP_INFO structure to be filled */
    )
    {
    FAST PPP_TASK_VARS *tmp;

    if (unit < 0 || unit > NPPP || (tmp = ppp_if[unit]) == NULL ||
	pInfo == NULL)
        return (ERROR);

    /* LCP variables */

    BCOPY (&tmp->lcp_wantoptions, &pInfo->lcp_wantoptions,
	sizeof(struct lcp_options));
    BCOPY (&tmp->lcp_gotoptions, &pInfo->lcp_gotoptions,
	sizeof(struct lcp_options));
    BCOPY (&tmp->lcp_allowoptions, &pInfo->lcp_allowoptions,
	sizeof(struct lcp_options));
    BCOPY (&tmp->lcp_hisoptions, &pInfo->lcp_hisoptions,
	sizeof(struct lcp_options));

    BCOPY (&tmp->lcp_fsm, &pInfo->lcp_fsm, sizeof(struct fsm));

    pInfo->lcp_echos_pending = tmp->lcp_echos_pending;
    pInfo->lcp_echo_number = tmp->lcp_echo_number;
    pInfo->lcp_echo_timer_running = tmp->lcp_echo_timer_running;
    pInfo->lcp_echo_interval = tmp->lcp_echo_interval;
    pInfo->lcp_echo_fails = tmp->lcp_echo_fails;

    /* IPCP variables */ 

    BCOPY (&tmp->ipcp_wantoptions, &pInfo->ipcp_wantoptions,
	sizeof(struct ipcp_options));
    BCOPY (&tmp->ipcp_gotoptions, &pInfo->ipcp_gotoptions,
	sizeof(struct ipcp_options));
    BCOPY (&tmp->ipcp_allowoptions, &pInfo->ipcp_allowoptions,
	sizeof(struct ipcp_options));
    BCOPY (&tmp->ipcp_hisoptions, &pInfo->ipcp_hisoptions,
	sizeof(struct ipcp_options));

    BCOPY (&tmp->ipcp_fsm, &pInfo->ipcp_fsm, sizeof(struct fsm));

    /* authentication variables */ 

    BCOPY (&tmp->upap, &pInfo->upap, sizeof(struct upap_state));
    BCOPY (&tmp->chap, &pInfo->chap, sizeof(struct chap_state));

    return (OK);
    }

/*******************************************************************************
*
* pppstatShow - display PPP link statistics
*
* This routine displays statistics for each initialized Point-to-Point
* Protocol (PPP) link.  Detailed are the numbers of bytes and packets received 
* and sent through each PPP interface.
*
* RETURNS: N/A
*
* SEE ALSO: pppLib
*/

void pppstatShow (void)
    {
    FAST int i;
    FAST struct ppp_softc *tmp;
    int link_found = 0;

    for (i = 0; i < NPPP; i++)
	{
	if ((tmp = ppp_softc[i]) == NULL)
	    continue;

	link_found = 1;

	printf ("ppp%d\r\n", i);
	printf ("\tInput\r\n");
	printf ("\t\t%-30.30s  %d\r\n", "total bytes ", tmp->sc_bytesrcvd);
	printf ("\t\t%-30.30s  %d\r\n", "total packets ",
	        tmp->sc_if.if_ipackets);
	printf ("\t\t%-30.30s  %d\r\n", "ip packets ", tmp->sc_iprcvd);
	printf ("\t\t%-30.30s  %d\r\n", "VJ compressed packets ",
	        tmp->sc_comp.sls_compressedin);
	printf ("\t\t%-30.30s  %d\r\n", "VJ uncompressed packets ",
	        tmp->sc_comp.sls_uncompressedin);
	printf ("\t\t%-30.30s  %d\r\n", "VJ uncompress errors",
	        tmp->sc_comp.sls_errorin);

	printf ("\tOutput\r\n");
	printf ("\t\t%-30.30s  %d\r\n", "total bytes ", tmp->sc_bytessent);
	printf ("\t\t%-30.30s  %d\r\n", "total packets ",
		tmp->sc_if.if_opackets);
	printf ("\t\t%-30.30s  %d\r\n", "ip packets ", tmp->sc_ipsent);
	printf ("\t\t%-30.30s  %d\r\n", "VJ compressed packets ",
	        tmp->sc_comp.sls_compressed);
	printf ("\t\t%-30.30s  %d\r\n", "VJ uncompressed packets ",
	        tmp->sc_comp.sls_packets - tmp->sc_comp.sls_compressed);
        }

    if (!link_found)
        printf ("No PPP links are present\r\n");
    }

/*******************************************************************************
*
* pppstatGet - get PPP link statistics
*
* This routine gets statistics for the specified Point-to-Point Protocol
* (PPP) link.  Detailed are the numbers of bytes and packets received 
* and sent through the PPP interface.
*
* The PPP link statistics are returned through a PPP_STAT structure, which
* is defined in h/netinet/ppp/pppShow.h.
*
* RETURNS: OK, or ERROR if <unit> is an invalid PPP unit number.
*
* SEE ALSO: pppLib
*/

STATUS pppstatGet
    (
    int unit,			/* PPP interface unit number to examine */
    PPP_STAT *pStat		/* PPP_STAT structure to be filled */
    )
    {
    FAST struct ppp_softc *tmp;

    if (unit < 0 || unit > NPPP || (tmp = ppp_softc[unit]) == NULL ||
	pStat == NULL)
	return (ERROR);

    pStat->in_bytes = tmp->sc_bytesrcvd;
    pStat->in_pkt = tmp->sc_if.if_ipackets;
    pStat->in_ip_pkt = tmp->sc_iprcvd;
    pStat->in_vj_compr_pkt = tmp->sc_comp.sls_compressedin;
    pStat->in_vj_uncompr_pkt = tmp->sc_comp.sls_uncompressedin;
    pStat->in_vj_compr_error = tmp->sc_comp.sls_errorin;
    
    pStat->out_bytes = tmp->sc_bytessent;
    pStat->out_pkt = tmp->sc_if.if_opackets;
    pStat->out_ip_pkt = tmp->sc_ipsent;
    pStat->out_vj_compr_pkt = tmp->sc_comp.sls_compressed;
    pStat->out_vj_uncompr_pkt = tmp->sc_comp.sls_packets -
	tmp->sc_comp.sls_compressed;

    return (OK);
    }

/*******************************************************************************
*
* pppSecretShow - display the PPP authentication secrets table
*
* This routine displays the Point-to-Point Protocol (PPP) authentication
* secrets table.  The information in the secrets table may be used by the
* Password Authentication Protocol (PAP) and Challenge-Handshake Authentication
* Protocol (CHAP) user authentication protocols.
*
* RETURNS: N/A
*
* SEE ALSO: pppLib, pppSecretAdd(), pppSecretDelete()
*/

void pppSecretShow (void)
    {
    PPP_SECRET *	pSecret = pppSecretHead;

    if (pSecret == NULL)				/* list empty ? */
	return;

    /* printout header */

    printf ("PPP AUTHENTICAITON SECRETS:\n");
    printf ("client            server            secret            address list\n");
    printf ("-------------------------------------------------------------------------------\n");

    /* printout secrets */

    for (; pSecret != NULL; pSecret = pSecret->secretNext)
	{
	printf ("%-17.17s ", pSecret->client);
	printf ("%-17.17s ", pSecret->server);
	printf ("%-17.17s ", pSecret->secret);
	printf ("%-25.25s\n", pSecret->addrs);
	}
    }

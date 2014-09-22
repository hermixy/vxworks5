/* pppSecretLib.c - PPP authentication secrets library */

/* Copyright 1995 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01g,14mar99,jdi  doc: removed refs to config.h and/or configAll.h (SPR 25663).
01f,20aug98,fle  doc : removed tab from func headers
01e,19dec95,vin  doc tweaks.
01d,11jul95,dzb  doc tweaks.
01c,06jul95,dzb  added doc.
01b,13jun95,dzb  cleaned up i960 compiler warning.  header file consolidation.
01a,08may95,dzb  written.
*/

/*
DESCRIPTION
This library provides routines to create and manipulate a table of
"secrets" for use with Point-to-Point Protocol (PPP) user authentication
protocols.  The secrets in the secrets table can be searched by peers on
a PPP link so that one peer (client) can send a secret word to the other
peer (server).  If the client cannot find a suitable secret when
required to do so, or the secret received by the server is not
valid, the PPP link may be terminated.

This library is automatically linked into the VxWorks system image when
the configuration macro INCLUDE_PPP is defined.

INCLUDE FILES: pppLib.h

SEE ALSO: pppLib, pppShow,
.pG "Network"

*/

/* includes */

#include "vxWorks.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"
#include "errnoLib.h"
#include "semLib.h"
#include "pppLib.h"

/* globals */

PPP_SECRET *	pppSecretHead = NULL;		/* head of table linked list */

/* locals */

LOCAL SEM_ID 	pppSemId = NULL;		/* protect access to table */

/*******************************************************************************
*
* pppSecretLibInit - initialize the PPP authentication secrets table facility
*
* This routine links the PPP secrets facility into the VxWorks system image.
* It is called from usrNetwork.c when the configuration macro INCLUDE_PPP
* is defined.
*
* RETURNS: N/A
*
* NOMANUAL
*/

STATUS pppSecretLibInit (void)
    {
    if (pppSemId == NULL)		/* already initialized? */
        if ((pppSemId = semMCreate (SEM_Q_PRIORITY | SEM_INVERSION_SAFE))
	    == NULL)
            return (ERROR);

    return (OK);
    }

/*******************************************************************************
*
* pppSecretAdd - add a secret to the PPP authentication secrets table
*
* This routine adds a secret to the Point-to-Point Protocol (PPP)
* authentication secrets table.  This table may be used by the
* Password Authentication Protocol (PAP) and Challenge-Handshake
* Authentication Protocol (CHAP) user authentication protocols.
*
* When a PPP link is established, a "server" may require a "client" to
* authenticate itself using a "secret".  Clients and servers obtain
* authentication secrets by searching secrets files, or by searching
* the secrets table constructed by this routine.  Clients and servers
* search the secrets table by matching client and server names with table
* entries, and retrieving the associated secret.
*
* Client and server names in the table consisting of "*" are considered
* wildcards; they serve as matches for any client and/or server name if
* an exact match cannot be found.
*
* If <secret> starts with "@", <secret> is assumed to be the name of a file,
* wherein the actual secret can be read.
*
* If <addrs> is not NULL, it should contain a list of acceptable client IP
* addresses.   When a server is authenticating a client and the client's
* IP address is not contained in the list of acceptable addresses,
* the link is terminated.  Any IP address will be considered acceptable
* if <addrs> is NULL.  If this parameter is "-", all IP addresses are
* disallowed.
* 
* RETURNS: OK, or ERROR if the secret cannot be added to the table.
*
* SEE ALSO: pppSecretDelete(), pppSecretShow()
*/

STATUS pppSecretAdd
    (
    char *		client,		/* client being authenticated */
    char *		server,		/* server performing authentication */
    char *		secret,		/* secret used for authentication */
    char *		addrs		/* acceptable client IP addresses */
    )
    {
    PPP_SECRET *	pSecret;		/* pointer to new secret */

    if (pppSemId == NULL)
	{
        errno = S_pppSecretLib_NOT_INITIALIZED;
	return (ERROR);
	}

    if ((pSecret = (PPP_SECRET *) calloc (1, sizeof (struct ppp_secret))) ==
	NULL)
	return (ERROR);

    /* copy secret information */

    if (client)
        strcpy (pSecret->client, client);
    if (server)
        strcpy (pSecret->server, server);		
    if (secret)
        strcpy (pSecret->secret, secret);
    if (addrs)
        strcpy (pSecret->addrs, addrs);

    /* hook into secret list */

    semTake (pppSemId, WAIT_FOREVER);		/* exclusive access to list */
    pSecret->secretNext = pppSecretHead;	/* put secret on front */
    pppSecretHead = pSecret;
    semGive (pppSemId);				/* give up access */

    return (OK);
    }

/*******************************************************************************
*
* pppSecretDelete - delete a secret from the PPP authentication secrets table
*
* This routine deletes a secret from the Point-to-Point Protocol (PPP)
* authentication secrets table.  When searching for a secret to delete
* from the table, the wildcard substitution (using "*") is not performed for
* client and/or server names.  The <client>, <server>, and <secret>
* strings must match the table entry exactly in order to be deleted.
*
* RETURNS: OK, or ERROR if the table entry being deleted is not found.
*
* SEE ALSO: pppSecretAdd(), pppSecretShow()
*/

STATUS pppSecretDelete
    (
    char *		client,		/* client being authenticated */
    char *		server,		/* server performing authentication */
    char *		secret		/* secret used for authentication */
    )
    {
    PPP_SECRET *	pSecret;
    PPP_SECRET **	ppPrev;			/* list trailer */

    if (pppSemId == NULL)
	{
        errno = S_pppSecretLib_NOT_INITIALIZED;
	return (ERROR);
	}

    semTake (pppSemId, WAIT_FOREVER);		/* exclusive access to list */

    /* find secret */

    ppPrev = &pppSecretHead;
    for (pSecret = pppSecretHead; pSecret != NULL; pSecret =
	pSecret->secretNext)
        {
        if ((!strcmp (client, pSecret->client)) &&
           (!strcmp (server, pSecret->server)) &&
           (!strcmp (secret, pSecret->secret)))
	    break;

        ppPrev = &pSecret->secretNext;		/* update list trailer */
        }
 
    if (pSecret != NULL)
        *ppPrev = pSecret->secretNext;

    /* unhook from secret list */

    semGive (pppSemId);				/* give up access */

    if (pSecret == NULL)			/* secret found ? */
	{
        errno = S_pppSecretLib_SECRET_DOES_NOT_EXIST;
	return (ERROR);
	}

    free (pSecret);				/* free secret */
    return (OK);
    }

/*******************************************************************************
*
* pppSecretFind - find the best-fit secret in the PPP auth. secrets table
*
* This routine searches the PPP authentication secrets table for a suitable
* secret to authenticate the given <client> and <server>.  The secret is
* returned in the <secret> parameter.  The list of authorized client IP
* addresses are returned in the <ppAddrs> parameter.  If the secret starts
* with "@", the secret is taken to be a filename, wherein which the actual
* secret is read.
* 
* RETURNS: flag determining strength of the secret, or ERROR if no
* suitable secret could be found.
*
* NOMANUAL
*/

int pppSecretFind
    (
    char *		client,		/* client being authenticated */
    char *		server,		/* server performing authentication */
    char *		secret,		/* secret used for authentication */
    struct wordlist **	ppAddrs
    )
    {
    PPP_SECRET *	pSecret = NULL;
    PPP_SECRET *	pEntry;
    char 		atfile [MAXWORDLEN];
    char		word [MAXWORDLEN];
    char *		pWord;
    char *		separators = {" \t"};
    char *		pAddr;
    char *		pLast = NULL;
    FILE *		sf;
    int			got_flag = 0;
    int			best_flag = -1;
    int			xxx;
    struct wordlist *	addr_list = NULL;
    struct wordlist *	addr_last = NULL;
    struct wordlist *	ap;

    if (pppSemId == NULL)
	{
        errno = S_pppSecretLib_NOT_INITIALIZED;
	return (ERROR);
	}

    semTake (pppSemId, WAIT_FOREVER);		/* exclusive access to list */

    /* find best secret entry */

    for (pEntry = pppSecretHead; pEntry != NULL; pEntry = pEntry->secretNext)
	{
         /* check for a match */

	if (((client != NULL) && client[0] && strcmp (client, pEntry->client)
            && !ISWILD (pEntry->client)) ||
            ((server != NULL) && server[0] && strcmp (server, pEntry->server)
            && !ISWILD (pEntry->server)))
            continue;

        if (!ISWILD (pEntry->client))
            got_flag = NONWILD_CLIENT;
        if (!ISWILD (pEntry->server))
            got_flag |= NONWILD_SERVER;
 
        if (got_flag > best_flag)
            {
            pSecret = pEntry;			/* update best entry */
	    best_flag = got_flag;
            }
        }

    semGive (pppSemId);				/* give up access */

    if (pSecret == NULL)			/* secret found ? */
	{
        errno = S_pppSecretLib_SECRET_DOES_NOT_EXIST;
	return (ERROR);
	}

    /* check for special syntax: @filename means read secret from file */

    if (secret != NULL)
	{
        if (pSecret->secret[0] == '@')
            {
            strcpy (atfile, pSecret->secret + 1);
            if ((sf = fopen(atfile, "r")) == NULL)
		{
                syslog (LOG_ERR, "can't open indirect secret file %s",
		    atfile);
                fclose (sf);
	        return (ERROR);
		}

            check_access (sf, atfile);
            if (!getword (sf, word, &xxx, atfile))
		{
                syslog (LOG_ERR, "no secret in indirect secret file %s",
		    atfile);
                fclose (sf);
	        return (ERROR);
		}

            fclose (sf);
            strcpy (secret, word);
            }
        else
            strcpy (secret, pSecret->secret);	/* stuff secret for return */
        }

    /* read address authorization info and make a wordlist */

    if (ppAddrs != NULL)
	{
        *ppAddrs = NULL;			/* tie off in case error */
        strcpy (word, pSecret->addrs);
	pWord = word;

        while ((pAddr = strtok_r (pWord, separators, &pLast)) != NULL)
	    {
	    pWord = NULL;

            if ((ap = (struct wordlist *) malloc (sizeof (struct wordlist)
                                        + strlen (pAddr))) == NULL)
                novm("authorized addresses");

            ap->next = NULL;
            strcpy (ap->word, pAddr);		/* stuff word */

            if (addr_list == NULL)
                addr_list = ap;			/* first word */
            else
                addr_last->next = ap;		/* tie in subsequent words */

            addr_last = ap;			/* bump current word pointer */
            }

        *ppAddrs = addr_list;			/* hook wordlist for return */
        }
 
    return (best_flag);
    }

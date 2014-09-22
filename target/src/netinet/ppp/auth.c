/* auth.c - PPP authentication and phase control */

/* Copyright 1995 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
 * Copyright (c) 1993 The Australian National University.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the Australian National University.  The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Copyright (c) 1989 Carnegie Mellon University.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by Carnegie Mellon University.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/*
modification history
--------------------
01l,14feb97,ms   allow VxSim/Solaris to use 127.x.x.x addresses
01k,30jul95,dzb  fixed to avoid trying to open NULL filenames (SPR #4560).
01j,26jun95,dzb  removed usehostname option.
01i,21jun95,dzb  created pppCryptRtn to unbundled DES encryption.
01h,16jun95,dzb  header file consolidation.
01g,07jun95,dzb  make sure we got neg_chap when we "required" CHAP.
                 made LOG_NOTICE message printed even w/o debug option.
01f,11may95,dzb  added calls to pppSecretLib for VxWorks secrets table.
                 added call to loginUserVerify() for "login" option.
		 re-activated auth_ip_addr() checking.
		 switched lcp_close() calls to go through die().
		 moved WILD defines to auth.h.  removed logged_in and logout().
		 removed checking for attempts at password checking.
01e,05may95,dzb  check for null client/server strings in scan_authfile().
01d,13feb95,dab  added check for debug option in link_terminated().
01c,09feb95,dab  removed the UNIX-centric routine login().
01b,16jan95,dab  reverse params for crypt() call.
01a,21dec94,dab  VxWorks port - first WRS version.
           +dzb  added: path for ppp header files, WRS copyright.
*/

#include "vxWorks.h"
#include "stdio.h"
#include "string.h"
#include "sysLib.h"
#include "taskLib.h"
#include "loginLib.h"
#include "hostLib.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "netinet/in.h"
#include "arpa/inet.h"
#include "pppLib.h"

/* Bits in auth_pending[] */
#define UPAP_WITHPEER	1
#define UPAP_PEER	2
#define CHAP_WITHPEER	4
#define CHAP_PEER	8

/* Prototypes */

static int  null_login __ARGS((int));
static int  get_upap_passwd __ARGS((void));
static int  have_upap_secret __ARGS((void));
static int  have_chap_secret __ARGS((char *, char *));
static int  scan_authfile __ARGS((FILE *, char *, char *, char *,
				  struct wordlist **, char *));
static void free_wordlist __ARGS((struct wordlist *));

FUNCPTR	pppCryptRtn = NULL;	/* PPP cryptographic authentication routine */

/*
 * An Open on LCP has requested a change from Dead to Establish phase.
 * Do what's necessary to bring the physical layer up.
 */
void
link_required(unit)
	int unit;
{
}

/*
 * LCP has terminated the link; go to the Dead phase and take the
 * physical layer down.
 */
void
link_terminated(unit)
	int unit;
{
    ppp_if[unit]->phase = PHASE_DEAD;
    syslog(LOG_NOTICE, "Connection terminated.");
}

/*
 * LCP has gone down; it will either die or try to re-establish.
 */
void
link_down(unit)
    int unit;
{
    ppp_if[unit]->phase = PHASE_TERMINATE;
}

/*
 * The link is established.
 * Proceed to the Dead, Authenticate or Network phase as appropriate.
 */
void
link_established(unit)
	int unit;
{
    int auth;
    lcp_options *wo = &ppp_if[unit]->lcp_wantoptions;
    lcp_options *go = &ppp_if[unit]->lcp_gotoptions;
    lcp_options *ho = &ppp_if[unit]->lcp_hisoptions;

    if (ppp_if[unit]->auth_required && !(go->neg_chap || go->neg_upap)) {
	/*
         * We wanted the peer to authenticate itself, and it refused:
         * treat it as though it authenticated with PAP using a username
         * of "" and a password of "".  If that's not OK, boot it out.
	 *
	 * NB:
	 * Changed this "if" statement to look for requiring CHAP and
	 * not PAP (covered by !wo->neg_upap), so that peer cannot simply
	 * refuse CHAP and get away with it...
	 */
        if (!wo->neg_upap || !null_login(unit)) {
            syslog(LOG_WARNING, "peer refused to authenticate");
	    die(unit, 1);
        }
    }

    ppp_if[unit]->phase = PHASE_AUTHENTICATE;
    auth = 0;
    if (go->neg_chap) {
	ChapAuthPeer(unit, ppp_if[unit]->our_name, go->chap_mdtype);
	auth |= CHAP_PEER;
    } else if (go->neg_upap) {
	upap_authpeer(unit);
	auth |= UPAP_PEER;
    }
    if (ho->neg_chap) {
	ChapAuthWithPeer(unit, ppp_if[unit]->our_name, ho->chap_mdtype);
	auth |= CHAP_WITHPEER;
    } else if (ho->neg_upap) {
	upap_authwithpeer(unit, ppp_if[unit]->user, ppp_if[unit]->passwd);
	auth |= UPAP_WITHPEER;
    }
    ppp_if[unit]->auth_pending = auth;

    if (!auth) {
        ppp_if[unit]->phase = PHASE_NETWORK;
        ipcp_open(unit);
    }
}

/*
 * The peer has failed to authenticate himself using `protocol'.
 */
void
auth_peer_fail(unit, protocol)
    int unit, protocol;
{
    /*
     * Authentication failure: take the link down
     */
    die(unit, 1);
}

/*
 * The peer has been successfully authenticated using `protocol'.
 */
void
auth_peer_success(unit, protocol)
    int unit, protocol;
{
    int bit;

    switch (protocol) {
    case CHAP:
	bit = CHAP_PEER;
	break;
    case UPAP:
	bit = UPAP_PEER;
	break;
    default:
	syslog(LOG_WARNING, "auth_peer_success: unknown protocol %x",
               protocol);
	return;
    }

    /*
     * If there is no more authentication still to be done,
     * proceed to the network phase.
     */
    if ((ppp_if[unit]->auth_pending &= ~bit) == 0) {
        ppp_if[unit]->phase = PHASE_NETWORK;
	ipcp_open(unit);
    }
}

/*
 * We have failed to authenticate ourselves to the peer using `protocol'.
 */
void
auth_withpeer_fail(unit, protocol)
    int unit, protocol;
{
    /*
     * We've failed to authenticate ourselves to our peer.
     * He'll probably take the link down, and there's not much
     * we can do except wait for that.
     */
}

/*
 * We have successfully authenticated ourselves with the peer using `protocol'.
 */
void
auth_withpeer_success(unit, protocol)
    int unit, protocol;
{
    int bit;

    switch (protocol) {
    case CHAP:
	bit = CHAP_WITHPEER;
	break;
    case UPAP:
	bit = UPAP_WITHPEER;
	break;
    default:
	syslog(LOG_WARNING, "auth_peer_success: unknown protocol %x",
               protocol);
        bit = 0;
    }

    /*
     * If there is no more authentication still being done,
     * proceed to the network phase.
     */
    if ((ppp_if[unit]->auth_pending &= ~bit) == 0) {
        ppp_if[unit]->phase = PHASE_NETWORK;
	ipcp_open(unit);
    }
}

/*
 * check_auth_options - called to check authentication options.
 */
void
check_auth_options()
{
    lcp_options *wo = &ppp_if[ppp_unit]->lcp_wantoptions;
    lcp_options *ao = &ppp_if[ppp_unit]->lcp_allowoptions;

    /* Default our_name to hostname, and user to our_name */
    if (ppp_if[ppp_unit]->our_name[0] == 0)
	strcpy(ppp_if[ppp_unit]->our_name, ppp_if[ppp_unit]->hostname);
    if (ppp_if[ppp_unit]->user[0] == 0)
	strcpy(ppp_if[ppp_unit]->user, ppp_if[ppp_unit]->our_name);

    /* If authentication is required, ask peer for CHAP or PAP. */
    if (ppp_if[ppp_unit]->auth_required && !wo->neg_chap && !wo->neg_upap) {
	wo->neg_chap = 1;
	wo->neg_upap = 1;
    }

    /*
     * Check whether we have appropriate secrets to use
     * to authenticate ourselves and/or the peer.
     */
    if (ao->neg_upap && ppp_if[ppp_unit]->passwd[0] == 0 && !get_upap_passwd())
	ao->neg_upap = 0;
    if (wo->neg_upap && !ppp_if[ppp_unit]->uselogin && !have_upap_secret())
	wo->neg_upap = 0;
    if (ao->neg_chap && !have_chap_secret(ppp_if[ppp_unit]->our_name, ppp_if[ppp_unit]->remote_name))
	ao->neg_chap = 0;
    if (wo->neg_chap && !have_chap_secret(ppp_if[ppp_unit]->remote_name, ppp_if[ppp_unit]->our_name))
	wo->neg_chap = 0;

    if (ppp_if[ppp_unit]->auth_required && !wo->neg_chap && !wo->neg_upap) {
	syslog(LOG_ERR, "peer authentication required but no authentication files accessible");
	die(ppp_unit, 1);
    }

}


/*
 * check_passwd - Check the user name and passwd against the PAP secrets
 * file.  If requested, also check against the system password database,
 * and login the user if OK.
 *
 * returns:
 *	UPAP_AUTHNAK: Authentication failed.
 *	UPAP_AUTHACK: Authentication succeeded.
 * In either case, msg points to an appropriate message.
 */
int
check_passwd(unit, auser, userlen, apasswd, passwdlen, msg, msglen)
    int unit;
    char *auser;
    int userlen;
    char *apasswd;
    int passwdlen;
    char **msg;
    int *msglen;
{
    int ret = UPAP_AUTHACK;
    char *filename;
    FILE *f = NULL;
    struct wordlist *addrs = NULL;
    char passwd[256], user[256];
    char secret[MAXWORDLEN];

    /*
     * Make copies of apasswd and auser, then null-terminate them.
     */
    BCOPY(apasswd, passwd, passwdlen);
    passwd[passwdlen] = '\0';
    BCOPY(auser, user, userlen);
    user[userlen] = '\0';

    if (ppp_if[unit]->uselogin)
	{
        if (loginUserVerify (user, passwd) == ERROR)
	    {
	    ret = UPAP_AUTHNAK;
            syslog(LOG_WARNING, "upap login failure for %s", user);
	    }
        }
    else if (pppSecretFind (user, ppp_if[unit]->our_name, secret, &addrs) < 0 ||
        (secret[0] != 0 && strcmp(passwd, secret) != 0 &&
	((pppCryptRtn == NULL) ||
        strcmp((char *) (*pppCryptRtn) (secret, passwd), passwd) != 0)))
        {
        /*
         * Open the file of upap secrets and scan for a suitable secret
         * for authenticating this user.
         */

        if ((filename = ppp_if[unit]->options->pap_file) != NULL)
            f = fopen(filename, "r");

        if (f == NULL) {
	    syslog(LOG_ERR, "Can't open upap password file %s", filename);
	    ret = UPAP_AUTHNAK;
        } else {
	    check_access(f, filename);
            if (scan_authfile(f, user, ppp_if[unit]->our_name, secret,
		&addrs, filename) < 0 || (secret[0] != 0 &&
		strcmp(passwd, secret) != 0 && ((pppCryptRtn == NULL) ||
		strcmp((char *) (*pppCryptRtn) (secret, passwd), passwd) != 0)))
		{
                syslog(LOG_WARNING, "upap authentication failure for %s", user);
	        ret = UPAP_AUTHNAK;
	        }
	    fclose(f);
            }
        }

    if (ret == UPAP_AUTHNAK) {
        *msg = "Login incorrect";
        *msglen = strlen(*msg);
	if (addrs != NULL)
	    free_wordlist(addrs);

    } else {
	*msg = "Login ok";
	*msglen = strlen(*msg);
	if (ppp_if[unit]->addresses != NULL)
	    free_wordlist(ppp_if[unit]->addresses);
	ppp_if[unit]->addresses = addrs;
    }

    return ret;
}


/*
 * upap_login - Check the user name and password against the system
 * password database, and login the user if OK.
 *
 * returns:
 *	UPAP_AUTHNAK: Login failed.
 *	UPAP_AUTHACK: Login succeeded.
 * In either case, msg points to an appropriate message.
 */

#ifdef	notyet
static int
login(user, passwd)
    char *user;
    char *passwd;
{
    struct passwd *pw;
    char *epasswd;
    char *tty;

    if ((pw = getpwnam(user)) == NULL) {
        return (UPAP_AUTHNAK);
    }

    /*
     * XXX If no passwd, let them login without one.
     */

    if (pw->pw_passwd == '\0') {
        return (UPAP_AUTHACK);
    }

    epasswd = crypt(passwd, pw->pw_passwd);
    if (strcmp(epasswd, pw->pw_passwd)) {
        return (UPAP_AUTHNAK);
    }

    syslog(LOG_INFO, "user %s logged in", user);

    /*
     * Write a wtmp entry for this user.
     */
    tty = strrchr(devname, '/');
    if (tty == NULL)
        tty = devname;
    else
        tty++;
    logwtmp(tty, user, "");             /* Add wtmp login entry */
    logged_in = TRUE;

}

/*
 * logout - Logout the user.
 */
static void
logout()
{
    char *tty;

    tty = strrchr(ppp_if[ppp_unit]->devname, '/');
    if (tty == NULL)
	tty = ppp_if[ppp_unit]->devname;
    else
	tty++;
    ppp_if[ppp_unit]->logged_in = FALSE;
}
#endif	/* notyet */

/*
 * null_login - Check if a username of "" and a password of "" are
 * acceptable, and iff so, set the list of acceptable IP addresses
 * and return 1.
 */
static int
null_login(unit)
    int unit;
{
    char *filename;
    FILE *f = NULL;
    int i, ret;
    struct wordlist *addrs = NULL;
    char secret[MAXWORDLEN];

    i = pppSecretFind ("", ppp_if[unit]->our_name, secret, &addrs);
    ret = i >= 0 && (i & NONWILD_CLIENT) != 0 && secret[0] == 0;

    if (!ret)
	{
        /*
         * Open the file of upap secrets and scan for a suitable secret.
         * We don't accept a wildcard client.
         */

        if ((filename = ppp_if[unit]->options->pap_file) != NULL)
            f = fopen(filename, "r");

        if (f == NULL)
            return 0;
        check_access(f, filename);

        i = scan_authfile(f, "", ppp_if[unit]->our_name, secret, &addrs,
		      filename);
        ret = i >= 0 && (i & NONWILD_CLIENT) != 0 && secret[0] == 0;
        fclose(f);
        }

    if (ret) {
        if (ppp_if[unit]->addresses != NULL)
            free_wordlist(ppp_if[unit]->addresses);
        ppp_if[unit]->addresses = addrs;
        }

    return ret;
}


/*
 * get_upap_passwd - get a password for authenticating ourselves with
 * our peer using PAP.  Returns 1 on success, 0 if no suitable password
 * could be found.
 */
static int
get_upap_passwd()
{
    char *filename;
    FILE *f = NULL;
    char secret[MAXWORDLEN];

    if (pppSecretFind (ppp_if[ppp_unit]->user, ppp_if[ppp_unit]->remote_name,
	secret, NULL) < 0)
        {
        if ((filename = ppp_if[ppp_unit]->options->pap_file) != NULL)
            f = fopen(filename, "r");

        if (f == NULL)
	    return 0;
        check_access(f, filename);
        if (scan_authfile(f, ppp_if[ppp_unit]->user,
	    ppp_if[ppp_unit]->remote_name, secret, NULL, filename) < 0)
	    {
            fclose(f);
	    return 0;
	    }
        fclose(f);
        }

    strncpy(ppp_if[ppp_unit]->passwd, secret, MAXSECRETLEN);
    ppp_if[ppp_unit]->passwd[MAXSECRETLEN-1] = 0;
    return 1;
}


/*
 * have_upap_secret - check whether we have a PAP file with any
 * secrets that we could possibly use for authenticating the peer.
 */
static int
have_upap_secret()
{
    FILE *f = NULL;
    int ret;
    char *filename;

    if (ppp_if[ppp_unit]->uselogin)
	return 1;

    if (pppSecretFind (NULL, ppp_if[ppp_unit]->our_name, NULL, NULL) < 0)
        {
        if ((filename = ppp_if[ppp_unit]->options->pap_file) != NULL)
            f = fopen(filename, "r");

        if (f == NULL)
	    return 0;

        ret = scan_authfile(f, NULL, ppp_if[ppp_unit]->our_name, NULL, NULL,
			filename);
        fclose(f);
        if (ret < 0)
	    return 0;
	}

    return 1;
}


/*
 * have_chap_secret - check whether we have a CHAP file with a
 * secret that we could possibly use for authenticating `client'
 * on `server'.  Either can be the null string, meaning we don't
 * know the identity yet.
 */
static int
have_chap_secret(client, server)
    char *client;
    char *server;
{
    FILE *f = NULL;
    int ret;
    char *filename;

    if (client[0] == 0)
        client = NULL;
    else if (server[0] == 0)
        server = NULL;

    if (pppSecretFind (client, server, NULL, NULL) < 0)
	{
        if ((filename = ppp_if[ppp_unit]->options->chap_file) != NULL)
            f = fopen(filename, "r");

        if (f == NULL)
            return 0;

        ret = scan_authfile(f, client, server, NULL, NULL, filename);
        fclose(f);
        if (ret < 0)
	    return 0;
	}

    return 1;
}


/*
 * get_secret - open the CHAP secret file and return the secret
 * for authenticating the given client on the given server.
 * (We could be either client or server).
 */
int
get_secret(unit, client, server, secret, secret_len, save_addrs)
    int unit;
    char *client;
    char *server;
    char *secret;
    int *secret_len;
    int save_addrs;
{
    FILE *f = NULL;
    int ret, len;
    char *filename;
    struct wordlist *addrs = NULL;
    char secbuf[MAXWORDLEN];

    secbuf[0] = 0;
    if (pppSecretFind (client, server, secbuf, &addrs) < 0)
	{
        if ((filename = ppp_if[unit]->options->chap_file) != NULL)
            f = fopen(filename, "r");

        if (f == NULL) {
	    syslog(LOG_ERR, "Can't open chap secret file %s", filename);
	    return 0;
        }
        check_access(f, filename);

        ret = scan_authfile(f, client, server, secbuf, &addrs, filename);
        fclose(f);
        if (ret < 0)
	    return 0;
        }

    if (save_addrs) {
	if (ppp_if[unit]->addresses != NULL)
	    free_wordlist(ppp_if[unit]->addresses);
	ppp_if[unit]->addresses = addrs;
    }

    len = strlen(secbuf);
    if (len > MAXSECRETLEN) {
	syslog(LOG_ERR, "Secret for %s on %s is too long", client, server);
	len = MAXSECRETLEN;
    }
    BCOPY(secbuf, secret, len);
    *secret_len = len;

    return 1;
}

/*
 * auth_ip_addr - check whether the peer is authorized to use
 * a given IP address.  Returns 1 if authorized, 0 otherwise.
 */
int
auth_ip_addr(unit, addr)
    int unit;
    u_long addr;
{
    u_long a;
    struct wordlist *addrs;

    /* don't allow loopback or multicast address */
    if (bad_ip_adrs(addr))
        return 0;

    if ((addrs = ppp_if[unit]->addresses) == NULL)
	return 1;		/* no restriction */

    for (; addrs != NULL; addrs = addrs->next) {
	/* "-" means no addresses authorized */
	if (strcmp(addrs->word, "-") == 0)
	    break;
	if ((a = inet_addr(addrs->word)) == -1) {
	    if ((a = hostGetByName (addrs->word)) == NULL) {
		syslog(LOG_WARNING, "unknown host %s in auth. address list",
		       addrs->word);
		continue;
		}
	}
	if (addr == a)
	    return 1;
    }
    return 0;			/* not in list => can't have it */
}

/*
 * bad_ip_adrs - return 1 if the IP address is one we don't want
 * to use, such as an address in the loopback net or a multicast address.
 * addr is in network byte order.
 */
int
bad_ip_adrs(addr)
    u_long addr;
{
    addr = ntohl(addr);
#if	CPU==SIMSPARCSOLARIS
    return IN_MULTICAST(addr) || IN_BADCLASS(addr);
#else
    return (addr >> IN_CLASSA_NSHIFT) == IN_LOOPBACKNET
        || IN_MULTICAST(addr) || IN_BADCLASS(addr);
#endif
}

/*
 * check_access - complain if a secret file has too-liberal permissions.
 */
void
check_access(f, fileName)
    FILE *f;
    char *fileName;
{
    struct stat sbuf;

    if (fstat(fileno(f), &sbuf) < 0) {
	syslog(LOG_WARNING, "cannot stat secret file %s: %m", fileName);
    } else if ((sbuf.st_mode & (S_IRWXG | S_IRWXO)) != 0) {
	syslog(LOG_WARNING, "Warning - secret file %s has world and/or group access", fileName);
    }
}


/*
 * scan_authfile - Scan an authorization file for a secret suitable
 * for authenticating `client' on `server'.  The return value is -1
 * if no secret is found, otherwise >= 0.  The return value has
 * NONWILD_CLIENT set if the secret didn't have "*" for the client, and
 * NONWILD_SERVER set if the secret didn't have "*" for the server.
 * Any following words on the line (i.e. address authorization
 * info) are placed in a wordlist and returned in *addrs.  
 */
static int
scan_authfile(f, client, server, secret, addrs, fileName)
    FILE *f;
    char *client;
    char *server;
    char *secret;
    struct wordlist **addrs;
    char *fileName;
{
    int newline, xxx;
    int got_flag, best_flag;
    FILE *sf;
    struct wordlist *ap, *addr_list, *addr_last;
    char word[MAXWORDLEN];
    char atfile[MAXWORDLEN];

    if (addrs != NULL)
	*addrs = NULL;
    addr_list = NULL;
    if (!getword(f, word, &newline, fileName))
	return -1;		/* file is empty??? */
    newline = 1;
    best_flag = -1;
    for (;;) {
	/*
	 * Skip until we find a word at the start of a line.
	 */
	while (!newline && getword(f, word, &newline, fileName))
	    ;
	if (!newline)
	    break;		/* got to end of file */

	/*
	 * Got a client - check if it's a match or a wildcard.
	 */
	got_flag = 0;
	if (client != NULL && client[0] && strcmp(word, client) != 0 &&
	    !ISWILD(word)) {
	    newline = 0;
	    continue;
	}
	if (!ISWILD(word))
	    got_flag = NONWILD_CLIENT;

	/*
	 * Now get a server and check if it matches.
	 */
	if (!getword(f, word, &newline, fileName))
	    break;
	if (newline)
	    continue;
	if (server != NULL && server[0] && strcmp(word, server) != 0 &&
	    !ISWILD(word))
	    continue;
	if (!ISWILD(word))
	    got_flag |= NONWILD_SERVER;

	/*
	 * Got some sort of a match - see if it's better than what
	 * we have already.
	 */
	if (got_flag <= best_flag)
	    continue;

	/*
	 * Get the secret.
	 */
	if (!getword(f, word, &newline, fileName))
	    break;
	if (newline)
	    continue;

	/*
	 * Special syntax: @filename means read secret from file.
	 */
	if (word[0] == '@') {
	    strcpy(atfile, word+1);
	    if ((sf = fopen(atfile, "r")) == NULL) {
		syslog(LOG_WARNING, "can't open indirect secret file %s",
                       atfile);
		continue;
	    }
	    check_access(sf, atfile);
	    if (!getword(sf, word, &xxx, atfile)) {
		syslog(LOG_WARNING, "no secret in indirect secret file %s",
                       atfile);
		fclose(sf);
		continue;
	    }
	    fclose(sf);
	}
	if (secret != NULL)
	    strcpy(secret, word);
		
	best_flag = got_flag;

	/*
	 * Now read address authorization info and make a wordlist.
	 */
	if (addr_list)
	    free_wordlist(addr_list);
	addr_list = addr_last = NULL;
	for (;;) {
	    if (!getword(f, word, &newline, fileName) || newline)
		break;
	    ap = (struct wordlist *) malloc(sizeof(struct wordlist)
					    + strlen(word));
	    if (ap == NULL)
		novm("authorized addresses");
	    ap->next = NULL;
	    strcpy(ap->word, word);
	    if (addr_list == NULL)
		addr_list = ap;
	    else
		addr_last->next = ap;
	    addr_last = ap;
	}
	if (!newline)
	    break;
    }

    if (addrs != NULL)
	*addrs = addr_list;
    else if (addr_list != NULL)
	free_wordlist(addr_list);

    return best_flag;
}

/*
 * free_wordlist - release memory allocated for a wordlist.
 */
static void
free_wordlist(wp)
    struct wordlist *wp;
{
    struct wordlist *next;

    while (wp != NULL) {
	next = wp->next;
	free(wp);
	wp = next;
    }
}

/* options.c - handles option processing for PPP */

/* Copyright 1995 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
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
01m,29nov95,vin  added code to accept hostnames for local_adrs & remote_adrs
01l,05jul95,dzb  close fd's in setupapfile() and setchapfile().
01k,23jun95,dzb  fixed setipcpfails() to access ipcp, not lcp.
                 removed usehostname option.
01j,16jun95,dzb  header file consolidation.  removed perror() references.
01i,22may95,dzb  Changed pap/chap_require_file to just pap/chap_require.
                 Changed to no params for reqpap() and reqchap().
01h,05may95,dzb  removed parsing code in setupapfile() (original +ua file).
		 fixed getword() for "/n", then EOF exit condition.
		 added setpasswd() for setting passwd[].
                 re-activated "login" option.
01g,16feb95,dab  added stringdup() call for pap_file in setupapfile().
01f,13feb95,dab  added call out to setchapfile() in parse_args() (SPR #4072).
01e,09feb95,dab  removed "login" option.
01d,07feb95,dab  only parse options struct if one is specified.
01c,18jan95,dzb  ifdef'ed out usage().
01b,16jan95,dab  ifdef'ed out options_from_user(), options_for_tty().
01a,21dec94,dab  VxWorks port - first WRS version.
	   +dzb  added: path for ppp header files, WRS copyright.
*/

#include <vxWorks.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <hostLib.h>
#include <ioLib.h>
#include <inetLib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <ctype.h>
#include "pppLib.h"

#ifndef GIDSET_TYPE
#define GIDSET_TYPE     int
#endif

/*
 * Prototypes
 */
static int setdebug __ARGS((void));
static int setkdebug __ARGS((void));
static int setpassive __ARGS((void));
static int setsilent __ARGS((void));
static int noopt __ARGS((void));
static int setnovj __ARGS((void));
static int setnovjccomp __ARGS((void));
static int setvjslots __ARGS((char *));
static int nopap __ARGS((void));
static int nochap __ARGS((void));
static int reqpap __ARGS((void));
static int reqchap __ARGS((void));
static int setchapfile __ARGS((char *));
static int setupapfile __ARGS((char *));
static int setspeed __ARGS((int, int));
static int noaccomp __ARGS((void));
static int noasyncmap __ARGS((void));
static int noipaddr __ARGS((void));
static int nomagicnumber __ARGS((void));
static int setasyncmap __ARGS((char *));
static int setescape __ARGS((char *));
static int setmru __ARGS((char *));
static int setmtu __ARGS((char *));
static int nomru __ARGS((void));
static int nopcomp __ARGS((void));
static int setnetmask __ARGS((char *));
static int setname __ARGS((char *));
static int setuser __ARGS((char *));
static int setpasswd __ARGS((char *));
static int setremote __ARGS((char *));
static int readfile __ARGS((char *));
static int setdefaultroute __ARGS((void));
static int setproxyarp __ARGS((void));
static int setdologin __ARGS((void));
static int setlcptimeout __ARGS((char *));
static int setlcpterm __ARGS((char *));
static int setlcpconf __ARGS((char *));
static int setlcpfails __ARGS((char *));
static int setipcptimeout __ARGS((char *));
static int setipcpterm __ARGS((char *));
static int setipcpconf __ARGS((char *));
static int setipcpfails __ARGS((char *));
static int setpaptimeout __ARGS((char *));
static int setpapreqs __ARGS((char *));
static int setchaptimeout __ARGS((char *));
static int setchapchal __ARGS((char *));
static int setchapintv __ARGS((char *));
static int setipcpaccl __ARGS((void));
static int setipcpaccr __ARGS((void));
static int setlcpechointv __ARGS((char *));
static int setlcpechofails __ARGS((char *));

static int number_option __ARGS((char *, long *, int));
static int readable __ARGS((int fd));

/*
 * Valid arguments.
 */
static struct cmd {
    char *cmd_name;
    int num_args;
    int (*cmd_func)();
} cmds[] = {
    {"no_all", 0, noopt},	/* Don't request/allow any options */
    {"no_acc", 0, noaccomp},	/* Disable Address/Control compress */
    {"no_asyncmap", 0, noasyncmap}, /* Disable asyncmap negotiation */
    {"debug", 0, setdebug},	/* Enable the daemon debug mode */
    {"driver_debug", 1, setkdebug}, /* Enable driver-level debugging */
    {"no_ip", 0, noipaddr},	/* Disable IP address negotiation */
    {"no_mn", 0, nomagicnumber}, /* Disable magic number negotiation */
    {"no_mru", 0, nomru},	/* Disable mru negotiation */
    {"passive_mode", 0, setpassive},	/* Set passive mode */
    {"no_pc", 0, nopcomp},	/* Disable protocol field compress */
    {"no_pap", 0, nopap},	/* Don't allow UPAP authentication with peer */
    {"no_chap", 0, nochap},	/* Don't allow CHAP authentication with peer */
    {"require_pap", 0, reqpap}, /* Require PAP auth from peer */
    {"require_chap", 0, reqchap}, /* Require CHAP auth from peer */
    {"no_vj", 0, setnovj},	/* disable VJ compression */
    {"no_vjccomp", 0, setnovjccomp}, /* disable VJ connection-ID compression */
    {"silent_mode", 0, setsilent},	/* Set silent mode */
    {"defaultroute", 0, setdefaultroute}, /* Add default route */
    {"proxyarp", 0, setproxyarp}, /* Add proxy ARP entry */
    {"login", 0, setdologin}, /* Use system password database for UPAP */
    {"ipcp_accept_local", 0, setipcpaccl}, /* Accept peer's address for us */
    {"ipcp_accept_remote", 0, setipcpaccr}, /* Accept peer's address for it */
    {"asyncmap", 1, setasyncmap}, /* set the desired async map */
    {"vj_max_slots", 1, setvjslots}, /* Set maximum VJ header slots */
    {"escape_chars", 1, setescape}, /* set chars to escape on transmission */
    {"pap_file", 1, setupapfile}, /* Get PAP user and password from file */
    {"chap_file", 1, setchapfile}, /* Get CHAP info */
    {"mru", 1, setmru},		/* Set MRU value for negotiation */
    {"mtu", 1, setmtu},		/* Set our MTU */
    {"netmask", 1, setnetmask},	/* Set netmask */
    {"local_auth_name", 1, setname}, /* Set local name for authentication */
    {"pap_user_name", 1, setuser}, /* Set username for PAP auth with peer */
    {"pap_passwd", 1, setpasswd}, /* Set password for PAP auth with peer */
    {"remote_auth_name", 1, setremote}, /* Set remote name for authentication */
    {"lcp_echo_failure", 1, setlcpechofails}, /* consecutive echo failures */
    {"lcp_echo_interval", 1, setlcpechointv}, /* time for lcp echo events */
    {"lcp_restart", 1, setlcptimeout}, /* Set timeout for LCP */
    {"lcp_max_terminate", 1, setlcpterm}, /* Set max #xmits for term-reqs */
    {"lcp_max_configure", 1, setlcpconf}, /* Set max #xmits for conf-reqs */
    {"lcp_max_failure", 1, setlcpfails}, /* Set max #conf-naks for LCP */
    {"ipcp_restart", 1, setipcptimeout}, /* Set timeout for IPCP */
    {"ipcp_max_terminate", 1, setipcpterm}, /* Set max #xmits for term-reqs */
    {"ipcp_max_configure", 1, setipcpconf}, /* Set max #xmits for conf-reqs */
    {"ipcp_max_failure", 1, setipcpfails}, /* Set max #conf-naks for IPCP */
    {"pap_restart", 1, setpaptimeout}, /* Set timeout for UPAP */
    {"pap_max_authreq", 1, setpapreqs}, /* Set max #xmits for auth-reqs */
    {"chap_restart", 1, setchaptimeout}, /* Set timeout for CHAP */
    {"chap_max_challenge", 1, setchapchal}, /* Set max #xmits for challenge */
    {"chap_interval", 1, setchapintv}, /* Set interval for rechallenge */
    {NULL, 0, NULL}
};

#ifdef	notyet

#ifndef IMPLEMENTATION
#define IMPLEMENTATION ""
#endif

static char *usage_string = "\
pppd version %s patch level %d%s\n\
Usage: %s [ arguments ], where arguments are:\n\
        <device>        Communicate over the named device\n\
        <speed>         Set the baud rate to <speed>\n\
        <loc>:<rem>     Set the local and/or remote interface IP\n\
                        addresses.  Either one may be omitted.\n\
        asyncmap <n>    Set the desired async map to hex <n>\n\
        auth            Require authentication from peer\n\
        connect <p>     Invoke shell command <p> to set up the serial line\n\
        defaultroute    Add default route through interface\n\
        file <f>        Take options from file <f>\n\
        modem           Use modem control lines\n\
        mru <n>         Set MRU value to <n> for negotiation\n\
        netmask <n>     Set interface netmask to <n>\n\
See pppd(8) for more options.\n\
";
#endif	/* notyet */


/*
 * parse_args - parse a string of arguments, from the command
 * line or from a file.
 */
int
parse_args(unit, devname, local_addr, remote_addr, baud, options, fileName)
    int unit;
    char *devname;
    char *local_addr;
    char *remote_addr;
    int baud;
    PPP_OPTIONS *options;
    char *fileName;
{
    if (options) {
        if (options->flags & OPT_NO_ALL)
            noopt();

        if (options->flags & OPT_NO_ACC)
            noaccomp();

        if (options->flags & OPT_NO_ASYNCMAP)
            noasyncmap();

        if (options->flags & OPT_DEBUG)
            setdebug();

        if (options->flags & OPT_DRIVER_DEBUG)
            setkdebug();

        if (options->flags & OPT_NO_IP)
            noipaddr();

        if (options->flags & OPT_NO_MN)
            nomagicnumber();

        if (options->flags & OPT_NO_MRU)
            nomru();

        if (options->flags & OPT_PASSIVE_MODE)
            setpassive();

        if (options->flags & OPT_NO_PC)
            nopcomp();

        if (options->flags & OPT_NO_PAP)
            nopap();

        if (options->flags & OPT_NO_CHAP)
            nochap();

        if (options->flags & OPT_REQUIRE_PAP)
            reqpap();

        if (options->flags & OPT_REQUIRE_CHAP)
            reqchap();

        if (options->flags & OPT_NO_VJ)
            setnovj();

        if (options->flags & OPT_NO_VJCCOMP)
            setnovjccomp();

        if (options->flags & OPT_SILENT_MODE)
            setsilent();

        if (options->flags & OPT_DEFAULTROUTE)
            setdefaultroute();

        if (options->flags & OPT_PROXYARP)
            setproxyarp();

        if (options->flags & OPT_LOGIN)
            setdologin();

        if (options->flags & OPT_IPCP_ACCEPT_LOCAL)
            setipcpaccl();

        if (options->flags & OPT_IPCP_ACCEPT_REMOTE)
            setipcpaccr();

        if (options->asyncmap)
            if (!setasyncmap(options->asyncmap)) {
	        syslog(LOG_ERR, "asyncmap error");
	        return 0;
            }

        if (options->vj_max_slots)
            if (!setvjslots(options->vj_max_slots)) {
	        syslog(LOG_ERR, "vj_max_slots error");
	        return 0;
            }

        if (options->escape_chars)
            if (!setescape(options->escape_chars)) {
	        syslog(LOG_ERR, "escape chars error");
	        return 0;
            }

        if (options->pap_file)
            if (!setupapfile(options->pap_file)) {
	        syslog(LOG_ERR, "pap file error");
	        return 0;
            }

        if (options->chap_file)
            if (!setchapfile(options->chap_file)) {
                syslog(LOG_ERR, "chap file error");
                return 0;
            }

        if (options->mru)
            if (!setmru(options->mru)) {
	        syslog(LOG_ERR, "mru error");
	        return 0;
            }

        if (options->mtu)
            if (!setmtu(options->mtu)) {
	        syslog(LOG_ERR, "mtu error");
	        return 0;
            }

        if (options->netmask)
            if (!setnetmask(options->netmask)) {
	        syslog(LOG_ERR, "netmask error");
	        return 0;
            }

        if (options->local_auth_name)
            if (!setname(options->local_auth_name)) {
	        syslog(LOG_ERR, "local auth name error");
	        return 0;
            }

        if (options->pap_user_name)
            if (!setuser(options->pap_user_name)) {
	        syslog(LOG_ERR, "pap auth name error");
	        return 0;
            }

        if (options->pap_passwd)
            if (!setpasswd(options->pap_passwd)) {
	        syslog(LOG_ERR, "pap auth password error");
	        return 0;
            }

        if (options->remote_auth_name)
            if (!setremote(options->remote_auth_name)) {
	        syslog(LOG_ERR, "remote auth name error");
	        return 0;
            }

        if (options->lcp_echo_failure)
            if (!setlcpechofails(options->lcp_echo_failure)) {
	        syslog(LOG_ERR, "lcp echo failure error");
	        return 0;
            }

        if (options->lcp_echo_interval)
            if (!setlcpechointv(options->lcp_echo_interval)) {
	        syslog(LOG_ERR, "lcp echo interval error");
	        return 0;
            }

        if (options->lcp_restart)
            if (!setlcptimeout(options->lcp_restart)) {
	        syslog(LOG_ERR, "lcp timeout error");
	        return 0;
            }

        if (options->lcp_max_terminate)
            if (!setlcpterm(options->lcp_max_terminate)) {
	        syslog(LOG_ERR, "lcp max terminate error");
	        return 0;
            }

        if (options->lcp_max_configure)
            if (!setlcpconf(options->lcp_max_configure)) {
	        syslog(LOG_ERR, "lcp max configure error");
	        return 0;
            }

        if (options->lcp_max_failure)
            if (!setlcpfails(options->lcp_max_failure)) {
	        syslog(LOG_ERR, "lcp max failure error");
	        return 0;
            }

        if (options->ipcp_restart)
            if (!setipcptimeout(options->ipcp_restart)) {
	        syslog(LOG_ERR, "ipcp restart error");
	        return 0;
            }

        if (options->ipcp_max_terminate)
            if (!setipcpterm(options->ipcp_max_terminate)) {
	        syslog(LOG_ERR, "ipcp max terminate error");
	        return 0;
            }

        if (options->ipcp_max_configure)
            if (!setipcpconf(options->ipcp_max_configure)) {
	        syslog(LOG_ERR, "ipcp max configure error");
	        return 0;
            }

        if (options->ipcp_max_failure)
            if (!setipcpfails(options->ipcp_max_failure)) {
	        syslog(LOG_ERR, "ipcp max failure error");
	        return 0;
            }

        if (options->pap_restart)
            if (!setpaptimeout(options->pap_restart)) {
	        syslog(LOG_ERR, "pap restart error");
	        return 0;
            }

        if (options->pap_max_authreq)
            if (!setpapreqs(options->pap_max_authreq)) {
	        syslog(LOG_ERR, "pap max reqs error");
	        return 0;
            }

        if (options->chap_restart)
            if (!setchaptimeout(options->chap_restart)) {
	        syslog(LOG_ERR, "chap restart error");
	        return 0;
            }

        if (options->chap_max_challenge)
            if (!setchapchal(options->chap_max_challenge)) {
	        syslog(LOG_ERR, "chap max challenge error");
	        return 0;
            }

        if (options->chap_interval)
            if (!setchapintv(options->chap_interval)) {
	        syslog(LOG_ERR, "chap interval error");
	        return 0;
            }
    }

    if (fileName)
        if (!readfile(fileName)) {
	    syslog(LOG_ERR, "options file error");
	    return 0;
        }

    if (!setdevname(devname)) {
	syslog(LOG_ERR, "error setting device name");
	return 0;
    }

    if (!setspeed(baud, 0)) {
	syslog(LOG_ERR, "error setting baud rate");
	return 0;
    }

    if (!setipaddr(local_addr, remote_addr)) {
	syslog(LOG_ERR, "error setting address");
	return 0;
    }

    return 1;
}

/*
 * usage - print out a message telling how to use the program.
 */

#ifdef	notyet
static void
usage()
{
    logMsg(usage_string, VERSION, PATCHLEVEL, IMPLEMENTATION,
           "pppInit");
}
#endif	/* notyet */

/*
 * options_from_file - Read a string of options from a file,
 * and interpret them.
 */
int
options_from_file(fileName, must_exist, check_prot)
    char *fileName;
    int must_exist;
    int check_prot;
{
    FILE *f;
    int i, newline;
    struct cmd *cmdp;
    char *argv[MAXARGS];
    char args[MAXARGS][MAXWORDLEN];
    char cmd[MAXWORDLEN];

    if ((f = fopen(fileName, "r")) == NULL) {
	if (!must_exist)
	    return 1;
        syslog(LOG_ERR, "%s: could not open\n", fileName);
	return 0;
    }
    if (check_prot && !readable(fileno(f))) {
        syslog(LOG_ERR, "%s: access denied\n", fileName);
        fclose(f);
        return 0;
    }

    while (getword(f, cmd, &newline, fileName)) {
	/*
	 * First see if it's a command.
	 */
	for (cmdp = cmds; cmdp->cmd_name; cmdp++)
	    if (!strcmp(cmd, cmdp->cmd_name))
		break;

	if (cmdp->cmd_name != NULL) {
	    for (i = 0; i < cmdp->num_args; ++i) {
		if (!getword(f, args[i], &newline, fileName)) {
		    syslog(LOG_ERR,
			    "In file %s: too few parameters for command %s\n",
			    fileName, cmd);
		    fclose(f);
		    return 0;
		}
		argv[i] = args[i];
	    }
	    if (!(*cmdp->cmd_func)(argv[0])) {
		fclose(f);
		return 0;
	    }

	} else {
	    syslog(LOG_ERR, "In file %s: unrecognized command %s\n",
                   fileName, cmd);
            fclose(f);
            return 0;
	}
    }
    fclose(f);
    return 1;
}

#ifdef	notyet

/*
 * options_from_user - See if the use has a ~/.ppprc file,
 * and if so, interpret options from it.
 */
int
options_from_user()
{
    char *user, *path, *file;
    int ret;

    if (user[0] == 0)
        return 1;
    file = _PATH_USEROPT;
    path = (char *)malloc(strlen(user) + strlen(file) + 2);
    if (path == NULL)
        novm("init file name");
    strcpy(path, user);
    strcat(path, "/");
    strcat(path, file);
    ret = options_from_file(path, 0, 1);
    free(path);
    return ret;
}

/*
 * options_for_tty - See if an options file exists for the serial
 * device, and if so, interpret options from it.
 */
int
options_for_tty(devname)
    char *devname;
{
    char *dev, *path;
    int ret;

    dev = strrchr(devname, '/');
    if (dev == NULL)
        dev = devname;
    else
        ++dev;
    if (strcmp(dev, "tty") == 0)
        return 1;               /* don't look for /etc/ppp/options.tty */
    path = (char *)malloc(strlen(_PATH_TTYOPT) + strlen(dev) + 1);
    if (path == NULL)
        novm("tty init file name");
    strcpy(path, _PATH_TTYOPT);
    strcat(path, dev);
    ret = options_from_file(path, 0, 0);
    free(path);
    return ret;
}

#endif	/* notyet */

/*
 * readable - check if a file is readable by the real user.
 */
static int
readable(fd)
    int fd;
{
#ifdef	notyet
    uid_t uid;
    int ngroups, i;
    struct stat sbuf;
    GIDSET_TYPE groups[NGROUPS_MAX];

    uid = getuid();
    if (uid == 0)
        return 1;
    if (fstat(fd, &sbuf) != 0)
        return 0;
    if (sbuf.st_uid == uid)
        return sbuf.st_mode & S_IRUSR;
    if (sbuf.st_gid == getgid())
        return sbuf.st_mode & S_IRGRP;
    ngroups = getgroups(NGROUPS_MAX, groups);
    for (i = 0; i < ngroups; ++i)
        if (sbuf.st_gid == groups[i])
            return sbuf.st_mode & S_IRGRP;
    return sbuf.st_mode & S_IROTH;
#else	/* notyet */
    return 1;
#endif	/* notyet */
}

/*
 * Read a word from a file.
 * Words are delimited by white-space or by quotes (").
 * Quotes, white-space and \ may be escaped with \.
 * \<newline> is ignored.
 */
int
getword(f, word, newlinep, fileName)
    FILE *f;
    char *word;
    int *newlinep;
    char *fileName;
{
    int c, len, escape;
    int quoted;

    *newlinep = 0;
    len = 0;
    escape = 0;
    quoted = 0;

    /*
     * First skip white-space and comments
     */
    while ((c = getc(f)) != EOF) {
	if (c == '\\') {
	    /*
	     * \<newline> is ignored; \ followed by anything else
	     * starts a word.
	     */
	    if ((c = getc(f)) == '\n')
		continue;
	    word[len++] = '\\';
	    escape = 1;
	    break;
	}
	if (c == '\n')
	    *newlinep = 1;	/* next word starts a line */
	else if (c == '#') {
	    /* comment - ignore until EOF or \n */
	    while ((c = getc(f)) != EOF && c != '\n')
		;
	    if (c == EOF)
		break;
	    *newlinep = 1;
	} else if (!isspace(c))
	    break;
    }

    /*
     * End of file or error - fail
     */
    if (c == EOF) {
	if (ferror(f)) {
            syslog(LOG_ERR, "%s: EOF\n", fileName);
	    die(ppp_unit, 1);
	}
        *newlinep = 0;	/* added to make sure scan_authfile() exits -dzb */
	return 0;
    }

    for (;;) {
	/*
	 * Is this character escaped by \ ?
	 */
	if (escape) {
	    if (c == '\n')
		--len;			/* ignore \<newline> */
	    else if (c == '"' || isspace(c) || c == '\\')
		word[len-1] = c;	/* put special char in word */
	    else {
		if (len < MAXWORDLEN-1)
		    word[len] = c;
		++len;
	    }
	    escape = 0;
	} else if (c == '"') {
	    quoted = !quoted;
	} else if (!quoted && (isspace(c) || c == '#')) {
	    ungetc(c, f);
	    break;
	} else {
	    if (len < MAXWORDLEN-1)
		word[len] = c;
	    ++len;
	    if (c == '\\')
		escape = 1;
	}
	if ((c = getc(f)) == EOF)
	    break;
    }

    if (ferror(f)) {
        syslog(LOG_ERR, "%s: error\n", fileName);
	die(ppp_unit, 1);
    }

    if (len >= MAXWORDLEN) {
	word[MAXWORDLEN-1] = 0;
	syslog(LOG_WARNING, "warning: word in file %s too long (%.20s...)\n",
		fileName, word);
    } else
	word[len] = 0;

    return 1;
}

/*
 * number_option - parse a numeric parameter for an option
 */
static int
number_option(str, valp, base)
    char *str;
    long *valp;
    int base;
{
    char *ptr;

    *valp = strtoul(str, &ptr, base);
    if (errno == ERANGE) {
        syslog(LOG_ERR, "invalid number: %s", str);
	return 0;
    }
    return 1;
}


/*
 * int_option - like number_option, but valp is int *,
 * the base is assumed to be 0, and *valp is not changed
 * if there is an error.
 */
static int
int_option(str, valp)
    char *str;
    int *valp;
{
    long v;

    if (!number_option(str, &v, 0))
	return 0;
    *valp = (int) v;
    return 1;
}


/*
 * The following procedures execute commands.
 */

/*
 * readfile - take commands from a file.
 */
static int
readfile(argv)
    char *argv;
{
    return options_from_file(argv, 1, 1);
}

/*
 * setdebug - Set debug (command line argument).
 */
static int
setdebug()
{
    ppp_if[ppp_unit]->debug = 1;
    return (1);
}

/*
 * setkdebug - Set kernel debugging level.
 */
static int
setkdebug()
{
    ppp_if[ppp_unit]->kdebugflag = 1;
    return (1);
}

/*
 * noopt - Disable all options.
 */
static int
noopt()
{
    BZERO((char *) &ppp_if[ppp_unit]->lcp_wantoptions,
          sizeof (struct lcp_options));
    BZERO((char *) &ppp_if[ppp_unit]->lcp_allowoptions,
          sizeof (struct lcp_options));
    BZERO((char *) &ppp_if[ppp_unit]->ipcp_wantoptions,
          sizeof (struct ipcp_options));
    BZERO((char *) &ppp_if[ppp_unit]->ipcp_allowoptions,
          sizeof (struct ipcp_options));
    return (1);
}

/*
 * noaccomp - Disable Address/Control field compression negotiation.
 */
static int
noaccomp()
{
    ppp_if[ppp_unit]->lcp_wantoptions.neg_accompression = 0;
    ppp_if[ppp_unit]->lcp_allowoptions.neg_accompression = 0;
    return (1);
}


/*
 * noasyncmap - Disable async map negotiation.
 */
static int
noasyncmap()
{
    ppp_if[ppp_unit]->lcp_wantoptions.neg_asyncmap = 0;
    ppp_if[ppp_unit]->lcp_allowoptions.neg_asyncmap = 0;
    return (1);
}


/*
 * noipaddr - Disable IP address negotiation.
 */
static int
noipaddr()
{
    ppp_if[ppp_unit]->ipcp_wantoptions.neg_addr = 0;
    ppp_if[ppp_unit]->ipcp_allowoptions.neg_addr = 0;
    return (1);
}


/*
 * nomagicnumber - Disable magic number negotiation.
 */
static int
nomagicnumber()
{
    ppp_if[ppp_unit]->lcp_wantoptions.neg_magicnumber = 0;
    ppp_if[ppp_unit]->lcp_allowoptions.neg_magicnumber = 0;
    return (1);
}


/*
 * nomru - Disable mru negotiation.
 */
static int
nomru()
{
    ppp_if[ppp_unit]->lcp_wantoptions.neg_mru = 0;
    ppp_if[ppp_unit]->lcp_allowoptions.neg_mru = 0;
    return (1);
}


/*
 * setmru - Set MRU for negotiation.
 */
static int
setmru(argv)
    char *argv;
{
    long mru;

    if (!number_option(argv, &mru, 0))
        return 0;
    ppp_if[ppp_unit]->lcp_wantoptions.mru = mru;
    ppp_if[ppp_unit]->lcp_wantoptions.neg_mru = 1;
    return (1);
}


/*
 * setmtu - Set the largest MTU we'll use.
 */
static int
setmtu(argv)
    char *argv;
{
    long mtu;

    if (!number_option(argv, &mtu, 0))
        return 0;
    if (mtu < MINMRU || mtu > MAXMRU) {
        syslog(LOG_ERR, "mtu option value of %d is too %s\n", mtu,
                (mtu < MINMRU? "small": "large"));
        return 0;
    }
    ppp_if[ppp_unit]->lcp_allowoptions.mru = mtu;
    return (1);
}


/*
 * nopcomp - Disable Protocol field compression negotiation.
 */
static int
nopcomp()
{
    ppp_if[ppp_unit]->lcp_wantoptions.neg_pcompression = 0;
    ppp_if[ppp_unit]->lcp_allowoptions.neg_pcompression = 0;
    return (1);
}


/*
 * setpassive - Set passive mode (don't give up if we time out sending
 * LCP configure-requests).
 */
static int
setpassive()
{
    ppp_if[ppp_unit]->lcp_wantoptions.passive = 1;
    return (1);
}


/*
 * setsilent - Set silent mode (don't start sending LCP configure-requests
 * until we get one from the peer).
 */
static int
setsilent()
{
    ppp_if[ppp_unit]->lcp_wantoptions.silent = 1;
    return (1);
}


/*
 * nopap - Disable PAP authentication with peer.
 */
static int
nopap()
{
    ppp_if[ppp_unit]->lcp_allowoptions.neg_upap = 0;
    return (1);
}


/*
 * reqpap - Require PAP authentication from peer.
 */
static int
reqpap()
{
    ppp_if[ppp_unit]->lcp_wantoptions.neg_upap = 1;
    ppp_if[ppp_unit]->auth_required = 1;
    return (1);
}

/*
 * setupapfile - specifies UPAP info for authenticating with peer.
 */
static int
setupapfile(argv)
    char *argv;
{
    FILE * ufile;

    ppp_if[ppp_unit]->lcp_allowoptions.neg_upap = 1;
    ppp_if[ppp_unit]->options->pap_file = (char *)stringdup(argv);

    /* open user info file */
    if ((ufile = fopen(argv, "r")) == NULL) {
	syslog(LOG_ERR, "unable to open PAP secrets file %s", argv);
	return 0;
    }
    if (!readable(fileno(ufile))) {
        syslog(LOG_ERR, "%s: access denied", argv);
        fclose(ufile);
        return 0;
    }
    check_access(ufile, argv);

    fclose(ufile);
    return (1);
}


/*
 * nochap - Disable CHAP authentication with peer.
 */
static int
nochap()
{
    ppp_if[ppp_unit]->lcp_allowoptions.neg_chap = 0;
    return (1);
}


/*
 * reqchap - Require CHAP authentication from peer.
 */
static int
reqchap()
{
    ppp_if[ppp_unit]->lcp_wantoptions.neg_chap = 1;
    ppp_if[ppp_unit]->auth_required = 1;
    return (1);
}


/*
 * setchapfile - specifies CHAP info for authenticating with peer.
 */
static int
setchapfile(argv)
char *argv;
{
    FILE * ufile;

    ppp_if[ppp_unit]->lcp_allowoptions.neg_chap = 1;
    ppp_if[ppp_unit]->options->chap_file = (char *)stringdup(argv);

    /* open user info file */
    if ((ufile = fopen(argv, "r")) == NULL) {
	syslog(LOG_ERR, "unable to open CHAP secrets file %s", argv);
	return 0;
    }
    if (!readable(fileno(ufile))) {
        syslog(LOG_ERR, "%s: access denied", argv);
        fclose(ufile);
        return 0;
    }
    check_access(ufile, argv);

    fclose(ufile);
    return (1);
}

/*
 * setnovj - disable vj compression
 */
static int
setnovj()
{
    ppp_if[ppp_unit]->ipcp_wantoptions.neg_vj = 0;
    ppp_if[ppp_unit]->ipcp_allowoptions.neg_vj = 0;
    return (1);
}


/*
 * setnovjccomp - disable VJ connection-ID compression
 */
static int
setnovjccomp()
{
    ppp_if[ppp_unit]->ipcp_wantoptions.cflag = 0;
    ppp_if[ppp_unit]->ipcp_allowoptions.cflag = 0;
    return (1);
}


/*
 * setvjslots - set maximum number of connection slots for VJ compression
 */
static int
setvjslots(argv)
    char *argv;
{
    int value;

    if (!int_option(argv, &value))
        return 0;
    if (value < 2 || value > 16) {
        syslog(LOG_ERR, "pppd: vj-max-slots value must be between 2 and 16");
        return 0;
    }
    ppp_if[ppp_unit]->ipcp_wantoptions.maxslotindex =
        ppp_if[ppp_unit]->ipcp_allowoptions.maxslotindex = value - 1;
    return 1;
}

#ifdef	notyet
/*
 * setconnector - Set a program to connect to a serial line
 */
static int
setconnector(argv)
    char *argv;
{
    ppp_if[ppp_unit]->connector = (char *)stringdup(argv);
    if (ppp_if[ppp_unit]->connector == NULL)
	novm("connector string");

    return (1);
}
#endif	/* notyet */

#ifdef	notyet
/*
 * setdisconnector - Set a program to disconnect from the serial line
 */
static int
setdisconnector(argv)
    char *argv;
{
    ppp_if[ppp_unit]->disconnector = (char *)stringdup(argv);
    if (ppp_if[ppp_unit]->disconnector == NULL)
        novm("disconnector string");

    return (1);
}
#endif	/* notyet */

#ifdef	notyet
/*
 * setdomain - Set domain name to append to hostname 
 */
static int
setdomain(argv)
    char *argv;
{
    strncat(ppp_if[ppp_unit]->hostname, argv,
	    MAXNAMELEN - strlen(ppp_if[ppp_unit]->hostname));
    ppp_if[ppp_unit]->hostname[MAXNAMELEN-1] = 0;
    return (1);
}
#endif	/* notyet */


/*
 * setasyncmap - add bits to asyncmap (what we request peer to escape).
 */
static int
setasyncmap(argv)
    char *argv;
{
    long asyncmap;

    if (!number_option(argv, &asyncmap, 16))
	return 0;
    ppp_if[ppp_unit]->lcp_wantoptions.asyncmap |= asyncmap;
    ppp_if[ppp_unit]->lcp_wantoptions.neg_asyncmap = 1;
    return (1);
}

/*
 * setescape - add chars to the set we escape on transmission.
 */
static int
setescape(argv)
    char *argv;
{
    int n, ret;
    char *p, *endp;

    p = argv;
    ret = 1;
    while (*p) {
        n = strtoul(p, &endp, 16);
        if (p == endp) {
            syslog(LOG_ERR, "invalid hex number: %s", p);
            return 0;
        }
        p = endp;
        if (n < 0 || (0x20 <= n && n <= 0x3F) || n == 0x5E || n > 0xFF) {
            syslog(LOG_ERR, "can't escape character 0x%x", n);
            ret = 0;
        } else
            ppp_if[ppp_unit]->xmit_accm[n >> 5] |= 1 << (n & 0x1F);
        while (*p == ',' || *p == ' ')
            ++p;
    }
    return ret;
}


/*
 * setspeed - Set the speed.
 */
static int
setspeed(arg, is_string)
    int arg;
    int is_string;
{
    char *ptr;
    int spd;

    if (is_string) {
        spd = strtol((char *)arg, &ptr, 0);
        if (ptr == (char *)arg || *ptr != 0 || spd == 0)
	    return 0;
        ppp_if[ppp_unit]->inspeed = spd;
    }
    else 
	ppp_if[ppp_unit]->inspeed = arg;
    
    return 1;
}


/*
 * setdevname - Set the device name.
 */
int
setdevname(cp)
    char *cp;
{
    (void) strncpy(ppp_if[ppp_unit]->devname, cp, MAXPATHLEN);
    ppp_if[ppp_unit]->devname[MAXPATHLEN-1] = 0;
  
    return 1;
}


/*
 * setipaddr - Set the IP address
 *
 * This function also accepts hostnames apart from ip address strings.
 * If hostnames are passed to this function, the hostnames should be
 * added to the system by making calls to hostAdd() prior to calling this
 * function. This requires the hostTblInit be called before hostGetByName
 * is called.
 */
int
setipaddr(local_addr, remote_addr)
    char *local_addr;
    char *remote_addr;
{
    u_long local, remote;
    ipcp_options *wo = &ppp_if[ppp_unit]->ipcp_wantoptions;

    if ((local = hostGetByName (local_addr)) == ERROR)
	{
	if ((((local = inet_addr(local_addr)) == ERROR)) || 
	    (bad_ip_adrs(local)))
	    {
	    syslog(LOG_ERR, "bad local IP address: %s", ip_ntoa(local));
	    return 0;
	    }
	}
    else
	{
	strncpy(ppp_if[ppp_unit]->our_name, local_addr, MAXNAMELEN);
	ppp_if[ppp_unit]->our_name[MAXNAMELEN-1] = 0;
	}

    if (local != 0)
        wo->ouraddr = local;

    if ((remote = hostGetByName (remote_addr)) == ERROR)
	{
	if ((((remote = inet_addr(remote_addr)) == ERROR)) || 
	    (bad_ip_adrs(remote)))
	    {
	    syslog(LOG_ERR, "bad remote IP address: %s", ip_ntoa(remote));
	    return 0;
	    }
	}
    else
	{
	strncpy(ppp_if[ppp_unit]->remote_name, remote_addr, MAXNAMELEN);
	ppp_if[ppp_unit]->remote_name[MAXNAMELEN-1] = 0;
	}

    if (remote != 0)
        wo->hisaddr = remote;

    return 1;
}

#ifdef	notyet
/*
 * setnoipdflt - disable setipdefault()
 */
static int
setnoipdflt()
{
    ppp_if[ppp_unit]->disable_defaultip = 1;
    return 1;
}
#endif	/* notyet */


/*
 * setipcpaccl - accept peer's idea of our address
 */
static int
setipcpaccl()
{
    ppp_if[ppp_unit]->ipcp_wantoptions.accept_local = 1;
    return 1;
}

/*
 * setipcpaccr - accept peer's idea of its address
 */
static int
setipcpaccr()
{
    ppp_if[ppp_unit]->ipcp_wantoptions.accept_remote = 1;
    return 1;
}


/*
 * setipdefault - default our local IP address based on our hostname.
 */
void
setipdefault()
{
    u_long local;
    ipcp_options *wo = &ppp_if[ppp_unit]->ipcp_wantoptions;

    /*
     * If local IP address already given, don't bother.
     */
    if (wo->ouraddr != 0 || ppp_if[ppp_unit]->disable_defaultip)
	return;

    /*
     * Look up our hostname (possibly with domain name appended)
     * and take the first IP address as our local IP address.
     * If there isn't an IP address for our hostname, too bad.
     */
    wo->accept_local = 1;       /* don't insist on this default value */
    if ((local = hostGetByName(ppp_if[ppp_unit]->hostname)) == ERROR)
	return;
    if (local != 0 && !bad_ip_adrs(local))
	wo->ouraddr = local;
}


/*
 * setnetmask - set the netmask to be used on the interface.
 */
static int
setnetmask(argv)
    char *argv;
{
    u_long mask;
	
    if ((mask = inet_addr(argv)) == ERROR || (ppp_if[ppp_unit]->netmask & ~mask) != 0) {
	syslog(LOG_ERR, "Invalid netmask %s\n", argv);
	return 0;
    }

    ppp_if[ppp_unit]->netmask = mask;
    return (1);
}

/*
 * Return user specified netmask. A value of zero means no netmask has
 * been set.
 */
/* ARGSUSED */
u_long
GetMask(addr)
    u_long addr;
{
    return(ppp_if[ppp_unit]->netmask);
}

static int
setname(argv)
    char *argv;
{
    if (ppp_if[ppp_unit]->our_name[0] == 0) {
	strncpy(ppp_if[ppp_unit]->our_name, argv, MAXNAMELEN);
	ppp_if[ppp_unit]->our_name[MAXNAMELEN-1] = 0;
    }
    return 1;
}

static int
setuser(argv)
    char *argv;
{
    strncpy(ppp_if[ppp_unit]->user, argv, MAXNAMELEN);
    ppp_if[ppp_unit]->user[MAXNAMELEN-1] = 0;
    return 1;
}

static int
setpasswd(argv)
    char *argv;
{
    strncpy(ppp_if[ppp_unit]->passwd, argv, MAXSECRETLEN);
    ppp_if[ppp_unit]->passwd[MAXSECRETLEN-1] = 0;
    return 1;
}

static int
setremote(argv)
    char *argv;
{
    strncpy(ppp_if[ppp_unit]->remote_name, argv, MAXNAMELEN);
    ppp_if[ppp_unit]->remote_name[MAXNAMELEN-1] = 0;
    return 1;
}

#ifdef	notyet
static int
setauth()
{
    ppp_if[ppp_unit]->auth_required = 1;
    return 1;
}
#endif	/* notyet */

static int
setdefaultroute()
{
    ppp_if[ppp_unit]->ipcp_wantoptions.default_route = 1;
    return 1;
}

static int
setproxyarp()
{
    ppp_if[ppp_unit]->ipcp_wantoptions.proxy_arp = 1;
    return 1;
}

static int
setdologin()
{
    ppp_if[ppp_unit]->uselogin = 1;
    return 1;
}

/*
 * Functions to set the echo interval for modem-less monitors
 */

static int
setlcpechointv(argv)
    char *argv;
{
    return int_option(argv, &ppp_if[ppp_unit]->lcp_echo_interval);
}

static int
setlcpechofails(argv)
    char *argv;
{
    return int_option(argv, &ppp_if[ppp_unit]->lcp_echo_fails);
}

/*
 * Functions to set timeouts, max transmits, etc.
 */
static int
setlcptimeout(argv)
    char *argv;
{
    return int_option(argv, &ppp_if[ppp_unit]->lcp_fsm.timeouttime);
}

static int setlcpterm(argv)
    char *argv;
{
    return int_option(argv, &ppp_if[ppp_unit]->lcp_fsm.maxtermtransmits);
}

static int setlcpconf(argv)
    char *argv;
{
    return int_option(argv, &ppp_if[ppp_unit]->lcp_fsm.maxconfreqtransmits);
}

static int setlcpfails(argv)
    char *argv;
{
    return int_option(argv, &ppp_if[ppp_unit]->lcp_fsm.maxnakloops);
}

static int setipcptimeout(argv)
    char *argv;
{
    return int_option(argv, &ppp_if[ppp_unit]->ipcp_fsm.timeouttime);
}

static int setipcpterm(argv)
    char *argv;
{
    return int_option(argv, &ppp_if[ppp_unit]->ipcp_fsm.maxtermtransmits);
}

static int setipcpconf(argv)
    char *argv;
{
    return int_option(argv, &ppp_if[ppp_unit]->ipcp_fsm.maxconfreqtransmits);
}

static int setipcpfails(argv)
    char *argv;
{
    return int_option(argv, &ppp_if[ppp_unit]->ipcp_fsm.maxnakloops);
}

static int setpaptimeout(argv)
    char *argv;
{
    return int_option(argv, &ppp_if[ppp_unit]->upap.us_timeouttime);
}

static int setpapreqs(argv)
    char *argv;
{
    return int_option(argv, &ppp_if[ppp_unit]->upap.us_maxtransmits);
}

static int setchaptimeout(argv)
    char *argv;
{
    return int_option(argv, &ppp_if[ppp_unit]->chap.timeouttime);
}

static int setchapchal(argv)
    char *argv;
{
    return int_option(argv, &ppp_if[ppp_unit]->chap.max_transmits);
}

static int setchapintv(argv)
    char *argv;
{
    return int_option(argv, &ppp_if[ppp_unit]->chap.chal_interval);
}

/* res_query.c - DNS Client query module */

/* Copyright 1984-2001 Wind River Systems, Inc. */

/*
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
modification history
--------------------
01b,17sep01,vvv  fixed compilation warnings
01a,13dec96,jag  Eliminated call to initialization and support for the file
                 HOSTALIASES.
*/

#include <resolvLib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#if PACKETSZ > 1024
#define MAXPACKET	PACKETSZ
#else
#define MAXPACKET	1024
#endif

#ifdef ORG_RESOLVER
char *__hostalias __P((const char *));
#endif /* ORG_RESOLVER */


#ifdef ORG_RESOLVER
int h_errno;
#endif

/* locals */

int res_querydomain (char *name, char *domain, int class, int type,
		     u_char *answer, int anslen);

/*
 * Formulate a normal query, send, and await answer.
 * Returned answer is placed in supplied buffer "answer".
 * Perform preliminary check of answer, returning success only
 * if no error is indicated and the answer count is nonzero.
 * Return the size of the response on success, ERROR on error.
 * Error number is left in h_errno.
 * Caller must parse answer and determine whether it answers the question.
 */

int
resolvQuery(name, class, type, answer, anslen)
	char *name;		/* domain name */
	int class, type;	/* class and type of query */
	u_char *answer;		/* buffer to put answer */
	int anslen;		/* size of answer buffer */
{
	char buf[MAXPACKET];
	HEADER *hp;
	int n;

#ifdef ORG_RESOLVER
	/* 
	 * Unecessary test under VxWorks; initialization is done from
	 * usrNetwork.c
	 */
	if ((_res.options & RES_INIT) == 0 && res_init() == -1)
		return (ERROR);
#endif /* ORG_RESOLVER */

#ifdef DEBUG
	if (_res.options & RES_DEBUG)
		printf(";; resolvQuery(%s, %d, %d)\n", name, class, type);
#endif
	n = resolvMkQuery(QUERY, name, class, type, (char *)NULL, 0, NULL,
	    buf, sizeof(buf));

	if (n == ERROR) {
#ifdef DEBUG
		if (_res.options & RES_DEBUG)
			printf(";; resolvQuery: mkquery failed\n");
#endif
		errno = S_resolvLib_NO_RECOVERY;
		return (n);
	}
	n = resolvSend(buf, n, (char *)answer, anslen);
	if (n == ERROR) {
#ifdef DEBUG
		if (_res.options & RES_DEBUG)
			printf(";; resolvQuery: send error\n");
#endif
		errno = S_resolvLib_TRY_AGAIN;
		return (n);
	}

	hp = (HEADER *) answer;
	if (hp->rcode != NOERROR || ntohs(hp->ancount) == 0) {
#ifdef DEBUG
		if (_res.options & RES_DEBUG)
			printf(";; rcode = %d, ancount=%d\n", hp->rcode,
			    ntohs(hp->ancount));
#endif
		switch (hp->rcode) {
			case NXDOMAIN:
				errno = S_resolvLib_HOST_NOT_FOUND;
				break;
			case SERVFAIL:
				errno = S_resolvLib_TRY_AGAIN;
				break;
			case NOERROR:
				errno = S_resolvLib_NO_DATA;
				break;
			case FORMERR:
			case NOTIMP:
			case REFUSED:
			default:
				errno = S_resolvLib_NO_RECOVERY;
				break;
		}
		return (ERROR);
	}
	return (n);
}

/*
 * Formulate a normal query, send, and retrieve answer in supplied buffer.
 * Return the size of the response on success, -1 on error.
 * If enabled, implement search rules until answer or unrecoverable failure
 * is detected.  Error number is left in h_errno.
 * Only useful for queries in the same name hierarchy as the local host
 * (not, for example, for host address-to-name lookups in domain in-addr.arpa).
 */
int
resSearch(name, class, type, answer, anslen)
	const char *name;	/* domain name */
	int class, type;	/* class and type of query */
	u_char *answer;		/* buffer to put answer */
	int anslen;		/* size of answer */
{
	register char *cp, **domain;
	int dots, trailing_dot, ret, got_nodata, saved_herrno, tried_as_is;

#if ORG_RESOLVER	
	/* Unecessary test under VxWorks */
	if ((_res.options & RES_INIT) == 0 && res_init() == -1)
		return (-1);
#endif /* ORG_RESOLVER */


	got_nodata = 0;

#ifdef ORG_RESOLVER
	errno = 0;
#endif /* ORG_RESOLVER */

	errno = S_resolvLib_HOST_NOT_FOUND;	/* default, if we never query */
	dots = 0;
	for (cp = (char *)name; *cp; cp++) {
		if (*cp == '.')
			dots++;
	}
	trailing_dot = 0;
	if ((cp > name) && (*--cp == '.'))
		trailing_dot++;

#ifdef ORG_RESOLVER
	/* In VxWorks we don't support the HOSTALIASES environment variable
	 * or the file pointed by HOSTALIASES.  This concept is described in
	 * the BSD man page for hostname. For now we just ignore this option.
	 */
	/*
	 * if there aren't any dots, it could be a user-level alias
	 */
	if (!dots && (cp = __hostalias(name)))
		return (resolvQuery(cp, class, type, answer, anslen));
#endif /* ORG_RESOLVER */

	/*
	 * If there are dots in the name already, let's just give it a try
	 * 'as is'.  The threshold can be set with the "ndots" option.
	 */
	saved_herrno = -1;
	tried_as_is = 0;
	if (dots >= _res.ndots) {
		ret = res_querydomain((char *)name, NULL, class, type, answer, 
				      anslen);
		if (ret > 0)
			return (ret);
		saved_herrno = errno;
		tried_as_is++;
	}

	/*
	 * We do at least one level of search if
	 *	- there is no dot and RES_DEFNAME is set, or
	 *	- there is at least one dot, there is no trailing dot,
	 *	  and RES_DNSRCH is set.
	 */
	if ((!dots && (_res.options & RES_DEFNAMES)) ||
	    (dots && !trailing_dot && (_res.options & RES_DNSRCH))) {
		for (domain = _res.dnsrch; *domain; domain++) {
			int done = 0;

			ret = res_querydomain((char *)name, *domain, class, type,
			    answer, anslen);
			if (ret > 0)
				return (ret);
			/*
			 * If no server present, give up.
			 * If name isn't found in this domain,
			 * keep trying higher domains in the search list
			 * (if that's enabled).
			 * On a NO_DATA error, keep trying, otherwise
			 * a wildcard entry of another type could keep us
			 * from finding this entry higher in the domain.
			 * If we get some other error (negative answer or
			 * server failure), then stop searching up,
			 * but try the input name below in case it's
			 * fully-qualified.
			 */
			if (errno == ECONNREFUSED) {
				errno = S_resolvLib_TRY_AGAIN;
				return (-1);
			}

			switch (errno) {
			case NO_DATA:
				got_nodata++;
				/* FALLTHROUGH */
			case HOST_NOT_FOUND:
				/* keep trying */
				break;
			default:
				/* anything else implies that we're done */
				done++;
			}
			/*
			 * if we got here for some reason other than DNSRCH,
			 * we only wanted one iteration of the loop, so stop.
			 */
			if (!(_res.options & RES_DNSRCH))
			        done++;

			if (done)
				break;
		}
	}

	/*
	 * if we have not already tried the name "as is", do that now.
	 * note that we do this regardless of how many dots were in the
	 * name or whether it ends with a dot.
	 */
	if (!tried_as_is) {
		ret = res_querydomain((char *)name, NULL, class, type, answer, 
				      anslen);
		if (ret > 0)
			return (ret);
		saved_herrno = errno;
	}

	/*
	 * if we got here, we didn't satisfy the search.
	 * if we did an initial full query, return that query's h_errno
	 * (note that we wouldn't be here if that query had succeeded).
	 * else if we ever got a nodata, send that back as the reason.
	 * else send back meaningless h_errno, that being the one from
	 * the last DNSRCH we did.
	 */
	if (saved_herrno != -1)
		errno = saved_herrno;
	else if (got_nodata)
		errno = S_resolvLib_NO_DATA;
	return (-1);
}

/*
 * Perform a call on resolvQuery on the concatenation of name and domain,
 * removing a trailing dot from name if domain is NULL.
 */
int
res_querydomain(name, domain, class, type, answer, anslen)
	char *name, *domain;
	int class, type;	/* class and type of query */
	u_char *answer;		/* buffer to put answer */
	int anslen;		/* size of answer */
{
	char nbuf[2*MAXDNAME+2];
	char *longname = nbuf;
	int n;

#ifdef DEBUG
	if (_res.options & RES_DEBUG)
		printf(";; res_querydomain(%s, %s, %d, %d)\n",
		    name, domain, class, type);
#endif
	if (domain == NULL) {
		/*
		 * Check for trailing '.';
		 * copy without '.' if present.
		 */
		n = strlen(name) - 1;
		if (n != (0 - 1) && name[n] == '.' && n < sizeof(nbuf) - 1) {
			bcopy(name, nbuf, n);
			nbuf[n] = '\0';
		} else
			longname = name;
	} else
		(void)sprintf(nbuf, "%.*s.%.*s",
		    MAXDNAME, name, MAXDNAME, domain);

	return (resolvQuery(longname, class, type, answer, anslen));
}

#ifdef ORG_RESOLVER

/* In VxWorks we don't support the HOSTALIASES environment variable
 * or the file pointed by HOSTALIASES.  This concept is described in
 * the BSD man page for hostname. For now we just ignore this option.  This
 * function is only invoked in this module.
 */

char *
__hostalias(name)
	register const char *name;
{
	register char *cp1, *cp2;
	FILE *fp;
	char *file, *getenv(), *strcpy(), *strncpy();
	char buf[BUFSIZ];
	static char abuf[MAXDNAME];

	file = getenv("HOSTALIASES");
	if (file == NULL || (fp = fopen(file, "r")) == NULL)
		return (NULL);
	buf[sizeof(buf) - 1] = '\0';
	while (fgets(buf, sizeof(buf), fp)) {
		for (cp1 = buf; *cp1 && !isspace(*cp1); ++cp1);
		if (!*cp1)
			break;
		*cp1 = '\0';
		if (!strcasecmp(buf, name)) {
			while (isspace(*++cp1));
			if (!*cp1)
				break;
			for (cp2 = cp1 + 1; *cp2 && !isspace(*cp2); ++cp2);
			abuf[sizeof(abuf) - 1] = *cp2 = '\0';
			(void)strncpy(abuf, cp1, sizeof(abuf) - 1);
			fclose(fp);
			return (abuf);
		}
	}
	fclose(fp);
	return (NULL);
}
#endif /* ORG_RESOLVER */

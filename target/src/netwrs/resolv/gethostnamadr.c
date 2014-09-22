/* gethostnamadr.c - */

/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"


/*
 * Copyright (c) 1985, 1988 Regents of the University of California.
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
01d,05nov01,vvv  fixed compilation warnings
01c,04sep01,vvv  fixed to correctly query multiple servers (SPR #67238);
		 fixed compilation warnings
01b,11feb96,jag  Fix the formating of the hostent structure in  _gethostbyname
                 when invoked with an IP address. Cleaned up warnings.
01a,13dec96,jag  Cleaned up and rename user I/F functions with the resolv
		 prefix.  Man pages for these functions appear in resolvLib
*/

#include <vxWorks.h>
#include <resolvLib.h>
#include <semLib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <inetLib.h>


#define  MAX_RR_TTL  172800         /* Maximum of Two days in seconds */

#if PACKETSZ > 1024
#define	MAXPACKET	PACKETSZ
#else
#define	MAXPACKET	1024
#endif

typedef union {
	HEADER hdr;
	u_char buf[MAXPACKET];
} querybuf;

typedef union {
	int32_t	al;
	char ac;
} align;

static int qcomp (struct in_addr **, struct in_addr **);
LOCAL struct hostent *getanswer (querybuf *, int, int, struct hostent *, 
				 char *, int);

LOCAL struct hostent *
getanswer(answer, anslen, iquery, pHost, hostbuf, bufLen)
	querybuf *answer;
	int anslen;
	int iquery;
	struct hostent   *  pHost;
	char *           hostbuf;
	int              bufLen;
{
	register HEADER *hp;
	register u_char *cp;
	register int n;
	u_char *eom;
	char *bp, **ap;
	int type, class, ancount, qdcount;
	int haveanswer, getclass = C_ANY;
	char **hap;
	char *hostbufEnd;
	unsigned int rrttl;		/* RR time to live field */

	eom = answer->buf + anslen;
	/*
	 * find first satisfactory answer
	 */
	hp = &answer->hdr;
	ancount = ntohs(hp->ancount);
	qdcount = ntohs(hp->qdcount);

	bp = hostbuf;

        hostbufEnd = hostbuf + bufLen - 1;

	cp = answer->buf + sizeof(HEADER);

	/* Process the "questions section" of the DNS response */
	if (qdcount) {
			    /* Check if this is a Pointer query */
		if (iquery) {
			if ((n = resolvDNExpand((u_char *)answer->buf,
			    (u_char *)eom, (u_char *)cp, (u_char *)bp,
			    bufLen)) < 0) {
				errno = S_resolvLib_NO_RECOVERY;
				return ((struct hostent *) NULL);
			}
			cp += n + QFIXEDSZ;
			pHost->h_name = bp;
			n = strlen(bp) + 1;
			bp += n;
			bufLen -= n;
		} else
			cp += __dn_skipname(cp, eom) + QFIXEDSZ;
		while (--qdcount > 0)
			cp += __dn_skipname(cp, eom) + QFIXEDSZ;
	} else if (iquery) {
		if (hp->aa)
			errno = S_resolvLib_HOST_NOT_FOUND;
		else
			errno = S_resolvLib_TRY_AGAIN;
		return ((struct hostent *) NULL);
	}

	/* Set everything up to process the resource records */

	ap = pHost->h_aliases;
	*ap = NULL;
	

	hap = pHost->h_addr_list;
	*hap = NULL;

	pHost->h_ttl = MAX_RR_TTL;  /* Maximum of Two days in seconds */

	haveanswer = 0;
	while (--ancount >= 0 && cp < eom) {
		if ((n = resolvDNExpand((u_char *)answer->buf, (u_char *)eom,
		    (u_char *)cp, (u_char *)bp, bufLen)) < 0)
			break;
		cp += n;
		type = _getshort(cp);		/* Get RR type field */
 		cp += sizeof(uint16_t);         /* Skip size of type field */
		class = _getshort(cp);		/* Get RR class field */
 		cp += sizeof(uint16_t);         /* Skip size of class field */
		rrttl = _getlong(cp);	        /* Get RR TTL field */
		cp += sizeof(uint32_t);	        /* Skip size of TTL field */
		n = _getshort(cp);		/* Get RR data length field */
		cp += sizeof(uint16_t);		/* Skip size of length filed */

		if (type == T_CNAME) {
			cp += n;
			if (ap >= & pHost->h_aliases [MAXALIASES-1])
				continue;
			*ap++ = bp;
			n = strlen(bp) + 1;
			bp += n;
			bufLen -= n;

			if (rrttl < pHost->h_ttl)
			    pHost->h_ttl = rrttl;

			continue;
		}
		if (iquery && type == T_PTR) {
			if ((n = resolvDNExpand((u_char *)answer->buf,
			    (u_char *)eom, (u_char *)cp, (u_char *)bp,
			    bufLen)) < 0)
				break;
			cp += n;
			pHost->h_name = bp;

			if (rrttl < pHost->h_ttl)
			    pHost->h_ttl = rrttl;

			return (pHost);
		}
		if (iquery || type != T_A)  {
#ifdef DEBUG
			if (_res.options & RES_DEBUG)
				printf("unexpected answer type %d, size %d\n",
					type, n);
#endif
			cp += n;
			continue;
		}
		if (haveanswer) {
			if (n != pHost->h_length) {
				cp += n;
				continue;
			}
			if (class != getclass) {
				cp += n;
				continue;
			}
		} else {
			pHost->h_length = n;
			getclass = class;
			pHost->h_addrtype = (class == C_IN) ? AF_INET : AF_UNSPEC;
			if (!iquery) {
			    pHost->h_name = bp;
			    bp += strlen(bp) + 1;

			    if (rrttl < pHost->h_ttl)
				pHost->h_ttl = rrttl;
			}
		}

		bp += sizeof(align) - ((u_long)bp % sizeof(align));

		if (bp + n >= hostbufEnd) {
#ifdef DEBUG
			if (_res.options & RES_DEBUG)
				printf("size (%d) too big\n", n);
#endif
			break;
		}
		bcopy(cp, *hap++ = bp, n);
		bp +=n;
		cp += n;
		haveanswer++;
	}
	if (haveanswer) {
		*ap = NULL;
		*hap = NULL;
		if (_res.nsort) {
			qsort(pHost->h_addr_list, haveanswer,
			    sizeof(struct in_addr),
			    (int (*)__P((const void *, const void *)))qcomp);
		}
		return (pHost);
	} else {
		errno = S_resolvLib_TRY_AGAIN;
		return ((struct hostent *) NULL);
	}
}

struct hostent *
_gethostbyname(name, pHost, hostbuf, bufLen)
	const char *name;
	struct hostent *pHost;
        char        *hostbuf;
        int         bufLen;
{
	querybuf buf;
	register const char *cp;
	int n, i;
	register struct hostent *hp;
	char lookups[MAXDNSLUS];

	/*
	 * disallow names consisting only of digits/dots, unless
	 * they end in a dot.
	 */
	if (isdigit((int) name[0]))
		for (cp = name;; ++cp) {
			if (!*cp) {
				if (*--cp == '.')
					break;
				/*
				 * All-numeric, no dot at the end.
				 * Fake up a hostent as if we'd actually
				 * done a lookup.
				 */
				pHost->h_addr_list[0] = hostbuf;
				hostbuf += sizeof(uint32_t);
							
				if (inet_aton((char *) name, 
					      (struct in_addr *) pHost->h_addr_list[0]) 
					   == ERROR) 
				    {
					errno = S_resolvLib_HOST_NOT_FOUND;

					return((struct hostent *) NULL);
				    }

				pHost->h_name = (char *)name;
				pHost->h_aliases[0] = NULL;
				pHost->h_addrtype = AF_INET;
				pHost->h_length = sizeof(uint32_t);
				pHost->h_addr_list [1] = NULL;

				/* Maximum of Two days in seconds */
				pHost->h_ttl = MAX_RR_TTL;


				return (pHost);
			}
			if (!isdigit((int) *cp) && *cp != '.') 
				break;
		}

	bcopy(_res.lookups, lookups, sizeof lookups);

	if (lookups[0] == '\0')
		strncpy(lookups, "bf", sizeof lookups);

	hp = (struct hostent *)NULL;
	
	for (i = 0; i < MAXDNSLUS && hp == NULL && lookups[i]; i++) {

		/*
		 * if resolvSend has already contacted this server, lookup
		 * is 'd' indicating 'done'. In this case the server will
		 * not be contacted again and lookup is reset.
		 */

		if (_res.lookups [i] == 'd')
		    {
		    _res.lookups [i] = 'b';
		    continue;
		    }

		switch (lookups[i]) {
		case 'b':

			/*
		         * Set this server to be the active server - 
			 * resolvSend will start its querying from this
			 * server, skipping the previous servers
			 */

			_res.lookups[i] = 'a';
			if ((n = resSearch(name, C_IN, T_A, buf.buf,
			    sizeof(buf))) < 0) {
#ifdef DEBUG
				if (_res.options & RES_DEBUG)
					printf("resSearch failed\n");
#endif
				break;
			}


			hp = getanswer(&buf, n, 0, pHost, hostbuf, bufLen );

			/* 
			 * Validate the address returned by the server - 
			 * should not be a network or broadcast address
			 * (SPR #67238)
			 */

			if (((hp->h_addr [3] & 0xff) == 0) ||
			    ((hp->h_addr [3] & 0xff) == 0xff))
			    {
			    hp = NULL;
			    errno = S_resolvLib_INVALID_ADDRESS;
			    }

			break;
		}
		if (_res.lookups [i] != 'f')
		    _res.lookups [i] = 'b'; 
	}
	return (hp);
}




struct hostent *
_gethostbyaddr(addr, len, type, pHost, hostbuf, bufLen)
	const char *addr;
	int len, type;
	struct hostent *pHost;
	char *           hostbuf;
	int             bufLen;
{
	int n, i;
	querybuf buf;
	register struct hostent *hp;
	char qbuf[MAXDNAME];
	char lookups[MAXDNSLUS];
	
	if (type != AF_INET)
		return ((struct hostent *) NULL);
	(void)sprintf(qbuf, "%u.%u.%u.%u.in-addr.arpa",
		((unsigned)addr[3] & 0xff),
		((unsigned)addr[2] & 0xff),
		((unsigned)addr[1] & 0xff),
		((unsigned)addr[0] & 0xff));


	bcopy(_res.lookups, lookups, sizeof lookups);
	if (lookups[0] == '\0')
		strncpy(lookups, "bf", sizeof lookups);

	hp = (struct hostent *)NULL;
	for (i = 0; i < MAXDNSLUS && hp == NULL && lookups[i]; i++) {
		switch (lookups[i]) {
		case 'b':
			n = resolvQuery(qbuf, C_IN, T_PTR, (char *)&buf, 
					sizeof(buf));
			if (n == ERROR) {
#ifdef DEBUG
				if (_res.options & RES_DEBUG)
					printf("resolvQuery failed\n");
#endif
				break;
			}
			hp = getanswer(&buf, n, 1, pHost, hostbuf, bufLen);
			if (hp == NULL)
				break;
			hp->h_addrtype = type;
			hp->h_length = len;
            		pHost->h_addr_list [1] = NULL;
            		bcopy (addr, (char *) &pHost->h_addr_list [2], 4);
            		pHost->h_addr_list [0] =  (char*) & pHost->h_addr_list [2];
			break;
		}
	}
	return (hp);
}



static int
qcomp(a1, a2)
	struct in_addr **a1, **a2;
{
	int pos1, pos2;

	for (pos1 = 0; pos1 < _res.nsort; pos1++)
		if (_res.sort_list[pos1].addr.s_addr ==
		    ((*a1)->s_addr & _res.sort_list[pos1].mask))
			break;
	for (pos2 = 0; pos2 < _res.nsort; pos2++)
		if (_res.sort_list[pos2].addr.s_addr ==
		    ((*a2)->s_addr & _res.sort_list[pos2].mask))
			break;
	return pos1 - pos2;
}


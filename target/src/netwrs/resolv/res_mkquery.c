/* res_mkquery.c - creates DNS queries */

/* Copyright 1984-2001 Wind River Systems, Inc. */


/*
 * Copyright (c) 1985 Regents of the University of California.
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
01c,17sep01,vvv fixed compilation warning
01b,29apr97,jag Changed T_NULL to T_NULL_RR to fix conflict with loadCoffLib.h
01b,03apr97,jag Move the increment of _res.id outside of the htons macro.
01a,13dec96,jag Cleaned up and rename user I/F functions with the resolv
		prefix.  Man pages can be found in resolvLib.c
*/

#include <resolvLib.h>
#include <stdio.h>
#include <string.h>

/*
 * Form all types of queries.
 * Returns the size of the result or ERROR
 */
int 
resolvMkQuery(op, dname, class, type, data, datalen, newrr_in, buf, buflen)
	int op;			/* opcode of query */
	const char *dname;	/* domain name */
	int class, type;	/* class and type of query */
	const char *data;	/* resource record data */
	int datalen;		/* length of data */
	const char *newrr_in;	/* new rr for modify or append */
	char *buf;		/* buffer to put query */
	int buflen;		/* size of buffer */
{
	register HEADER *hp;
	register char *cp;
	register int n;
	char *dnptrs[10], **dpp, **lastdnptr;

#ifdef DEBUG
	if (_res.options & RES_DEBUG)
		printf(";; resolvMkQuery(%d, %s, %d, %d)\n", op, dname, class, type);
#endif
	/*
	 * Initialize header fields.
	 */
	if ((buf == NULL) || (buflen < sizeof(HEADER)))
		return(ERROR);
	bzero(buf, sizeof(HEADER));
	hp = (HEADER *) buf;
	++_res.id;
	hp->id = htons(_res.id);
	hp->opcode = op;
	hp->pr = (_res.options & RES_PRIMARY) != 0;
	hp->rd = (_res.options & RES_RECURSE) != 0;
	hp->rcode = NOERROR;
	cp = buf + sizeof(HEADER);
	buflen -= sizeof(HEADER);
	dpp = dnptrs;
	*dpp++ = buf;
	*dpp++ = NULL;
	lastdnptr = dnptrs + sizeof(dnptrs)/sizeof(dnptrs[0]);
	/*
	 * perform opcode specific processing
	 */
	switch (op) {
	case QUERY:
		if ((buflen -= QFIXEDSZ) < 0)
			return(ERROR);
		if ((n = resolvDNComp((u_char *)dname, (u_char *)cp, buflen,
		    (u_char **)dnptrs, (u_char **)lastdnptr)) < 0)
			return (ERROR);
		cp += n;
		buflen -= n;
		__putshort(type, (u_char *)cp);
		cp += sizeof(u_short);
		__putshort(class, (u_char *)cp);
		cp += sizeof(u_short);
		hp->qdcount = htons(1);
		if (op == QUERY || data == NULL)
			break;
		/*
		 * Make an additional record for completion domain.
		 */
		buflen -= RRFIXEDSZ;
		if ((n = resolvDNComp((u_char *)data, (u_char *)cp, buflen,
		    (u_char **)dnptrs, (u_char **)lastdnptr)) < 0)
			return (ERROR);
		cp += n;
		buflen -= n;
		__putshort(T_NULL_RR, (u_char *)cp);
		cp += sizeof(u_short);
		__putshort(class, (u_char *)cp);
		cp += sizeof(u_short);
		__putlong(0, (u_char *)cp);
		cp += sizeof(u_long);
		__putshort(0, (u_char *)cp);
		cp += sizeof(u_short);
		hp->arcount = htons(1);
		break;

	case IQUERY:
		/*
		 * Initialize answer section
		 */
		if (buflen < 1 + RRFIXEDSZ + datalen)
			return (ERROR);
		*cp++ = '\0';	/* no domain name */
		__putshort(type, (u_char *)cp);
		cp += sizeof(u_short);
		__putshort(class, (u_char *)cp);
		cp += sizeof(u_short);
		__putlong(0, (u_char *)cp);
		cp += sizeof(u_long);
		__putshort(datalen, (u_char *)cp);
		cp += sizeof(u_short);
		if (datalen) {
			bcopy(data, cp, datalen);
			cp += datalen;
		}
		hp->ancount = htons(1);
		break;

#ifdef ALLOW_UPDATES
	/*
	 * For UPDATEM/UPDATEMA, do UPDATED/UPDATEDA followed by UPDATEA
	 * (Record to be modified is followed by its replacement in msg.)
	 */
	case UPDATEM:
	case UPDATEMA:

	case UPDATED:
		/*
		 * The res code for UPDATED and UPDATEDA is the same; user
		 * calls them differently: specifies data for UPDATED; server
		 * ignores data if specified for UPDATEDA.
		 */
	case UPDATEDA:
		buflen -= RRFIXEDSZ + datalen;
		if ((n = resolvDNComp(dname, cp, buflen, dnptrs, lastdnptr)) < 0)
			return (ERROR);
		cp += n;
		__putshort(type, cp);
                cp += sizeof(u_short);
                __putshort(class, cp);
                cp += sizeof(u_short);
		__putlong(0, cp);
		cp += sizeof(u_long);
		__putshort(datalen, cp);
                cp += sizeof(u_short);
		if (datalen) {
			bcopy(data, cp, datalen);
			cp += datalen;
		}
		if ( (op == UPDATED) || (op == UPDATEDA) ) {
			hp->ancount = htons(0);
			break;
		}
		/* Else UPDATEM/UPDATEMA, so drop into code for UPDATEA */

	case UPDATEA:	/* Add new resource record */
		buflen -= RRFIXEDSZ + datalen;
		if ((n = resolvDNComp(dname, cp, buflen, dnptrs, lastdnptr)) < 0)
			return (ERROR);
		cp += n;
		__putshort(newrr->r_type, cp);
                cp += sizeof(u_short);
                __putshort(newrr->r_class, cp);
                cp += sizeof(u_short);
		__putlong(0, cp);
		cp += sizeof(u_long);
		__putshort(newrr->r_size, cp);
                cp += sizeof(u_short);
		if (newrr->r_size) {
			bcopy(newrr->r_data, cp, newrr->r_size);
			cp += newrr->r_size;
		}
		hp->ancount = htons(0);
		break;

#endif /* ALLOW_UPDATES */
	}
	return (cp - buf);
}

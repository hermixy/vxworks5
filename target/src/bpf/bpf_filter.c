/* bpf_filter.c - Berkeley Packet Filter support routines for filter program */

/* Copyright 1999 - 2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*-
 * Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995, 1996, 1997
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from the Stanford/CMU enet packet filter,
 * (net/enet.c) distributed as part of 4.3BSD, and code contributed
 * to Berkeley by Steven McCanne and Van Jacobson both of Lawrence 
 * Berkeley Laboratory.
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
 *
 *	@(#)bpf.c	7.5 (Berkeley) 7/15/91
 */

static const char rcsid[] =
    "@(#) $Header: bpf_filter.c,v 1.33 97/04/26 13:37:18 leres Exp $ (LBL)";

/*
modification history
--------------------
01c,25apr02,wap  Don't choose EXTRACT_xx() macros based on byte-ordering (SPR
                 #76316)
01b,26oct00,spm  fixed merge from tor3_x branch and updated mod history
01a,23oct00,niq  created from version 01a of tor3_x branch
*/

#include <sys/types.h>
#include <sys/times.h>
#include <net/bpf.h>

#define int32 bpf_int32
#define u_int32 bpf_u_int32

/*
 * On architectures where unaligned accesses are legal, we can use
 * a slightly simpler set of macros.
 */
#if (CPU_FAMILY == I80X86)
#include <netinet/in.h>

#define EXTRACT_SHORT(p)	((u_short)ntohs(*(u_short *)p))
#define EXTRACT_LONG(p)		(ntohl(*(u_int32 *)p))

#else

#define EXTRACT_SHORT(p)\
	((u_short)\
		((u_short)*((u_char *)p+0)<<8|\
		 (u_short)*((u_char *)p+1)<<0))
#define EXTRACT_LONG(p)\
		((u_int32)*((u_char *)p+0)<<24|\
		 (u_int32)*((u_char *)p+1)<<16|\
		 (u_int32)*((u_char *)p+2)<<8|\
		 (u_int32)*((u_char *)p+3)<<0)

#endif

#include <net/mbuf.h>
#define MINDEX(len, m, k) \
{ \
	len = m->m_len; \
	while (k >= len) { \
		k -= len; \
		m = m->m_next; \
		if (m == 0) \
			return 0; \
		len = m->m_len; \
	} \
}

static int
m_xword(m, k, err)
	register struct mbuf *m;
	register int k, *err;
{
	register int len;
	register u_char *cp, *np;
	register struct mbuf *m0;

	MINDEX(len, m, k);
	cp = mtod(m, u_char *) + k;
	if (len - k >= 4) {
		*err = 0;
		return EXTRACT_LONG(cp);
	}
	m0 = m->m_next;
	if (m0 == 0 || m0->m_len + len - k < 4)
		goto bad;
	*err = 0;
	np = mtod(m0, u_char *);
	switch (len - k) {

	case 1:
		return (cp[0] << 24) | (np[0] << 16) | (np[1] << 8) | np[2];

	case 2:
		return (cp[0] << 24) | (cp[1] << 16) | (np[0] << 8) | np[1];

	default:
		return (cp[0] << 24) | (cp[1] << 16) | (cp[2] << 8) | np[0];
	}
    bad:
	*err = 1;
	return 0;
}

static int
m_xhalf(m, k, err)
	register struct mbuf *m;
	register int k, *err;
{
	register int len;
	register u_char *cp;
	register struct mbuf *m0;

	MINDEX(len, m, k);
	cp = mtod(m, u_char *) + k;
	if (len - k >= 2) {
		*err = 0;
		return EXTRACT_SHORT(cp);
	}
	m0 = m->m_next;
	if (m0 == 0)
		goto bad;
	*err = 0;
	return (cp[0] << 8) | mtod(m0, u_char *)[0];
 bad:
	*err = 1;
	return 0;
}

/*
 * Execute the filter program starting at pc on the packet p
 * wirelen is the length of the original packet
 * buflen is the amount of data present in the first mBlk (0 if unknown)
 * type is the MUX encoding for the frame type and offset is the
 *      value needed to skip the link-level header
 */
u_int
bpf_filter(pc, p, wirelen, buflen, type, offset)
	register struct bpf_insn *pc;
	register u_char *p;
	u_int wirelen;
	register u_int buflen;
        long type;
        int offset;
{
	register u_int32 A, X;
	register int k;
	int32 mem[BPF_MEMWORDS];

	if (pc == 0)
		/*
		 * No filter means accept all.
		 */
		return (u_int)-1;
	A = 0;
	X = 0;
	--pc;
	while (1) {
		++pc;
		switch (pc->code) {

		default:
			return 0;
		case BPF_RET|BPF_K:
			return (u_int)pc->k;

		case BPF_RET|BPF_A:
			return (u_int)A;

                case BPF_RET|BPF_K|BPF_HLEN:
                        return (u_int)(pc->k + offset);

		case BPF_LD|BPF_W|BPF_ABS:
			k = pc->k;
			if (k + sizeof(int32) > buflen) {
				int merr;

				if (buflen != 0)
					return 0;
				A = m_xword((struct mbuf *)p, k, &merr);
				if (merr != 0)
					return 0;
				continue;
			}
			A = EXTRACT_LONG(&p[k]);
			continue;

		case BPF_LD|BPF_H|BPF_ABS:
			k = pc->k;
			if (k + sizeof(short) > buflen) {
				int merr;

				if (buflen != 0)
					return 0;
				A = m_xhalf((struct mbuf *)p, k, &merr);
				continue;
			}
			A = EXTRACT_SHORT(&p[k]);
			continue;

		case BPF_LD|BPF_B|BPF_ABS:
			k = pc->k;
			if (k >= buflen) {
				register struct mbuf *m;
				register int len;

				if (buflen != 0)
					return 0;
				m = (struct mbuf *)p;
				MINDEX(len, m, k);
				A = mtod(m, u_char *)[k];
				continue;
			}
			A = p[k];
			continue;

		case BPF_LD|BPF_W|BPF_ABS|BPF_HLEN:
			k = pc->k + offset;
			if (k + sizeof(int32) > buflen) {
				int merr;

				if (buflen != 0)
					return 0;
				A = m_xword((struct mbuf *)p, k, &merr);
				if (merr != 0)
					return 0;
				continue;
			}
			A = EXTRACT_LONG(&p[k]);
			continue;

		case BPF_LD|BPF_H|BPF_ABS|BPF_HLEN:
			k = pc->k + offset;
			if (k + sizeof(short) > buflen) {
				int merr;

				if (buflen != 0)
					return 0;
				A = m_xhalf((struct mbuf *)p, k, &merr);
				continue;
			}
			A = EXTRACT_SHORT(&p[k]);
			continue;

		case BPF_LD|BPF_B|BPF_ABS|BPF_HLEN:
			k = pc->k + offset;
			if (k >= buflen) {
				register struct mbuf *m;
				register int len;

				if (buflen != 0)
					return 0;
				m = (struct mbuf *)p;
				MINDEX(len, m, k);
				A = mtod(m, u_char *)[k];
				continue;
			}
			A = p[k];
			continue;

		case BPF_LD|BPF_W|BPF_LEN:
			A = wirelen;
			continue;

		case BPF_LDX|BPF_W|BPF_LEN:
			X = wirelen;
			continue;

		case BPF_LD|BPF_TYPE:
			A = type;
			continue;

		case BPF_LD|BPF_HLEN:
			A = offset;
			continue;

                case BPF_LDX|BPF_HLEN:
                        X = offset;
                        continue;

		case BPF_LD|BPF_W|BPF_IND:
			k = X + pc->k;
			if (k + sizeof(int32) > buflen) {
				int merr;

				if (buflen != 0)
					return 0;
				A = m_xword((struct mbuf *)p, k, &merr);
				if (merr != 0)
					return 0;
				continue;
			}
			A = EXTRACT_LONG(&p[k]);
			continue;

		case BPF_LD|BPF_H|BPF_IND:
			k = X + pc->k;
			if (k + sizeof(short) > buflen) {
				int merr;

				if (buflen != 0)
					return 0;
				A = m_xhalf((struct mbuf *)p, k, &merr);
				if (merr != 0)
					return 0;
				continue;
			}
			A = EXTRACT_SHORT(&p[k]);
			continue;

		case BPF_LD|BPF_B|BPF_IND:
			k = X + pc->k;
			if (k >= buflen) {
				register struct mbuf *m;
				register int len;

				if (buflen != 0)
					return 0;
				m = (struct mbuf *)p;
				MINDEX(len, m, k);
				A = mtod(m, u_char *)[k];
				continue;
			}
			A = p[k];
			continue;

		case BPF_LDX|BPF_MSH|BPF_B|BPF_HLEN:
			k = pc->k + offset;
			if (k >= buflen) {
				register struct mbuf *m;
				register int len;

				if (buflen != 0)
					return 0;
				m = (struct mbuf *)p;
				MINDEX(len, m, k);
				X = (mtod(m, char *)[k] & 0xf) << 2;
				continue;
			}
			X = (p[pc->k] & 0xf) << 2;
			continue;

		case BPF_LDX|BPF_MSH|BPF_B:
			k = pc->k;
			if (k >= buflen) {
				register struct mbuf *m;
				register int len;

				if (buflen != 0)
					return 0;
				m = (struct mbuf *)p;
				MINDEX(len, m, k);
				X = (mtod(m, char *)[k] & 0xf) << 2;
				continue;
			}
			X = (p[pc->k] & 0xf) << 2;
			continue;

		case BPF_LD|BPF_IMM:
			A = pc->k;
			continue;

		case BPF_LDX|BPF_IMM:
			X = pc->k;
			continue;

		case BPF_LD|BPF_MEM:
			A = mem[pc->k];
			continue;
			
		case BPF_LDX|BPF_MEM:
			X = mem[pc->k];
			continue;

		case BPF_ST:
			mem[pc->k] = A;
			continue;

		case BPF_STX:
			mem[pc->k] = X;
			continue;

		case BPF_JMP|BPF_JA:
			pc += pc->k;
			continue;

		case BPF_JMP|BPF_JGT|BPF_K:
			pc += (A > pc->k) ? pc->jt : pc->jf;
			continue;

		case BPF_JMP|BPF_JGE|BPF_K:
			pc += (A >= pc->k) ? pc->jt : pc->jf;
			continue;

		case BPF_JMP|BPF_JEQ|BPF_K:
			pc += (A == pc->k) ? pc->jt : pc->jf;
			continue;

		case BPF_JMP|BPF_JSET|BPF_K:
			pc += (A & pc->k) ? pc->jt : pc->jf;
			continue;

		case BPF_JMP|BPF_JGT|BPF_X:
			pc += (A > X) ? pc->jt : pc->jf;
			continue;

		case BPF_JMP|BPF_JGE|BPF_X:
			pc += (A >= X) ? pc->jt : pc->jf;
			continue;

		case BPF_JMP|BPF_JEQ|BPF_X:
			pc += (A == X) ? pc->jt : pc->jf;
			continue;

		case BPF_JMP|BPF_JSET|BPF_X:
			pc += (A & X) ? pc->jt : pc->jf;
			continue;

		case BPF_ALU|BPF_ADD|BPF_X:
			A += X;
			continue;
			
		case BPF_ALU|BPF_SUB|BPF_X:
			A -= X;
			continue;
			
		case BPF_ALU|BPF_MUL|BPF_X:
			A *= X;
			continue;
			
		case BPF_ALU|BPF_DIV|BPF_X:
			if (X == 0)
				return 0;
			A /= X;
			continue;
			
		case BPF_ALU|BPF_AND|BPF_X:
			A &= X;
			continue;
			
		case BPF_ALU|BPF_OR|BPF_X:
			A |= X;
			continue;

		case BPF_ALU|BPF_LSH|BPF_X:
			A <<= X;
			continue;

		case BPF_ALU|BPF_RSH|BPF_X:
			A >>= X;
			continue;

		case BPF_ALU|BPF_ADD|BPF_K:
			A += pc->k;
			continue;
			
		case BPF_ALU|BPF_SUB|BPF_K:
			A -= pc->k;
			continue;
			
		case BPF_ALU|BPF_MUL|BPF_K:
			A *= pc->k;
			continue;
			
		case BPF_ALU|BPF_DIV|BPF_K:
			A /= pc->k;
			continue;
			
		case BPF_ALU|BPF_AND|BPF_K:
			A &= pc->k;
			continue;
			
		case BPF_ALU|BPF_OR|BPF_K:
			A |= pc->k;
			continue;

		case BPF_ALU|BPF_LSH|BPF_K:
			A <<= pc->k;
			continue;

		case BPF_ALU|BPF_RSH|BPF_K:
			A >>= pc->k;
			continue;

		case BPF_ALU|BPF_NEG:
			A = -A;
			continue;

		case BPF_MISC|BPF_TAX:
			X = A;
			continue;

		case BPF_MISC|BPF_TXA:
			A = X;
			continue;
		}
	}
}

/*
 * Return true if the 'fcode' is a valid filter program.
 * The constraints are that each jump be forward and to a valid
 * code.  The code must terminate with either an accept or reject. 
 * 'valid' is an array for use by the routine (it must be at least
 * 'len' bytes long).  
 *
 * The kernel needs to be able to verify an application's filter code.
 * Otherwise, a bogus program could easily crash the system.
 */
int
bpf_validate(f, len)
	struct bpf_insn *f;
	int len;
{
	register int i;
	register struct bpf_insn *p;

	for (i = 0; i < len; ++i) {
		/*
		 * Check that that jumps are forward, and within 
		 * the code block.
		 */
		p = &f[i];
		if (BPF_CLASS(p->code) == BPF_JMP) {
			register int from = i + 1;

			if (BPF_OP(p->code) == BPF_JA) {
				if (from + p->k >= (unsigned)len)
					return 0;
			}
			else if (from + p->jt >= len || from + p->jf >= len)
				return 0;
		}
		/*
		 * Check that memory operations use valid addresses.
		 */
		if ((BPF_CLASS(p->code) == BPF_ST ||
		     (BPF_CLASS(p->code) == BPF_LD && 
		      (p->code & 0xe0) == BPF_MEM)) &&
		    (p->k >= BPF_MEMWORDS || p->k < 0))
			return 0;
		/*
		 * Check for constant division by 0.
		 */
		if (p->code == (BPF_ALU|BPF_DIV|BPF_K) && p->k == 0)
			return 0;
	}
	return BPF_CLASS(f[len - 1].code) == BPF_RET;
}

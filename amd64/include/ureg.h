/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

struct Ureg {
	uint64_t	ax;
	uint64_t	bx;
	uint64_t	cx;
	uint64_t	dx;
	uint64_t	si;
	uint64_t	di;
	uint64_t	bp;
	uint64_t	r8;
	uint64_t	r9;
	uint64_t	r10;
	uint64_t	r11;
	uint64_t	r12;
	uint64_t	r13;
	uint64_t	r14;
	uint64_t	r15;

	uint16_t	ds;
	uint16_t	es;
	uint16_t	fs;
	uint16_t	gs;

	uint64_t	type;
	uint64_t	error;				/* error code (or zero) */
	uint64_t	ip;				/* pc */
	uint64_t	cs;				/* old context */
	uint64_t	flags;				/* old flags */
	uint64_t	sp;				/* sp */
	uint64_t	ss;				/* old stack segment */
};

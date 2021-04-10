/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

struct Ureg {
	u64	ax;
	u64	bx;
	u64	cx;
	u64	dx;
	u64	si;
	u64	di;
	u64	bp;
	u64	r8;
	u64	r9;
	u64	r10;
	u64	r11;
	u64	r12;
	u64	r13;
	u64	r14;
	u64	r15;

	// These are pointless and not pushing them would simplify the
	// interrupt handler a lot. However, removing them would break
	// compatibility with Go Plan 9 binaries.
	u16	ds;
	u16	es;
	u16	fs;
	u16	gs;

	u64	type;
	u64	error;				/* error code (or zero) */
	u64	ip;				/* pc */
	u64	cs;				/* old context */
	u64	flags;				/* old flags */
	u64	sp;				/* sp */
	u64	ss;				/* old stack segment */
};

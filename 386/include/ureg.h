/* 
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

struct Ureg
{
	u32	di;		/* general registers */
	u32	si;		/* ... */
	u32	bp;		/* ... */
	u32	nsp;
	u32	bx;		/* ... */
	u32	dx;		/* ... */
	u32	cx;		/* ... */
	u32	ax;		/* ... */
	u32	gs;		/* data segments */
	u32	fs;		/* ... */
	u32	es;		/* ... */
	u32	ds;		/* ... */
	u32	trap;		/* trap type */
	u32	ecode;		/* error code (or zero) */
	u32	pc;		/* pc */
	u32	cs;		/* old context */
	u32	flags;		/* old flags */
	union {
		u32	usp;
		u32	sp;
	};
	u32	ss;		/* old stack segment */
};

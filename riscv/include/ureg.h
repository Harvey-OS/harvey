/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

struct Ureg {
	usize ip;
	usize sp;
	usize gp;
	usize tp;
	usize t0;
	usize t1;
	usize t2;
	usize s0; // NOTE: this is the bp in gcc with -fno-omit-frame-pointer
	usize s1;
	usize a0;
	usize a1;
	usize a2;
	usize a3;
	usize a4;
	usize a5;
	usize a6;
	usize a7;
	usize s2;
	usize s3;
	usize s4;
	usize s5;
	usize s6;
	usize s7;
	usize s8;
	usize s9;
	usize s10;
	usize s11;
	usize t3;
	usize t4;
	usize t5;
	usize t6;
	/* Supervisor CSRs */
	usize status;
	usize epc;
	usize badaddr;
	usize cause;
	usize insnn;
	usize bp; // BOGUS: need a real frame pointer here. 
	usize ftype;
};


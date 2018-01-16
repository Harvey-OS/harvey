/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

struct Ureg {
	uintptr_t ip;
	uintptr_t sp;
	uintptr_t gp;
	uintptr_t tp;
	uintptr_t t0;
	uintptr_t t1;
	uintptr_t t2;
	uintptr_t s0; // NOTE: this is the bp in gcc with -fno-omit-frame-pointer
	uintptr_t s1;
	uintptr_t a0;
	uintptr_t a1;
	uintptr_t a2;
	uintptr_t a3;
	uintptr_t a4;
	uintptr_t a5;
	uintptr_t a6;
	uintptr_t a7;
	uintptr_t s2;
	uintptr_t s3;
	uintptr_t s4;
	uintptr_t s5;
	uintptr_t s6;
	uintptr_t s7;
	uintptr_t s8;
	uintptr_t s9;
	uintptr_t s10;
	uintptr_t s11;
	uintptr_t t3;
	uintptr_t t4;
	uintptr_t t5;
	uintptr_t t6;
	/* Supervisor CSRs */
	uintptr_t status;
	uintptr_t epc;
	uintptr_t badaddr;
	uintptr_t cause;
	uintptr_t insnn;
	uintptr_t bp; // BOGUS: need a real frame pointer here. 
	uintptr_t ftype;
};


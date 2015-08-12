/* 
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef struct Ureg Ureg;

struct Ureg {
	uint32_t di; /* general registers */
	uint32_t si; /* ... */
	uint32_t bp; /* ... */
	uint32_t nsp;
	uint32_t bx;    /* ... */
	uint32_t dx;    /* ... */
	uint32_t cx;    /* ... */
	uint32_t ax;    /* ... */
	uint32_t gs;    /* data segments */
	uint32_t fs;    /* ... */
	uint32_t es;    /* ... */
	uint32_t ds;    /* ... */
	uint32_t trap;  /* trap type */
	uint32_t ecode; /* error code (or zero) */
	uint32_t pc;    /* pc */
	uint32_t cs;    /* old context */
	uint32_t flags; /* old flags */
	union {
		uint32_t usp;
		uint32_t sp;
	};
	uint32_t ss; /* old stack segment */
};

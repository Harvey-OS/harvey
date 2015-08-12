/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

struct Ureg {
	uint32_t status;
	uint32_t pc;
	union {
		uint32_t sp;  /* r29 */
		uint32_t usp; /* r29 */
	};
	uint32_t cause;
	uint32_t badvaddr;
	uint32_t tlbvirt;
	uint32_t hi;
	uint32_t lo;
	uint32_t r31;
	uint32_t r30;
	uint32_t r28;
	uint32_t r27; /* unused */
	uint32_t r26; /* unused */
	uint32_t r25;
	uint32_t r24;
	uint32_t r23;
	uint32_t r22;
	uint32_t r21;
	uint32_t r20;
	uint32_t r19;
	uint32_t r18;
	uint32_t r17;
	uint32_t r16;
	uint32_t r15;
	uint32_t r14;
	uint32_t r13;
	uint32_t r12;
	uint32_t r11;
	uint32_t r10;
	uint32_t r9;
	uint32_t r8;
	uint32_t r7;
	uint32_t r6;
	uint32_t r5;
	uint32_t r4;
	uint32_t r3;
	uint32_t r2;
	uint32_t r1;
};

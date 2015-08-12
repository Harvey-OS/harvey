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
	long pc;
	union {
		struct
		    {
			long sp; /* r29 */
			uint32_t cause;
		};
		struct
		    {
			long usp; /* r29 */
			uint32_t ucause;
		};
	};
	uint32_t badvaddr;
	uint32_t tlbvirt;

	long hhi;
	long hi;
	long hlo;
	long lo;
	long hr31;
	long r31;
	long hr30;
	long r30;
	long hr28;
	long r28;
	long hr27;
	long r27;
	long hr26;
	long r26;
	long hr25;
	long r25;
	long hr24;
	long r24;
	long hr23;
	long r23;
	long hr22;
	long r22;
	long hr21;
	long r21;
	long hr20;
	long r20;
	long hr19;
	long r19;
	long hr18;
	long r18;
	long hr17;
	long r17;
	long hr16;
	long r16;
	long hr15;
	long r15;
	long hr14;
	long r14;
	long hr13;
	long r13;
	long hr12;
	long r12;
	long hr11;
	long r11;
	long hr10;
	long r10;
	long hr9;
	long r9;
	long hr8;
	long r8;
	long hr7;
	long r7;
	long hr6;
	long r6;
	long hr5;
	long r5;
	long hr4;
	long r4;
	long hr3;
	long r3;
	long hr2;
	long r2;
	long hr1;
	long r1;
};

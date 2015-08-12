/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

struct Ureg {
	/*  0*/ uint32_t cause;
	/*  4*/ union {
		uint32_t srr1;
		uint32_t status;
	};
	/*  8*/ uint32_t pc; /* SRR0 */
	/* 12*/ uint32_t pad;
	/* 16*/ uint32_t lr;
	/* 20*/ uint32_t cr;
	/* 24*/ uint32_t xer;
	/* 28*/ uint32_t ctr;
	/* 32*/ uint32_t r0;
	/* 36*/ union {
		uint32_t r1;
		uint32_t sp;
		uint32_t usp;
	};
	/* 40*/ uint32_t r2;
	/* 44*/ uint32_t r3;
	/* 48*/ uint32_t r4;
	/* 52*/ uint32_t r5;
	/* 56*/ uint32_t r6;
	/* 60*/ uint32_t r7;
	/* 64*/ uint32_t r8;
	/* 68*/ uint32_t r9;
	/* 72*/ uint32_t r10;
	/* 76*/ uint32_t r11;
	/* 80*/ uint32_t r12;
	/* 84*/ uint32_t r13;
	/* 88*/ uint32_t r14;
	/* 92*/ uint32_t r15;
	/* 96*/ uint32_t r16;
	/*100*/ uint32_t r17;
	/*104*/ uint32_t r18;
	/*108*/ uint32_t r19;
	/*112*/ uint32_t r20;
	/*116*/ uint32_t r21;
	/*120*/ uint32_t r22;
	/*124*/ uint32_t r23;
	/*128*/ uint32_t r24;
	/*132*/ uint32_t r25;
	/*136*/ uint32_t r26;
	/*140*/ uint32_t r27;
	/*144*/ uint32_t r28;
	/*148*/ uint32_t r29;
	/*152*/ uint32_t r30;
	/*156*/ uint32_t r31;
	/*160*/ uint32_t dcmp;
	/*164*/ uint32_t icmp;
	/*168*/ uint32_t dmiss;
	/*172*/ uint32_t imiss;
	/*176*/ uint32_t hash1;
	/*180*/ uint32_t hash2;
	/*184*/ uint32_t dar;
	/*188*/ uint32_t dsisr;
};

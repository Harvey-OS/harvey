struct Ureg
{
	union {
		uintptr	pc;
		uintptr regs[1];
	};
	uintptr	r1;		/* link */
	union{
		uintptr	r2;
		uintptr	sp;
		uintptr	usp;
	};
	uintptr	r3;		/* sb */
	uintptr	r4;
	uintptr	r5;
	uintptr	r6;		/* up in kernel */
	uintptr	r7;		/* m in kernel */
	union{
		uintptr	r8;
		uintptr arg;
		uintptr ret;
	};
	uintptr	r9;
	uintptr	r10;
	uintptr	r11;
	uintptr	r12;
	uintptr	r13;
	uintptr	r14;
	uintptr	r15;
	uintptr	r16;
	uintptr	r17;
	uintptr	r18;
	uintptr	r19;
	uintptr	r20;
	uintptr	r21;
	uintptr	r22;
	uintptr	r23;
	uintptr	r24;
	uintptr	r25;
	uintptr	r26;
	uintptr	r27;
	uintptr	r28;
	uintptr	r29;
	uintptr	r30;
	uintptr	r31;

	/* csrs: generally supervisor ones */
	uintptr	status;
	uintptr	ie;
	union {
		uintptr	cause;
		uintptr	type;
	};
	uintptr	tval;			/* faulting address */

	uintptr	curmode;
};

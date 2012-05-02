struct Ureg
{
	ulong	status;
	ulong	pc;
	union{
		ulong	sp;		/* r29 */
		ulong	usp;		/* r29 */
	} u0;
	ulong	cause;
	ulong	badvaddr;
	ulong	tlbvirt;
	ulong	hi;
	ulong	lo;
	ulong	r31;
	ulong	r30;
	ulong	r28;
	ulong	r27;		/* unused */
	ulong	r26;		/* unused */
	ulong	r25;
	ulong	r24;
	ulong	r23;
	ulong	r22;
	ulong	r21;
	ulong	r20;
	ulong	r19;
	ulong	r18;
	ulong	r17;
	ulong	r16;
	ulong	r15;
	ulong	r14;
	ulong	r13;
	ulong	r12;
	ulong	r11;
	ulong	r10;
	ulong	r9;
	ulong	r8;
	ulong	r7;
	ulong	r6;
	ulong	r5;
	ulong	r4;
	ulong	r3;
	ulong	r2;
	ulong	r1;
};

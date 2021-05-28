struct Ureg
{
	u64int	status;
	u64int	pc;
	union{
		u64int	sp;		/* r29 */
		u64int	usp;		/* r29 */
	};
	u64int	cause;
	u64int	badvaddr;
	u64int	tlbvirt;
	u64int	hi;
	u64int	lo;
	u64int	r31;
	u64int	r30;
	u64int	r28;
	u64int	r27;		/* unused */
	u64int	r26;		/* unused */
	u64int	r25;
	u64int	r24;
	u64int	r23;
	u64int	r22;
	u64int	r21;
	u64int	r20;
	u64int	r19;
	u64int	r18;
	u64int	r17;
	u64int	r16;
	u64int	r15;
	u64int	r14;
	u64int	r13;
	u64int	r12;
	u64int	r11;
	u64int	r10;
	u64int	r9;
	u64int	r8;
	u64int	r7;
	u64int	r6;
	u64int	r5;
	u64int	r4;
	u64int	r3;
	u64int	r2;
	u64int	r1;
};

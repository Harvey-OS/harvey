typedef struct Ureg {
	ulong	r0;
	ulong	r1;
	ulong	r2;
	ulong	r3;
	ulong	r4;
	ulong	r5;
	ulong	r6;
	ulong	r7;
	ulong	r8;
	ulong	r9;
	ulong	r10;
	ulong	r11;
	ulong	r12;	/* sb */
	union {
		ulong	r13;
		ulong	sp;
	};
	union {
		ulong	r14;
		ulong	link;
	};
	ulong	type;	/* of exception */
	ulong	psr;
	ulong	pc;	/* interrupted addr */
} Ureg;

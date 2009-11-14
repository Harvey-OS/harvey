typedef struct Ureg {
	u32int	r0;
	u32int	r1;
	u32int	r2;
	u32int	r3;
	u32int	r4;
	u32int	r5;
	u32int	r6;
	u32int	r7;
	u32int	r8;
	u32int	r9;				/* up */
	u32int	r10;				/* m */
	u32int	r11;				/* loader temprorary */
	u32int	r12;				/* SB */
	union {
		u32int	r13;
		u32int	sp;
	};
	union {
		u32int	r14;
		u32int	link;
	};
	u32int	type;				/* of exception */
	u32int	psr;
	u32int	pc;				/* interrupted addr */
} Ureg;

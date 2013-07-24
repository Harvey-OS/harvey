/*
 * mips 24k machine assist
 */
#undef	MASK
#define	MASK(w) ((1<<(w))-1)

#define	SP	R29

#define NOP	NOR R0, R0, R0

#define	CONST(x,r) MOVW $((x)&0xffff0000), r; OR  $((x)&0xffff), r

/* a mips 24k erratum requires a NOP after; experience dictates EHB before */
#define	ERET	EHB; WORD $0x42000018; NOP

#define RETURN	RET; NOP

/*
 *  R4000 instructions
 */
#define	LL(base, rt)	WORD	$((060<<26)|((base)<<21)|((rt)<<16))
#define	SC(base, rt)	WORD	$((070<<26)|((base)<<21)|((rt)<<16))

/* new instructions in mips 24k (mips32r2) */
#define DI(rt)	WORD $(0x41606000|((rt)<<16))	/* interrupts off */
#define EI(rt)	WORD $(0x41606020|((rt)<<16))	/* interrupts on */
#define EHB	WORD $0xc0
/* jalr with hazard barrier, link in R22 */
#define JALRHB(r) WORD $(((r)<<21)|(22<<11)|(1<<10)|9); NOP
/* jump register with hazard barrier */
#define JRHB(r)	WORD $(((r)<<21)|(1<<10)|8); NOP
#define MFC0(src,sel,dst) WORD $(0x40000000|((src)<<11)|((dst)<<16)|(sel))
#define MTC0(src,dst,sel) WORD $(0x40800000|((dst)<<11)|((src)<<16)|(sel))
#define MIPS24KNOP NOP				/* for erratum #48 */
#define RDHWR(hwr, r)	WORD $(0x7c00003b|((hwr)<<11)|((r)<<16))
#define SYNC	WORD $0xf			/* all sync barriers */
#define WAIT	WORD $0x42000020		/* wait for interrupt */

/* all barriers, clears all hazards; clobbers r/Reg and R22 */
#define BARRIERS(r, Reg, label) \
	SYNC; EHB; MOVW $ret(SB), Reg; JALRHB(r)
/* same but return to KSEG1 */
#define UBARRIERS(r, Reg, label) \
	SYNC; EHB; MOVW $ret(SB), Reg; OR $KSEG1, Reg; JALRHB(r)

/* alternative definitions using labels */
#ifdef notdef
/* all barriers, clears all hazards; clobbers r/Reg */
#define BARRIERS(r, Reg, label) \
	SYNC; EHB; \
	MOVW	$label(SB), Reg; \
	JRHB(r); \
TEXT label(SB), $-4; \
	NOP
#define UBARRIERS(r, Reg, label) \
	SYNC; EHB; \
	MOVW	$label(SB), Reg; \
	OR	$KSEG1, Reg; \
	JRHB(r); \
TEXT label(SB), $-4; \
	NOP
#endif

#define PUTC(c, r1, r2)	CONST(PHYSCONS, r1); MOVW $(c), r2; MOVW r2, (r1); NOP

/*
 *  cache manipulation
 */

#define	CACHE	BREAK		/* overloaded op-code */

#define	PI	R((0		/* primary I cache */
#define	PD	R((1		/* primary D cache */
#define	TD	R((2		/* tertiary I/D cache */
#define	SD	R((3		/* secondary combined I/D cache */

#define	IWBI	(0<<2)))	/* index write-back invalidate */
#define	ILT	(1<<2)))	/* index load tag */
#define	IST	(2<<2)))	/* index store tag */
/* #define CDE	(3<<2)))	/* create dirty exclusive */
#define	HINV	(4<<2)))	/* hit invalidate */
#define	HWBI	(5<<2)))	/* hit write back invalidate */
#define	HWB	(6<<2)))	/* hit write back */
/* #define HSV	(7<<2)))	/* hit set virtual */

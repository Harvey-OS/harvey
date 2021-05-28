#undef	MASK
#define	MASK(w) ((1<<(w))-1)

#define NOP		NOR	R0, R0, R0
#define	CONST(x,r)		MOVW $((x)&0xffff0000), r; OR  $((x)&0xffff), r

#define	LL(base, rt)	WORD	$((060<<26)|((base)<<21)|((rt)<<16))
#define	SC(base, rt)	WORD	$((070<<26)|((base)<<21)|((rt)<<16))

#define	ERET	WORD $0x42000018; NOP
#define SYNC	WORD $0xf			/* all sync barriers */
#define EHB		NOP				/* seems work */
#define WAIT					/* loongson 2e does not have this */

#define JALR(d, r)		WORD	$(((r)<<21)|((d)<<11)|9); NOP

#define TOKSEG0(r)	MOVW $ret0(SB), R(r); JALR(22, r)
#define TOKSEG1(r)	MOVW $ret1(SB), R(r); OR $KSEG1, R(r); JALR(22, r)

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
#define	ILD	(6<<2)))	/* index load data (loongson 2e) */
#define ISD	(7<<2)))	/* index store data (loongson 2e) */

/*
 *	serial output
 */

#define PUTC(c, r1, r2)	CONST(PHYSCONS, r1); MOVW $(c), r2; MOVBU r2, (r1); NOP

#define DELAY(r)	\
	CONST(34000000, r); \
	SUBU	$1, r; \
	BNE		r, -1(PC)

/*
 *	FCR31 bits copied from u.h
 */

/* FCR (FCR31) */
#define	FPINEX	(1<<7)		/* enables */
#define	FPUNFL	(1<<8)
#define	FPOVFL	(1<<9)
#define	FPZDIV	(1<<10)
#define	FPINVAL	(1<<11)
#define	FPRNR	(0<<0)		/* rounding modes */
#define	FPRZ	(1<<0)
#define	FPRPINF	(2<<0)
#define	FPRNINF	(3<<0)
#define	FPRMASK	(3<<0)
#define	FPPEXT	0
#define	FPPSGL	0
#define	FPPDBL	0
#define	FPPMASK	0
#define FPCOND	(1<<23)

/* FSR (also FCR31) */
#define	FPAINEX	(1<<2)		/* flags */
#define	FPAOVFL	(1<<4)
#define	FPAUNFL	(1<<3)
#define	FPAZDIV	(1<<5)
#define	FPAINVAL (1<<6)

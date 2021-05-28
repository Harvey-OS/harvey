/*
 *	RISC-V atomic operations
 *	assumes A extension
 *	LR/SC only work on cached regions
 */

#define ARG	8

#define MASK(w)	((1<<(w))-1)
#define FENCE	WORD $(0xf | MASK(8)<<20)  /* all i/o, mem ops before & after */
#define AQ	(1<<26)			/* acquire */
#define RL	(1<<25)			/* release */
#define LRW(rs1, rd) \
	WORD $((2<<27)|(    0<<20)|((rs1)<<15)|(2<<12)|((rd)<<7)|057|AQ)
#define SCW(rs2, rs1, rd) \
	WORD $((3<<27)|((rs2)<<20)|((rs1)<<15)|(2<<12)|((rd)<<7)|057|AQ|RL)
#define LRD(rs1, rd) \
	WORD $((2<<27)|(    0<<20)|((rs1)<<15)|(3<<12)|((rd)<<7)|057|AQ)
#define SCD(rs2, rs1, rd) \
	WORD $((3<<27)|((rs2)<<20)|((rs1)<<15)|(3<<12)|((rd)<<7)|057|AQ|RL)


TEXT adec(SB), 1, $-4			/* long adec(long*); */
	MOV	$-1, R9
	JMP	ainctop
TEXT ainc(SB), 1, $-4			/* long ainc(long *); */
	MOV	$1, R9
ainctop:
	MOV	R(ARG), R12		/* address of counter */
	FENCE
loop:
	LRW(12, ARG)			/* (R12) -> R(ARG) */
	ADD	R9, R(ARG)		/* return new value */
	SCW(ARG, 12, 14)		/* R(ARG) -> (R12) maybe, R14=0 if ok */
	BNE	R14, loop
	FENCE
	RET

/*
 * int cas(uint* p, int ov, int nv);
 *
 * compare-and-swap: atomically set *addr to nv only if it contains ov,
 * and returns the old value.  this version returns 0 on failure, 1 on success
 * instead.
 */
TEXT cas(SB), 1, $-4
	MOVWU	ov+XLEN(FP), R12
	MOVWU	nv+(XLEN+4)(FP), R13
	FENCE
spincas:
	LRW(ARG, 14)		/* (R(ARG)) -> R14 */
	SLL	$32, R14
	SRL	$32, R14	/* don't sign extend */
	BNE	R12, R14, fail
	SCW(13, ARG, 14)	/* R13 -> (R(ARG)) maybe, R14=0 if ok */
	BNE	R14, spincas	/* R14 != 0 means store failed */
	MOV	$1, R(ARG)
	RET
fail:
	MOV	R0, R(ARG)
	RET

/*
 * int	casp(void **p, void *ov, void *nv);
 */
TEXT casp(SB), 1, $-4
	MOV	ov+XLEN(FP), R12
	MOV	nv+(2*XLEN)(FP), R13
	FENCE
spincasp:
	LRD(ARG, 14)		/* (R(ARG)) -> R14 */
	BNE	R12, R14, failp
	SCD(13, ARG, 14)	/* R13 -> (R(ARG)) maybe, R14=0 if ok */
	BNE	R14, spincasp	/* R14 != 0 means store failed */
	MOV	$1, R(ARG)
	RET
failp:
	MOV	R0, R(ARG)
	RET

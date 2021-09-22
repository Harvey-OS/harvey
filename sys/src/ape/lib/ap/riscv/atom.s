/*
 *	RISC-V atomic operations
 *	assumes A extension
 */

#define ARG	8

#define SYNC	WORD $0xf	/* FENCE */
#define LRW(rs2, rs1, rd) \
	WORD $((2<<27)|(    0<<20)|((rs1)<<15)|(2<<12)|((rd)<<7)|057)
#define SCW(rs2, rs1, rd) \
	WORD $((3<<27)|((rs2)<<20)|((rs1)<<15)|(2<<12)|((rd)<<7)|057)

TEXT ainc(SB), 1, $-4			/* long ainc(long *); */
	MOVW	R(ARG), R12		/* address of counter */
	SYNC
loop:
	MOVW	$1, R13
	LRW(0, 12, ARG)	// LR_W	R0, R12, R(ARG) /* (R12) -> R(ARG) */
	ADD	R(ARG), R13
	MOVW	R13, R(ARG)		/* return new value */
	SCW(13, 12, 14)	// SC_W	R13, R12, R14 /* R13 -> (R12) maybe, R14=0 if ok */
	BNE	R14, loop
	RET

TEXT adec(SB), 1, $-4			/* long adec(long*); */
	MOVW	R(ARG), R12		/* address of counter */
	SYNC
loop1:
	MOVW	$-1, R13
	LRW(0, 12, ARG)	// LR_W	R0, R12, R(ARG) /* (R12) -> R(ARG) */
	ADD	R(ARG), R13
	MOVW	R13, R(ARG)		/* return new value */
	SCW(13, 12, 14)	// SC_W R13, R12, R14 /* R13 -> (R12) maybe, R14=0 if ok */
	BNE	R14, loop1
	RET

/*
 * int cas(uint* p, int ov, int nv);
 *
 * compare-and-swap: atomically set *addr to nv only if it contains ov,
 * and returns the old value.  this version returns 0 on success, -1 on failure
 * instead.
 */
TEXT cas(SB), 1, $-4
	MOVW	ov+4(FP), R12
	MOVW	nv+8(FP), R13
	SYNC
spincas:
	LRW(0, ARG, 14)	// LR_W	R0, R(ARG), R14 /* (R(ARG)) -> R14 */
	BNE	R12, R14, fail
	SCW(13, ARG, 14) // SC_W R13, R(ARG), R14 /* R13 -> (R(ARG)) maybe, R14=0 if ok */
	BNE	R14, spincas	/* R14 != 0 means store failed */
	MOVW	$1, R(ARG)
	RET
fail:
	MOVW	R0, R(ARG)
	RET

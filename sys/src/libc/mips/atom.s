/*
 *	R4000 user-level atomic operations
 */

#define	LL(base, rt)	WORD	$((060<<26)|((base)<<21)|((rt)<<16))
#define	SC(base, rt)	WORD	$((070<<26)|((base)<<21)|((rt)<<16))
#define	NOOP		WORD	$0x27

TEXT ainc(SB), 1, $-4			/* long ainc(long *); */
TEXT _xinc(SB), 1, $-4			/* void _xinc(long *); */
	MOVW	R1, R2			/* address of counter */
loop:	MOVW	$1, R3
	LL(2, 1)
	NOOP
	ADD	R1,R3,R3
	SC(2, 3)
	NOOP
	BEQ	R3,loop
	RET

TEXT adec(SB), 1, $-4			/* long adec(long*); */
TEXT _xdec(SB), 1, $-4			/* long _xdec(long *); */
	MOVW	R1, R2			/* address of counter */
loop1:	MOVW	$-1, R3
	LL(2, 1)
	NOOP
	ADD	R1,R3,R3
	MOVW	R3, R1
	SC(2, 3)
	NOOP
	BEQ	R3,loop1
	RET

/*
 * int cas(uint* p, int ov, int nv);
 */
TEXT cas(SB), 1, $-4
	MOVW	ov+4(FP), R2
	MOVW	nv+8(FP), R3
spincas:
	LL(1, 4)			/* R4 = *R1 */
	NOOP
	BNE	R2, R4, fail
	SC(1, 3)			/* *R1 = R3 */
	NOOP
	BEQ	R3, spincas		/* R3 == 0 means store failed */
	MOVW	$1, R1
	RET
fail:
	MOVW	$0, R1
	RET

/* general-purpose abort */
_trap:
	MOVD	$0, R0
	MOVD	0(R0), R0
	RET

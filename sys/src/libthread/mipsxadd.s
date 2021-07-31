/*
 *	R4000 user level lock code
 */

#define	LL(base, rt)	WORD	$((060<<26)|((base)<<21)|((rt)<<16))
#define	SC(base, rt)	WORD	$((070<<26)|((base)<<21)|((rt)<<16))
#define	NOOP		WORD	$0x27

#ifdef oldstyle
TEXT	xadd(SB), $0

	MOVW	R1, R2		/* address of counter */
loop:	MOVW	n+4(FP), R3	/* increment */
	LL(2, 1)
	NOOP
	ADD	R1,R3,R3
	SC(2, 3)
	NOOP
	BEQ	R3,loop
	RET
#endif

TEXT	_xinc(SB), $0

	MOVW	R1, R2		/* address of counter */
loop:	MOVW	$1, R3
	LL(2, 1)
	NOOP
	ADD	R1,R3,R3
	SC(2, 3)
	NOOP
	BEQ	R3,loop
	RET

TEXT	_xdec(SB), $0

	MOVW	R1, R2		/* address of counter */
loop1:	MOVW	$-1, R3
	LL(2, 1)
	NOOP
	ADD	R1,R3,R3
	MOVW	R3, R1
	SC(2, 3)
	NOOP
	BEQ	R3,loop1
	RET

	TEXT	strcat(SB),$0

	MOVL	$0, AX
	MOVQ	$-1, CX
	CLD

/*
 * find length of second string
 */

	MOVQ	p2+8(FP), DI
	REPN;	SCASB

	MOVQ	DI, BX
	SUBQ	p2+8(FP), BX

/*
 * find end of first string
 */

	MOVQ	RARG, DI
	REPN;	SCASB

/*
 * copy the memory
 */
	SUBQ	$1, DI
	MOVQ	p2+8(FP), SI
/*
 * copy whole longs, if aligned
 */
	MOVQ	DI, CX
	ORQ	SI, CX
	ANDL	$3, CX
	JNE	c3
	MOVQ	BX, CX
	SHRQ	$2, CX
	REP;	MOVSL
/*
 * copy the rest, by bytes
 */
	ANDL	$3, BX
c3:
	MOVQ	BX, CX
	REP;	MOVSB

	MOVQ	RARG, AX
	RET

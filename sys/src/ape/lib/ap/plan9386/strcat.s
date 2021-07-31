	TEXT	strcat(SB),$0

	MOVL	$0, AX
	MOVL	$-1, CX
	CLD

/*
 * find length of second string
 */

	MOVL	p2+4(FP), DI
	REPN;	SCASB

	MOVL	DI, BX
	SUBL	p2+4(FP), BX

/*
 * find end of first string
 */

	MOVL	p1+0(FP), DI
	REPN;	SCASB

/*
 * copy the memory
 */
	SUBL	$1, DI
	MOVL	p2+4(FP), SI
/*
 * copy whole longs
 */
	MOVL	BX, CX
	SHRL	$2, CX
	REP;	MOVSL
/*
 * copy the rest, by bytes
 */
	ANDL	$3, BX
	MOVL	BX, CX
	REP;	MOVSB

	MOVL	p1+0(FP), AX
	RET

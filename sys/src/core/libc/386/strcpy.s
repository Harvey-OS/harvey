	TEXT	strcpy(SB),$0

	MOVL	$0, AX
	MOVL	$-1, CX
	CLD
/*
 * find end of second string
 */

	MOVL	p2+4(FP), DI
	REPN;	SCASB

	MOVL	DI, BX
	SUBL	p2+4(FP), BX

/*
 * copy the memory
 */
	MOVL	p1+0(FP), DI
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

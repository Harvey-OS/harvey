	TEXT	memcmp(SB),$0

	MOVL	n+8(FP), BX
	CMPL	BX, $0
	JEQ	none
	MOVL	p1+0(FP), DI
	MOVL	p2+4(FP), SI
	CLD
/*
 * first by longs
 */

	MOVL	BX, CX
	SHRL	$2, CX

	REP;	CMPSL
	JNE	found

/*
 * then by bytes
 */
	ANDL	$3, BX
	MOVL	BX, CX
	REP;	CMPSB
	JNE	found1

none:
	MOVL	$0, AX
	RET

/*
 * if long found,
 * back up and look by bytes
 */
found:
	MOVL	$4, CX
	SUBL	CX, DI
	SUBL	CX, SI
	REP;	CMPSB

found1:
	JLS	lt
	MOVL	$-1, AX
	RET
lt:
	MOVL	$1, AX
	RET

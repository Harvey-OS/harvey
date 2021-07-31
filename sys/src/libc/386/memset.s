	TEXT	memset(SB),$0

	CLD
	MOVL	p+0(FP), DI
	MOVBLZX	c+4(FP), AX
	MOVL	n+8(FP), BX
/*
 * if not enough bytes, just copy
 */
	CMPL	BX, $9
	JLS	c3
/*
 * build word in AX
 */
	MOVB	AL, AH
	MOVL	AX, CX
	SHLL	$16, CX
	ORL	CX, AX
/*
 * copy whole longs
 */
c1:
	MOVL	BX, CX
	SHRL	$2, CX
	ANDL	$3, BX
	REP;	STOSL
/*
 * copy the rest, by bytes
 */
c3:
	MOVL	BX, CX
	REP;	STOSB
ret:
	MOVL	p+0(FP),AX
	RET

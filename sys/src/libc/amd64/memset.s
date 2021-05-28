	TEXT	memset(SB),$0

	CLD
	MOVQ	RARG, DI
	MOVBLZX	c+8(FP), AX
	MOVL	n+16(FP), BX
/*
 * if not enough bytes, just set bytes
 */
	CMPL	BX, $9
	JLS	c3
/*
 * if not aligned, just set bytes
 */
	MOVQ	RARG, CX
	ANDL	$3,CX
	JNE	c3
/*
 * build word in AX
 */
	MOVB	AL, AH
	MOVL	AX, CX
	SHLL	$16, CX
	ORL	CX, AX
/*
 * set whole longs
 */
c1:
	MOVQ	BX, CX
	SHRQ	$2, CX
	ANDL	$3, BX
	REP;	STOSL
/*
 * set the rest, by bytes
 */
c3:
	MOVL	BX, CX
	REP;	STOSB
ret:
	MOVQ	RARG,AX
	RET

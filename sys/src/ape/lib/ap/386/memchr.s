	TEXT	memchr(SB),$0

	MOVL	n+8(FP), CX
	CMPL	CX, $0
	JEQ	none
	MOVL	p+0(FP), DI
	MOVBLZX	c+4(FP), AX
	CLD
/*
 * SCASB is memchr instruction
 */

	REPN;	SCASB
	JEQ	found

none:
	MOVL	$0, AX
	RET

found:
	MOVL	DI, AX
	SUBL	$1, AX
	RET

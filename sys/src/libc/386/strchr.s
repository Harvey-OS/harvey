	TEXT	strchr(SB),$0
/*
 * look for null
 */
	MOVL	p+0(FP), DI
	MOVL	$-1, CX
	MOVL	$0, AX
	CLD

	REPN;	SCASB

/*
 * look for real char
 */
	MOVL	DI, CX
	MOVL	p+0(FP), DI
	SUBL	DI, CX
	MOVBLZX	c+4(FP), AX

	REPN;	SCASB

	JEQ	found
	MOVL	$0, AX
	RET

found:
	MOVL	DI, AX
	SUBL	$1, AX
	RET

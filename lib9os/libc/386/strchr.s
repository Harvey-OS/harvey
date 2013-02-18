	TEXT	strchr(SB), $0

	MOVL	s+0(FP), DI
	MOVB	c+4(FP), AX
	CMPB	AX, $0
	JEQ	l2	/**/

/*
 * char is not null
 */
l1:
	MOVB	(DI), BX
	CMPB	BX, $0
	JEQ	ret0
	ADDL	$1, DI
	CMPB	AX, BX
	JNE	l1

	MOVL	DI, AX
	SUBL	$1, AX
	RET

/*
 * char is null
 */
l2:
	MOVL	$-1, CX
	CLD

	REPN;	SCASB

	MOVL	DI, AX
	SUBL	$1, AX
	RET

ret0:
	MOVL	$0, AX
	RET

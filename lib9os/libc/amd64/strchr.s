	TEXT	strchr(SB), $0

	MOVQ	RARG, DI
	MOVB	c+8(FP), AX
	CMPB	AX, $0
	JEQ	l2	/**/

/*
 * char is not null
 */
l1:
	MOVB	(DI), BX
	CMPB	BX, $0
	JEQ	ret0
	ADDQ	$1, DI
	CMPB	AX, BX
	JNE	l1

	MOVQ	DI, AX
	SUBQ	$1, AX
	RET

/*
 * char is null
 */
l2:
	MOVQ	$-1, CX
	CLD

	REPN;	SCASB

	MOVQ	DI, AX
	SUBQ	$1, AX
	RET

ret0:
	MOVQ	$0, AX
	RET

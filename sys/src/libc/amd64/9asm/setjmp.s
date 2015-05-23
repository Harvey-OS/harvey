TEXT	longjmp(SB), $0
	MOVL	r+8(FP), AX
	CMPL	AX, $0
	JNE	ok		/* ansi: "longjmp(0) => longjmp(1)" */
	MOVL	$1, AX		/* bless their pointed heads */
ok:
	MOVQ	0(RARG), SP	/* restore sp */
	MOVQ	8(RARG), BX	/* put return pc on the stack */
	MOVQ	BX, 0(SP)
	RET

TEXT	setjmp(SB), $0
	MOVQ	SP, 0(RARG)	/* store sp */
	MOVQ	0(SP), BX	/* store return pc */
	MOVQ	BX, 8(RARG)
	MOVL	$0, AX		/* return 0 */
	RET

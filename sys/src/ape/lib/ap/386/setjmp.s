TEXT	longjmp(SB), $0
	MOVL	r+4(FP), AX
	CMPL	AX, $0
	JNE	ok		/* ansi: "longjmp(0) => longjmp(1)" */
	MOVL	$1, AX		/* bless their pointed heads */
ok:	MOVL	l+0(FP), BX
	MOVL	0(BX), SP	/* restore sp */
	MOVL	4(BX), BX	/* put return pc on the stack */
	MOVL	BX, 0(SP)
	RET

TEXT	setjmp(SB), $0
	MOVL	l+0(FP), AX
	MOVL	SP, 0(AX)	/* store sp */
	MOVL	0(SP), BX	/* store return pc */
	MOVL	BX, 4(AX)
	MOVL	$0, AX		/* return 0 */
	RET

TEXT	sigsetjmp(SB), $0
	MOVL	buf+0(FP), AX
	MOVL	savemask+4(FP),BX
	MOVL	BX,0(AX)
	MOVL	$_psigblocked(SB),4(AX)
	MOVL	SP, 8(AX)	/* store sp */
	MOVL	0(SP), BX	/* store return pc */
	MOVL	BX, 12(AX)
	MOVL	$0, AX		/* return 0 */
	RET

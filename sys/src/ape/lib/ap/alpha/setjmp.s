TEXT	setjmp(SB), 1, $-8
	MOVL	R30, (R0)
	MOVL	R26, 4(R0)
	MOVQ	$0, R0
	RET

TEXT	sigsetjmp(SB), 1, $-8
	MOVL	savemask+4(FP), R3
	MOVL	R3, 0(R0)
	MOVL	$_psigblocked(SB), R3
	MOVL	R3, 4(R0)
	MOVL	R30, 8(R0)
	MOVL	R26, 12(R0)
	MOVQ	$0, R0
	RET

TEXT	longjmp(SB), 1, $-8
	MOVL	r+4(FP), R3
	BNE	R3, ok		/* ansi: "longjmp(0) => longjmp(1)" */
	MOVQ	$1, R3		/* bless their pointed heads */
ok:	MOVL	(R0), R30
	MOVL	4(R0), R26
	MOVL	R3, R0
	RET

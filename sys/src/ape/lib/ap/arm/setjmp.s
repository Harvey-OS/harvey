arg=0
link=14

TEXT	setjmp(SB), 1, $-4
	MOVW	SP, (R(arg))
	MOVW	R(link), 4(R(arg))
	MOVW	$0, R(arg)
	RET

TEXT	sigsetjmp(SB), 1, $-4
	MOVW	savemask+4(FP), R2
	MOVW	R2, 0(R(arg))
	MOVW	$_psigblocked(SB), R2
	MOVW	R2, 4(R(arg))
	MOVW	SP, 8(R(arg))
	MOVW	R(link), 12(R(arg))
	MOVW	$0, R(arg)
	RET

TEXT	longjmp(SB), 1, $-4
	MOVW	r+4(FP), R2
	CMP	$0, R2
	BNE	ok			/* ansi: "longjmp(0) => longjmp(1)" */
	MOVW	$1, R2		/* bless their pointed heads */
ok:	MOVW	(R(arg)), SP
	MOVW	4(R(arg)), R(link)
	MOVW	R2, R(arg)
	RET

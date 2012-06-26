arg=0
link=14
sp=13

TEXT	setjmp(SB), 1, $-4
	MOVW	R(sp), (R(arg+0))
	MOVW	R(link), 4(R(arg+0))
	MOVW	$0, R0
	RET

TEXT	sigsetjmp(SB), 1, $-4
	MOVW	savemask+4(FP), R(arg+2)
	MOVW	R(arg+2), 0(R(arg+0))
	MOVW	$_psigblocked(SB), R(arg+2)
	MOVW	R2, 4(R(arg+0))
	MOVW	R(sp), 8(R(arg+0))
	MOVW	R(link), 12(R(arg+0))
	MOVW	$0, R(arg+0)
	RET

TEXT	longjmp(SB), 1, $-4
	MOVW	r+4(FP), R(arg+2)
	CMP	$0, R(arg+2)
	BNE	ok			/* ansi: "longjmp(0) => longjmp(1)" */
	MOVW	$1, R(arg+2)		/* bless their pointed heads */
ok:	MOVW	(R(arg+0)), R(sp)
	MOVW	4(R(arg+0)), R(link)
	MOVW	R(arg+2), R(arg+0)
	RET

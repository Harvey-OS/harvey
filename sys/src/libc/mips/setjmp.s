TEXT	setjmp(SB), 1, $-4
	MOVW	R29, (R1)
	MOVW	R31, 4(R1)
	MOVW	$0, R1
	RET

TEXT	longjmp(SB), 1, $-4
	MOVW	r+4(FP), R3
	BNE	R3, ok		/* ansi: "longjmp(0) => longjmp(1)" */
	MOVW	$1, R3		/* bless their pointed heads */
ok:	MOVW	(R1), R29
	MOVW	4(R1), R31
	MOVW	R3, R1
	RET

TEXT	setjmp(SB), 1, $-8
	MOVV	R29, (R1)
	MOVV	R31, 8(R1)
	MOVV	$0, R1
	RET

TEXT	sigsetjmp(SB), 1, $-8
	MOVV	savemask+4(FP), R2
	MOVV	R2, 0(R1)
	MOVV	$_psigblocked(SB), R2
	MOVV	R2, 8(R1)
	MOVV	R29, 16(R1)
	MOVV	R31, 24(R1)
	MOVV	$0, R1
	RET

TEXT	longjmp(SB), 1, $-8
	MOVW	r+8(FP), R3
	BNE	R3, ok		/* ansi: "longjmp(0) => longjmp(1)" */
	MOVW	$1, R3		/* bless their pointed heads */
ok:	MOVV	(R1), R29
	MOVV	8(R1), R31
	MOVV	R3, R1
	RET

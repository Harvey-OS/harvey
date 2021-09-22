TEXT	setjmp(SB), 1, $-4
	MOVW	R29, (R1)	/* store sp */
	MOVW	R31, 4(R1)	/* store return pc */
	MOVW	$0, R1		/* return 0 */
	RET

TEXT	longjmp(SB), 1, $-4
	MOVW	r+4(FP), R3
	BNE	R3, ok		/* ansi: "longjmp(0) => longjmp(1)" */
	MOVW	$1, R3		/* bless their pointed heads */
ok:	MOVW	(R1), R29	/* restore sp */
	MOVW	4(R1), R31	/* restore return pc */
	MOVW	R3, R1
	RET

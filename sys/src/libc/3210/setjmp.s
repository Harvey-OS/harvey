TEXT	setjmp(SB), 1, $0

	MOVW	R1, (R3)+		/* sp */
	MOVW	R18, (R3)		/* link reg */
	MOVW	$0, R3
	RETURN

TEXT	longjmp(SB), 1, $0

	MOVW	R3, R4
	MOVW	$r+4(FP), R3
	MOVW	(R3), R3
	CMP	R3, R0
	BRA	NE, ok		/* ansi: "longjmp(0) => longjmp(1)" */
	MOVW	$1, R3		/* bless their pointed heads */
ok:	MOVW	(R4)+, R1
	MOVW	(R4), R18
	RETURN

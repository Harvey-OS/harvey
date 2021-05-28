TEXT	setjmp(SB), 1, $-8
	MOVD	LR, R4
	MOVD	R1, (R3)
	MOVD	R4, 4(R3)
	MOVW	$0, R3
	RETURN

TEXT	longjmp(SB), 1, $-8
	MOVD	R3, R4
	MOVW	r+12(FP), R3
	CMP	R3, $0
	BNE	ok		/* ansi: "longjmp(0) => longjmp(1)" */
	MOVW	$1, R3		/* bless their pointed heads */
ok:	MOVD	(R4), R1
	MOVD	4(R4), R4
	MOVD	R4, LR
	BR	(LR)

/*
 * trampoline functions because the kernel smashes r3
 * in the uregs given to notejmp
 */
TEXT	__noterestore(SB), 1, $-8
	MOVD	R4, R3
	MOVD	R5, LR
	BR	(LR)

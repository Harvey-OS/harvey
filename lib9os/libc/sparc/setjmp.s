TEXT	setjmp(SB), 1, $0

	MOVW	R1, (R7)
	MOVW	R15, 4(R7)
	MOVW	$0, R7
	RETURN

TEXT	longjmp(SB), 1, $0

	MOVW	R7, R8
	MOVW	r+4(FP), R7
	CMP	R7, R0
	BNE	ok		/* ansi: "longjmp(0) => longjmp(1)" */
	MOVW	$1, R7		/* bless their pointed heads */
ok:	MOVW	(R8), R1
	MOVW	4(R8), R15
	RETURN

/*
 * trampoline functions because the kernel smashes r7
 * in the uregs given to notejmp
 */
TEXT	__noterestore(SB), 1, $-4
	MOVW	R8, R7
	JMP	(R9)

	TEXT	tramp(SB), 1, $0
	ADD	$-8, R3, R4		/* new stack */
	MOVW	4(FP), R5		/* func to exec */
	MOVW	R5, LR
	MOVW	8(FP), R3		/* arg to reg */
	MOVW	R4, R1			/* new stack */
	BL	(LR)
	MOVW	R0, R3
	MOVW	$_exits(SB), R4
	MOVW	R4, LR
	BR	(LR)

	TEXT	vstack(SB), 1, $0	/* Passes &targ through R3 */
	MOVW	ustack(SB), R1
	MOVW	$exectramp(SB), R4
	MOVW	R4, CTR
	BR	(CTR)
	RETURN

	TEXT	FPsave(SB), 1, $0
	MOVFL	FPSCR, F0
	FMOVD	F0, 0(R3)
	RETURN

	TEXT	FPrestore(SB), 1, $0
	FMOVD	0(R3), F0
	MOVFL	F0, FPSCR
	RETURN
